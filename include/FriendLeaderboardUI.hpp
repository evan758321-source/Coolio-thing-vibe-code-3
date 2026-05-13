#pragma once
#include <vector>
#include <string>
#include "BeatLeaderAPI.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/Image.hpp"

namespace FriendLeaderboard {

    // The in-game HUD panel on the right side of the screen
    class FriendLeaderboardUI {
    public:
        static FriendLeaderboardUI& GetInstance();

        // Called when a map starts
        void OnMapStart(const std::string& mapHash,
                        const std::string& difficulty,
                        const std::string& characteristic);

        // Called every score update from the score controller
        void OnScoreUpdated(int newScore, float newAccuracy);

        // Called when the map ends / player exits
        void OnMapEnd();

        // Build / destroy the UI root
        void CreateUI();
        void DestroyUI();

        bool IsVisible() const { return m_visible; }

    private:
        FriendLeaderboardUI() = default;

        void RefreshDisplay();
        void CheckForSnipes(int newScore);
        void BuildEntryRow(const PlayerScore& ps, int rowIndex, bool isLocalPlayer);
        void AnimateRowMovement(int fromIndex, int toIndex);

        UnityEngine::GameObject* m_root     = nullptr;
        TMPro::TextMeshProUGUI*  m_header   = nullptr;

        struct UIRow {
            UnityEngine::GameObject*    go        = nullptr;
            TMPro::TextMeshProUGUI*     rankText  = nullptr;
            TMPro::TextMeshProUGUI*     nameText  = nullptr;
            TMPro::TextMeshProUGUI*     scoreText = nullptr;
            TMPro::TextMeshProUGUI*     accText   = nullptr;
            UnityEngine::UI::Image*     bg        = nullptr;
            std::string                 playerId;
        };

        std::vector<UIRow>   m_rows;
        LeaderboardData      m_data;
        PlayerScore          m_localScore;
        bool                 m_visible       = false;
        bool                 m_dataLoaded    = false;
        std::string          m_currentHash;
        std::string          m_currentDiff;
        std::string          m_currentChar;
        int                  m_localRank     = -1;

        // track which friends we've already sniped this session
        std::vector<std::string> m_snipedIds;
    };

} // namespace FriendLeaderboard
