#include "SnipeNotification.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/Coroutine.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"

using namespace FriendLeaderboard;
using namespace UnityEngine;

static modloader::ModInfo modInfo{"FriendLeaderboard", "0.1.0", 0};
static Logger& logger() {
    static auto* l = new Logger(modInfo);
    return *l;
}

SnipeNotification& SnipeNotification::GetInstance() {
    static SnipeNotification inst;
    return inst;
}

// -----------------------------------------------------------------------
// Coroutine: float the popup up and fade it out
// -----------------------------------------------------------------------
custom_types::Helpers::Coroutine AnimateSnipePopup(
    GameObject* go,
    TMPro::TextMeshProUGUI* text,
    float duration,
    float riseSpeed)
{
    float elapsed = 0.0f;
    auto* rt = go->GetComponent<RectTransform*>();

    while (elapsed < duration) {
        elapsed += Time::get_deltaTime();
        float t = elapsed / duration;

        // Rise
        auto pos = rt->get_anchoredPosition();
        pos.y += riseSpeed * Time::get_deltaTime();
        rt->set_anchoredPosition(pos);

        // Fade alpha
        float alpha = 1.0f;
        if (t > 0.6f) {
            alpha = 1.0f - ((t - 0.6f) / 0.4f);
        }

        // Scale pulse at start
        float scale = 1.0f;
        if (t < 0.15f) {
            scale = 1.0f + 0.3f * std::sin(t / 0.15f * 3.14159f);
        }
        go->get_transform()->set_localScale({scale, scale, scale});

        auto col = text->get_color();
        col.a = alpha;
        text->set_color(col);

        co_yield nullptr;
    }

    Object::Destroy(go);
    co_return;
}

// -----------------------------------------------------------------------
void SnipeNotification::ShowSnipe(const std::string& snipedUsername) {
    // Create a world-space canvas popup in front of the player
    auto* go = GameObject::New_ctor(il2cpp_utils::newcsstr("SnipePopup"));
    Object::DontDestroyOnLoad(go);

    auto* canvas = go->AddComponent<Canvas*>();
    canvas->set_renderMode(RenderMode::WorldSpace);
    canvas->set_sortingOrder(200);

    auto* rt = go->GetComponent<RectTransform*>();
    rt->set_sizeDelta({160.0f, 40.0f});

    // Position slightly in front of and above the player's view, left-center
    go->get_transform()->set_position({0.0f, 1.5f, 2.8f});
    go->get_transform()->set_eulerAngles({0.0f, 0.0f, 0.0f});
    go->get_transform()->set_localScale({0.01f, 0.01f, 0.01f});

    // "Sniped" label
    std::string msg = "Sniped  " + snipedUsername;

    auto* textGO = QuestUI::BeatSaberUI::CreateText(rt, msg, {0.0f, 0.0f});
    auto* tmp = textGO->GetComponent<TMPro::TextMeshProUGUI*>();
    tmp->set_fontSize(14.0f);
    tmp->set_alignment(TMPro::TextAlignmentOptions::Center);
    tmp->set_color(Color(1.0f, 0.2f, 0.2f, 1.0f)); // bright red

    // Enable outline / bold for HSV-visualizer feel
    tmp->set_fontStyle(TMPro::FontStyles::Bold);

    // Kick off coroutine on the GO itself
    // We use a static helper component with a coroutine runner
    // (beatsaber-hook coroutine helper)
    BSML::Helpers::StartCoroutine(
        AnimateSnipePopup(go, tmp, DISPLAY_DURATION, RISE_SPEED)
    );

    logger().info("Snipe popup shown: %s", msg.c_str());
}
