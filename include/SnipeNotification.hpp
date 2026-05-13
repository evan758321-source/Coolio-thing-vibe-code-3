#pragma once
#include <string>

namespace UnityEngine { class GameObject; }
namespace TMPro { class TextMeshProUGUI; }

namespace FriendLeaderboard {

    // Hit-score-visualizer style popup that shows "Sniped <username>"
    class SnipeNotification {
    public:
        static SnipeNotification& GetInstance();

        // Triggers the popup with the sniped player's name
        void ShowSnipe(const std::string& snipedUsername);

    private:
        SnipeNotification() = default;

        // Coroutine-style animate out after delay
        void AnimateOut(UnityEngine::GameObject* popup);

        static constexpr float DISPLAY_DURATION    = 3.0f;
        static constexpr float RISE_SPEED          = 0.6f;
        static constexpr float FADE_SPEED          = 1.2f;
    };

} // namespace FriendLeaderboard
