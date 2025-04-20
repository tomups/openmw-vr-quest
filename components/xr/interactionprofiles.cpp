#include "interactionprofiles.hpp"
#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <extern/oics/tinyxml.h>
#include <set>
#include <cstring>

namespace
{
    using InteractionProfilesXML = std::vector<std::pair<std::vector<XR::InteractionProfile>, XR::Controllers>>;
    std::set<std::string> sRequestTheseExtensions;
    constexpr const char* LEFT_HAND = "/user/hand/left";
    constexpr const char* RIGHT_HAND = "/user/hand/right";
    constexpr const char* BOTH_HANDS = "/user/hand/*";
    //constexpr const char* GAMEPAD = "/user/gamepad";

    // Note: LoadInteractionProfilesFromXML was mostly generated using ChatGPT
    //------------------------------------------------------------------------------
    // Loads an XML file into an InteractionProfiles object
    //------------------------------------------------------------------------------
    std::optional<InteractionProfilesXML> LoadInteractionProfilesFromXML(const std::filesystem::path& xmlFilePath)
    {
        InteractionProfilesXML outProfiles;

        TiXmlDocument doc;
        if (!doc.LoadFile(xmlFilePath.string().c_str()))
        {
            std::cerr << "ERROR: Failed to load XML file '" << xmlFilePath << "'\n";
            return std::nullopt;
        }

        // <InteractionProfiles> is the root element
        TiXmlElement* root = doc.FirstChildElement("InteractionProfileDatabase");
        if (!root)
        {
            std::cerr << "ERROR: Missing <InteractionProfileDatabase> root element.\n";
            return std::nullopt;
        }

        // Loop through each <Entry> child of <InteractionProfileDatabase>
        for (TiXmlElement* profileElem = root->FirstChildElement("Entry"); profileElem != nullptr;
             profileElem = profileElem->NextSiblingElement("Entry"))
        {
            // Each Entry has:
            // (1) <InteractionProfiles> -> list of <InteractionProfile>
            // (2) <InteractionsMap> -> list of <Controller> -> child <Interaction>

            std::pair<std::vector<XR::InteractionProfile>, XR::Controllers> singleProfile;
            auto& interactionProfilesVector = singleProfile.first;
            auto& controllersMap = singleProfile.second;

            // 1. Parse <InteractionProfiles>
            {
                TiXmlElement* interactionProfilesElem = profileElem->FirstChildElement("InteractionProfiles");
                if (interactionProfilesElem)
                {
                    for (TiXmlElement* ipElem = interactionProfilesElem->FirstChildElement("InteractionProfile");
                         ipElem != nullptr; ipElem = ipElem->NextSiblingElement("InteractionProfile"))
                    {
                        const char* pathAttr = ipElem->Attribute("path");
                        if (!pathAttr)
                        {
                            // If 'path' is missing, skip this Controller
                            continue;
                        }

                        XR::InteractionProfile ip;
                        ip.path = pathAttr;

                        if (const char* extAttr = ipElem->Attribute("requiredExtension"))
                        {
                            ip.requiredExtension = extAttr;
                            sRequestTheseExtensions.insert(extAttr);
                        }

                        interactionProfilesVector.push_back(ip);
                    }
                }
            }

            // 2. Parse <ControllersMap>
            {
                TiXmlElement* interactionsMapElem = profileElem->FirstChildElement("InteractionsMap");
                if (interactionsMapElem)
                {
                    // Each child <Controller> in ControllersMap
                    for (TiXmlElement* ctrlElem = interactionsMapElem->FirstChildElement("Controller");
                         ctrlElem != nullptr; ctrlElem = ctrlElem->NextSiblingElement("Controller"))
                    {
                        const char* controllerPath = ctrlElem->Attribute("path");
                        if (!controllerPath)
                        {
                            // If path is missing, skip
                            continue;
                        }


                        XR::Interactions interactionsList;

                        // For each <Interaction> child
                        for (TiXmlElement* interactionElem = ctrlElem->FirstChildElement("Interaction");
                             interactionElem != nullptr;
                             interactionElem = interactionElem->NextSiblingElement("Interaction"))
                        {
                            const char* typeStr = interactionElem->Attribute("type");
                            const char* pathStr = interactionElem->Attribute("path");

                            if (!pathStr)
                            {
                                // Missing path -> skip
                                continue;
                            }

                            XR::Interaction interaction;
                            interaction.valueType = XR::ParseValueType(typeStr);
                            interaction.path = pathStr;

                            if (const char* extAttr = interactionElem->Attribute("requiredExtension"))
                            {
                                interaction.requiredExtension = extAttr;
                                sRequestTheseExtensions.insert(extAttr);
                            }

                            interactionsList.push_back(interaction);
                        }

                        // Put into the map: key = controllerPath, value = interactionsList
                        if (std::string(controllerPath) == BOTH_HANDS)
                        {
                            auto& left = controllersMap[LEFT_HAND];
                            left.insert(left.end(), interactionsList.begin(), interactionsList.end());
                            auto& right = controllersMap[RIGHT_HAND];
                            right.insert(right.end(), interactionsList.begin(), interactionsList.end());
                        }
                        else
                        {
                            auto& list = controllersMap[controllerPath];
                            list.insert(list.end(), interactionsList.begin(), interactionsList.end());
                        }
                    }
                }
            }

            // Now we have parsed this single <Profile>, add to outProfiles
            outProfiles.push_back(singleProfile);
        }

        return outProfiles;
    }

