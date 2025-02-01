#ifndef INTERACTIONPROFILES_H
#define INTERACTIONPROFILES_H

#include <components/stereo/types.hpp>
#include <components/vr/constants.hpp>
#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <filesystem>

namespace XR
{

    struct Interaction
    {
        enum ValueType
        {
            BOOLEAN,
            FLOAT,
            AXIS,
            POSE
        };

        ValueType valueType;
        std::string path;
        std::optional<std::string> requiredExtension = std::nullopt;
    };
    struct InteractionProfile
    {
        std::string path;
        std::optional<std::string> requiredExtension = std::nullopt;
    };
    using Interactions = std::vector<Interaction>;
    using Controllers = std::map<std::string, Interactions>;
    using InteractionProfiles = std::vector<std::pair<InteractionProfile, Controllers>>;

    Interaction::ValueType ParseValueType(const char* typeStr);

    const char* ValueTypeToString(Interaction::ValueType valueType);

    const InteractionProfiles& getAllKnownInteractionProfiles();
    const std::set<std::string>& getAllKnownInteractionProfileExtensions();
    void loadInteractionProfiles(const std::filesystem::path& xrControllerSuggestionsFile,
        const std::filesystem::path& defaultXrControllerSuggestionsFile);
}

#endif
