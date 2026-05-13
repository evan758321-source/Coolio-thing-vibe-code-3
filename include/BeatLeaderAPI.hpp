#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include "beatsaber-hook/shared/utils/logging.hpp"

namespace FriendLeaderboard {

    struct PlayerScore {
        std::string playerId;
        std::string playerName;
        std::string playerAvatar;
        int         rank;
        float       accuracy;
        int         score;
        int         pp;
        bool        isLocalPlayer;
        bool        isFriend;

        bool operator<(const PlayerScore& other) const {
            return score > other.score; // descending
        }
    };

    struct LeaderboardData {
        std::string   mapHash;
        std::string   difficulty;
        std::string   characteristic;
        std::vector<PlayerScore> scores; // sorted by score desc
    };

    struct AuthResult {
        bool        success;
        std::string token;
        std::string userId;
        std::string username;
        std::string errorMessage;
    };

    // ---- Callbacks ----
    using AuthCallback        = std::function<void(AuthResult)>;
    using LeaderboardCallback = std::function<void(std::optional<LeaderboardData>)>;
    using FriendsCallback     = std::function<void(std::vector<PlayerScore>)>;

    // ---- API ----
    class BeatLeaderAPI {
    public:
        static BeatLeaderAPI& GetInstance();

        // Authenticate with username + password (OculusID approach via BL OAuth)
        // BeatLeader uses OAuth / cookie auth — we store the session token
        void Authenticate(const std::string& login, const std::string& password, AuthCallback cb);

        // Fetch the friend leaderboard for a given map+diff
        void FetchFriendLeaderboard(
            const std::string& mapHash,
            const std::string& difficulty,
            const std::string& characteristic,
            const std::string& token,
            LeaderboardCallback cb
        );

        // Fetch friend list for the local player
        void FetchFriends(const std::string& playerId, const std::string& token, FriendsCallback cb);

        // Get local player's personal best on a map
        void FetchMyScore(
            const std::string& mapHash,
            const std::string& difficulty,
            const std::string& characteristic,
            const std::string& playerId,
            const std::string& token,
            std::function<void(std::optional<PlayerScore>)> cb
        );

    private:
        BeatLeaderAPI() = default;

        void HttpGet(const std::string& url,
                     const std::string& token,
                     std::function<void(bool, std::string)> cb);

        void HttpPost(const std::string& url,
                      const std::string& body,
                      std::function<void(bool, std::string)> cb);

        static const std::string BASE_URL;
    };

} // namespace FriendLeaderboard