    XR::InteractionProfiles sInteractionProfiles;
}

namespace XR
{
    Interaction::ValueType ParseValueType(const char* typeStr)
    {
        if (!typeStr)
            return Interaction::BOOLEAN; // default/fallback

        if (std::strcmp(typeStr, "BOOLEAN") == 0)
            return Interaction::BOOLEAN;
        if (std::strcmp(typeStr, "FLOAT") == 0)
            return Interaction::FLOAT;
        if (std::strcmp(typeStr, "AXIS") == 0)
            return Interaction::AXIS;
        if (std::strcmp(typeStr, "POSE") == 0)
            return Interaction::POSE;

        // Unknown or missing -> fallback
        return Interaction::BOOLEAN;
    }

    const char* ValueTypeToString(Interaction::ValueType valueType)
    {
        switch (valueType)
        {
            case Interaction::BOOLEAN:
                return "BOOLEAN";
            case Interaction::FLOAT:
                return "FLOAT";
            case Interaction::AXIS:
                return "AXIS";
            case Interaction::POSE:
                return "POSE";
            default:
                return "UNKNOWN";
        }
    }

    const InteractionProfiles& getAllKnownInteractionProfiles()
    {
        return sInteractionProfiles;
    }

    const std::set<std::string>& getAllKnownInteractionProfileExtensions()
    {
        return sRequestTheseExtensions;
    }

    void loadInteractionProfiles(const std::filesystem::path& path)
    {
        if(auto profiles_ = LoadInteractionProfilesFromXML(path))
        {
            const auto& profiles = *profiles_;
            for (auto& it : profiles)
            {
                for (auto& controller : it.first)
                {
                    sInteractionProfiles.push_back(std::make_pair(controller, it.second));
                }
            }
        }
    }

    void loadInteractionProfiles(const std::filesystem::path& xrInputProfiles, const std::filesystem::path& defaultXrInputProfiles)
    {
        if (!std::filesystem::exists(defaultXrInputProfiles))
            throw std::runtime_error(
                "Unable to locate xr input profiles xml database: " + defaultXrInputProfiles.string());
        loadInteractionProfiles(defaultXrInputProfiles);
        if (std::filesystem::exists(xrInputProfiles) && xrInputProfiles != defaultXrInputProfiles)
        {
            loadInteractionProfiles(xrInputProfiles);
        }
    }
}
