#pragma once
#include <string>
#include <vector>
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,
    CONFIG_VALUE(BeatLeaderToken, std::string, "BeatLeaderToken", "");
    CONFIG_VALUE(BeatLeaderUsername, std::string, "BeatLeaderUsername", "");
    CONFIG_VALUE(BeatLeaderUserId, std::string, "BeatLeaderUserId", "");
    CONFIG_VALUE(ShowFriendLeaderboard, bool, "ShowFriendLeaderboard", true);
    CONFIG_VALUE(SnipeNotifications, bool, "SnipeNotifications", true);
    CONFIG_VALUE(UIScale, float, "UIScale", 1.0f);
)
