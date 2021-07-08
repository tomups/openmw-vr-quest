#include "shadermanager.hpp"

#include <fstream>
#include <algorithm>
#include <sstream>
#include <regex>

#include <osg/Program>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/sceneutil/lightmanager.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

namespace Shader
{

    ShaderManager::ShaderManager()
        : mLightingMethod(SceneUtil::LightingMethod::FFP)
    {
    }

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    void ShaderManager::setLightingMethod(SceneUtil::LightingMethod method)
    {
        mLightingMethod = method;
    }

    bool addLineDirectivesAfterConditionalBlocks(std::string& source)
    {
        for (size_t position = 0; position < source.length(); )
        {
            size_t foundPos = source.find("#endif", position);
            foundPos = std::min(foundPos, source.find("#elif", position));
            foundPos = std::min(foundPos, source.find("#else", position));

            if (foundPos == std::string::npos)
                break;

            foundPos = source.find_first_of("\n\r", foundPos);
            foundPos = source.find_first_not_of("\n\r", foundPos);

            if (foundPos == std::string::npos)
                break;

            size_t lineDirectivePosition = source.rfind("#line", foundPos);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString) - 1;
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 1;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + foundPos, '\n');

            source.replace(foundPos, 0, "#line " + std::to_string(lineNumber) + "\n");

            position = foundPos;
        }

        return true;
    }

    // Recursively replaces include statements with the actual source of the included files.
    // Adjusts #line statements accordingly and detects cyclic includes.
    // includingFiles is the set of files that include this file directly or indirectly, and is intentionally not a reference to allow automatic cleanup.
    static bool parseIncludes(const boost::filesystem::path& shaderPath, std::string& source, const std::string& fileName, int& fileNumber, std::set<boost::filesystem::path> includingFiles)
    {
        // An include is cyclic if it is being included by itself
        if (includingFiles.insert(shaderPath/fileName).second == false)
        {
            Log(Debug::Error) << "Shader " << fileName << " error: Detected cyclic #includes";
            return false;
        }

        Misc::StringUtils::replaceAll(source, "\r\n", "\n");

        size_t foundPos = 0;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size() - 1)
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Invalid #include";
                return false;
            }
            size_t end = source.find('"', start + 1);
            if (end == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Invalid #include";
                return false;
            }
            std::string includeFilename = source.substr(start + 1, end - (start + 1));
            boost::filesystem::path includePath = shaderPath / includeFilename;

            // Determine the line number that will be used for the #line directive following the included source
            size_t lineDirectivePosition = source.rfind("#line", foundPos);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString) - 1;
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 0;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + foundPos, '\n');

            // Include the file recursively
            boost::filesystem::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Failed to open include " << includePath.string();
                return false;
            }
            int includedFileNumber = fileNumber++;

            std::stringstream buffer;
            buffer << includeFstream.rdbuf();
            std::string stringRepresentation = buffer.str();
            if (!addLineDirectivesAfterConditionalBlocks(stringRepresentation)
                || !parseIncludes(shaderPath, stringRepresentation, includeFilename, fileNumber, includingFiles))
            {
                Log(Debug::Error) << "In file included from " << fileName << "." << lineNumber;
                return false;
            }

            std::stringstream toInsert;
            toInsert << "#line 0 " << includedFileNumber << "\n" << stringRepresentation << "\n#line " << lineNumber << " 0\n";

            source.replace(foundPos, (end - foundPos + 1), toInsert.str());
        }
        return true;
    }

    struct DeclarationMeta
    {
        std::string interpolationType;
        std::string interfaceKeyword;
        std::string type;
        std::string identifier;
        std::string mangledIdentifier;
    };

    // Mangle identifiers of the interface declarations of this shader source, updating all identifiers, and returning a list of the declarations for use in generating
    // a geometry shader.
    // IN/OUT source: The source to mangle
    // IN interfaceKeywordPattern: A regular expression matching all interface keywords to look for (e.g. "out|varying" when mangling output variables). Must not contain subexpressions.
    // IN mangleString: Identifiers are mangled by prepending this string. Must be a valid identifier prefix.
    // OUT declarations: All mangled declarations are added to this vector. Includes interpolation, interface, and type information as well as both the mangled and unmangled identifier.
    static void mangleInterface(std::string& source, const std::string& interfaceKeywordPattern, const std::string& mangleString, std::vector<DeclarationMeta>& declarations)
    {
        std::string commentPattern = "//.*";
        std::regex commentRegex(commentPattern);
        std::string commentlessSource = std::regex_replace(source, commentRegex, "");

        std::string identifierPattern = "[a-zA-Z_][0-9a-zA-Z_]*";
        std::string declarationPattern = "(centroid|flat)?\\s*\\b(" + interfaceKeywordPattern + ")\\s+(" + identifierPattern + ")\\s+(" + identifierPattern + ")\\s*;";
        std::regex declarationRegex(declarationPattern);
        
        std::vector<std::smatch> matches(std::sregex_iterator(commentlessSource.begin(), commentlessSource.end(), declarationRegex), std::sregex_iterator());
        std::string replacementPattern;
        for (auto& match : matches)
        {
            declarations.emplace_back(DeclarationMeta{ match[1].str(), match[2].str(), match[3].str(), match[4].str(), mangleString + match[4].str() });
            if (!replacementPattern.empty())
                replacementPattern += "|";
            replacementPattern = replacementPattern + "(" + declarations.back().identifier + "\\b)";
        }

        if (!replacementPattern.empty())
        {
            std::regex replacementRegex(replacementPattern);
            source = std::regex_replace(source, replacementRegex, mangleString + "$&");
        }
    }

    static std::string generateGeometryShader(const std::string& geometryTemplate, const std::vector<DeclarationMeta>& declarations)
    {
        if (geometryTemplate.empty())
            return "";

        static std::map<std::string, std::string> overriddenForwardStatements =
        {
            {"linearDepth", "linearDepth = gl_Position.z;"},
            {"euclideanDepth", "euclideanDepth = length(viewPos.xyz);"},
            {"passViewPos", "passViewPos = viewPos.xyz;"},
            {
                "screenCoordsPassthrough",
                "                mat4 scalemat = mat4(0.25, 0.0, 0.0, 0.0,\n"
                "                    0.0, -0.5, 0.0, 0.0,\n"
                "                    0.0, 0.0, 0.5, 0.0,\n"
                "                    0.25, 0.5, 0.5, 1.0);\n"
                "                vec4 texcoordProj = ((scalemat) * (gl_Position));\n"
                "                screenCoordsPassthrough = texcoordProj.xyw;\n"
                "                if(viewport == 1)\n"
                "                    screenCoordsPassthrough.x += 0.5 * screenCoordsPassthrough.z;\n"
            }
        };

        std::stringstream ssInputDeclarations;
        std::stringstream ssOutputDeclarations;
        std::stringstream ssForwardStatements;
        std::stringstream ssExtraStatements;
        std::set<std::string> identifiers;
        for (auto& declaration : declarations)
        {
            if (!declaration.interpolationType.empty())
            {
                ssInputDeclarations << declaration.interpolationType << " ";
                ssOutputDeclarations << declaration.interpolationType << " ";
            }
            ssInputDeclarations << "in " << declaration.type << " " << declaration.mangledIdentifier << "[3];\n";
            ssOutputDeclarations << "out " << declaration.type << " " << declaration.identifier << ";\n";

            if (overriddenForwardStatements.count(declaration.identifier) > 0)
                ssForwardStatements << overriddenForwardStatements[declaration.identifier] << ";\n";
            else
                ssForwardStatements << "            " << declaration.identifier << " = " << declaration.mangledIdentifier << "[vertex];\n";

            identifiers.insert(declaration.identifier);
        }

        // passViewPos output is required
        if (identifiers.find("passViewPos") == identifiers.end())
        {
            Log(Debug::Error) << "Vertex shader is missing 'vec3 passViewPos' on its interface. Geometry shader will NOT work.";
            return "";
        }

        std::string geometryShader = geometryTemplate;
        geometryShader = std::regex_replace(geometryShader, std::regex("@INPUTS"), ssInputDeclarations.str());
        geometryShader = std::regex_replace(geometryShader, std::regex("@OUTPUTS"), ssOutputDeclarations.str());
        geometryShader = std::regex_replace(geometryShader, std::regex("@FORWARDING"), ssForwardStatements.str());

        return geometryShader;
    }

    bool parseFors(std::string& source, const std::string& templateName)
    {
        const char escapeCharacter = '$';
        size_t foundPos = 0;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string command = source.substr(foundPos + 1, endPos - (foundPos + 1));
            if (command != "foreach")
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unknown shader directive: $" << command;
                return false;
            }

            size_t iterNameStart = endPos + 1;
            size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
            if (iterNameEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string iteratorName = "$" + source.substr(iterNameStart, iterNameEnd - iterNameStart);

            size_t listStart = iterNameEnd + 1;
            size_t listEnd = source.find_first_of("\n\r", listStart);
            if (listEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string list = source.substr(listStart, listEnd - listStart);
            std::vector<std::string> listElements;
            if (list != "")
                Misc::StringUtils::split(list, listElements, ",");

            size_t contentStart = source.find_first_not_of("\n\r", listEnd);
            size_t contentEnd = source.find("$endforeach", contentStart);
            if (contentEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string content = source.substr(contentStart, contentEnd - contentStart);

            size_t overallEnd = contentEnd + std::string("$endforeach").length();

            size_t lineDirectivePosition = source.rfind("#line", overallEnd);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString);
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 2;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + overallEnd, '\n');

            std::string replacement = "";
            for (std::vector<std::string>::const_iterator element = listElements.cbegin(); element != listElements.cend(); element++)
            {
                std::string contentInstance = content;
                size_t foundIterator;
                while ((foundIterator = contentInstance.find(iteratorName)) != std::string::npos)
                    contentInstance.replace(foundIterator, iteratorName.length(), *element);
                replacement += contentInstance;
            }
            replacement += "\n#line " + std::to_string(lineNumber);
            source.replace(foundPos, overallEnd - foundPos, replacement);
        }

        return true;
    }

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines,
        const ShaderManager::DefineMap& globalDefines, const std::string& templateName)
    {
        const char escapeCharacter = '@';
        size_t foundPos = 0;
        std::vector<std::string> forIterators;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string define = source.substr(foundPos + 1, endPos - (foundPos + 1));
            ShaderManager::DefineMap::const_iterator defineFound = defines.find(define);
            ShaderManager::DefineMap::const_iterator globalDefineFound = globalDefines.find(define);
            if (define == "foreach")
            {
                source.replace(foundPos, 1, "$");
                size_t iterNameStart = endPos + 1;
                size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
                if (iterNameEnd == std::string::npos)
                {
                    Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                    return false;
                }
                forIterators.push_back(source.substr(iterNameStart, iterNameEnd - iterNameStart));
            }
            else if (define == "endforeach")
            {
                source.replace(foundPos, 1, "$");
                if (forIterators.empty())
                {
                    Log(Debug::Error) << "Shader " << templateName << " error: endforeach without foreach";
                    return false;
                }
                else
                    forIterators.pop_back();
            }
            else if (std::find(forIterators.begin(), forIterators.end(), define) != forIterators.end())
            {
                source.replace(foundPos, 1, "$");
            }
            else if (defineFound != defines.end())
            {
                source.replace(foundPos, endPos - foundPos, defineFound->second);
            }
            else if (globalDefineFound != globalDefines.end())
            {
                source.replace(foundPos, endPos - foundPos, globalDefineFound->second);
            }
            else
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Undefined " << define;
                return false;
            }
        }
        return true;
    }

    osg::ref_ptr<osg::Shader> ShaderManager::getShader(const std::string& templateName, const ShaderManager::DefineMap& defines, osg::Shader::Type shaderType)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(templateName, defines));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = getTemplateSource(templateName);

            if (!parseDefines(shaderSource, defines, mGlobalDefines, templateName) || !parseFors(shaderSource, templateName))
            {
                // Add to the cache anyway to avoid logging the same error over and over.
                mShaders.insert(std::make_pair(std::make_pair(templateName, defines), nullptr));
                return nullptr;
            }

            osg::ref_ptr<osg::Shader> shader(new osg::Shader(shaderType));
            // Assign a unique prefix to allow the SharedStateManager to compare shaders efficiently.
            // Append shader source filename for debugging.
            static unsigned int counter = 0;
            shader->setName(Misc::StringUtils::format("%u %s", counter++, templateName));

            if (mGeometryShadersEnabled && defines.count("geometryShader") && defines.find("geometryShader")->second == "1" && shaderType == osg::Shader::VERTEX)
            {
                std::vector<DeclarationMeta> declarations;
                mangleInterface(shaderSource, "out|varying", "vertex_", declarations);
                std::string geometryTemplate = getTemplateSource("stereo_geometry.glsl");
                std::string geometryShaderSource = generateGeometryShader(geometryTemplate, declarations);
                if (!geometryShaderSource.empty())
                {
                    osg::ref_ptr<osg::Shader> geometryShader(new osg::Shader(osg::Shader::GEOMETRY));
                    geometryShader->setShaderSource(geometryShaderSource);
                    geometryShader->setName(shader->getName() + ".geom");
                    mGeometryShaders[shader] = geometryShader;
                }
                else
                {
                    Log(Debug::Error) << "Failed to generate geometry shader for " << templateName;
                }
            }

            shader->setShaderSource(shaderSource);

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(templateName, defines), shader)).first;
        }
        return shaderIt->second;
    }

    osg::ref_ptr<osg::Program> ShaderManager::getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        ProgramMap::iterator found = mPrograms.find(std::make_pair(vertexShader, fragmentShader));
        if (found == mPrograms.end())
        {
            osg::ref_ptr<osg::Program> program(new osg::Program);
            program->addShader(vertexShader);
            program->addShader(fragmentShader);
            program->addBindAttribLocation("aOffset", 6);
            program->addBindAttribLocation("aRotation", 7);

            auto git = mGeometryShaders.find(vertexShader);
            if (git != mGeometryShaders.end())
            {
                program->addShader(git->second);
            }

            if (mLightingMethod == SceneUtil::LightingMethod::SingleUBO)
                program->addBindUniformBlock("LightBufferBinding", static_cast<int>(UBOBinding::LightBuffer));
            found = mPrograms.insert(std::make_pair(std::make_pair(vertexShader, fragmentShader), program)).first;
        }
        return found->second;
    }

    ShaderManager::DefineMap ShaderManager::getGlobalDefines()
    {
        return DefineMap(mGlobalDefines);
    }

    void ShaderManager::setStereoGeometryShaderEnabled(bool enabled)
    {
        mGeometryShadersEnabled = enabled;
    }

    bool ShaderManager::stereoGeometryShaderEnabled() const
    {
        return mGeometryShadersEnabled;
    }

    void ShaderManager::setGlobalDefines(DefineMap& globalDefines)
    {
        mGlobalDefines = globalDefines;
        for (auto shaderMapElement : mShaders)
        {
            std::string templateId = shaderMapElement.first.first;
            ShaderManager::DefineMap defines = shaderMapElement.first.second;
            osg::ref_ptr<osg::Shader> shader = shaderMapElement.second;
            if (shader == nullptr)
                // I'm not sure how to handle a shader that was already broken as there's no way to get a potential replacement to the nodes that need it.
                continue;
            std::string shaderSource = mShaderTemplates[templateId];
            if (!parseDefines(shaderSource, defines, mGlobalDefines, templateId) || !parseFors(shaderSource, templateId))
                // We just broke the shader and there's no way to force existing objects back to fixed-function mode as we would when creating the shader.
                // If we put a nullptr in the shader map, we just lose the ability to put a working one in later.
                continue;
            shader->setShaderSource(shaderSource);
        }
    }

    void ShaderManager::releaseGLObjects(osg::State* state)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto shader : mShaders)
        {
            if (shader.second != nullptr)
                shader.second->releaseGLObjects(state);
        }
        for (auto program : mPrograms)
            program.second->releaseGLObjects(state);
    }

    std::string ShaderManager::getTemplateSource(const std::string& templateName)
    {
        // read the template if we haven't already
        TemplateMap::iterator templateIt = mShaderTemplates.find(templateName);
        if (templateIt == mShaderTemplates.end())
        {
            boost::filesystem::path path = (boost::filesystem::path(mPath) / templateName);
            boost::filesystem::ifstream stream;
            stream.open(path);
            if (stream.fail())
            {
                Log(Debug::Error) << "Failed to open " << path.string();
                return std::string();
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            // parse includes
            int fileNumber = 1;
            std::string source = buffer.str();
            if (!addLineDirectivesAfterConditionalBlocks(source)
                || !parseIncludes(boost::filesystem::path(mPath), source, templateName, fileNumber, {}))
                return std::string();

            templateIt = mShaderTemplates.insert(std::make_pair(templateName, source)).first;
        }
        return templateIt->second;
    }

}
