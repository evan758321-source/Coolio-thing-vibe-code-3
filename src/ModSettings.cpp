#include "ModSettings.hpp"
#include "Config.hpp"
#include "BeatLeaderAPI.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Settings/StringSetting.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Color.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

using namespace FriendLeaderboard;
using namespace UnityEngine;

static modloader::ModInfo modInfo{"FriendLeaderboard", "0.1.0", 0};
static Logger& logger() {
    static auto* l = new Logger(modInfo);
    return *l;
}

// -----------------------------------------------------------------------
// Settings UI built with QuestUI / BSML
// -----------------------------------------------------------------------

static TMPro::TextMeshProUGUI* g_statusText    = nullptr;
static BSML::StringSetting*    g_loginField    = nullptr;
static BSML::StringSetting*    g_passwordField = nullptr;

static void OnLoginPressed() {
    if (!g_loginField || !g_passwordField) return;

    auto login    = il2cpp_utils::from_il2cppString(g_loginField->get_text());
    auto password = il2cpp_utils::from_il2cppString(g_passwordField->get_text());

    if (login.empty() || password.empty()) {
        if (g_statusText)
            g_statusText->set_text(il2cpp_utils::newcsstr("⚠ Enter username and password"));
        return;
    }

    if (g_statusText)
        g_statusText->set_text(il2cpp_utils::newcsstr("Logging in..."));

    BeatLeaderAPI::GetInstance().Authenticate(login, password, [](AuthResult result) {
        if (result.success) {
            getModConfig().BeatLeaderToken.SetValue(result.token);
            getModConfig().BeatLeaderUsername.SetValue(result.username);
            getModConfig().BeatLeaderUserId.SetValue(result.userId);

            if (g_statusText)
                g_statusText->set_text(il2cpp_utils::newcsstr(
                    "✓ Logged in as " + result.username));
            logger().info("BeatLeader login OK: %s (%s)", result.username.c_str(), result.userId.c_str());
        } else {
            if (g_statusText)
                g_statusText->set_text(il2cpp_utils::newcsstr(
                    "✗ " + result.errorMessage));
            logger().warning("BeatLeader login failed: %s", result.errorMessage.c_str());
        }
    });
}

static void OnLogoutPressed() {
    getModConfig().BeatLeaderToken.SetValue("");
    getModConfig().BeatLeaderUsername.SetValue("");
    getModConfig().BeatLeaderUserId.SetValue("");

    if (g_statusText)
        g_statusText->set_text(il2cpp_utils::newcsstr("Logged out."));
}

// -----------------------------------------------------------------------
// Build the settings UI — called by QuestUI via the registration callback
// -----------------------------------------------------------------------
static void BuildSettingsUI(GameObject* parent, HMUI::ViewController*) {
    using namespace QuestUI::BeatSaberUI;

    auto* container = CreateScrollableSettingsContainer(parent->get_transform());
    auto* rt = container->GetComponent<UnityEngine::RectTransform*>();

    // ---- Section: BeatLeader Account ----
    CreateText(rt, "── BeatLeader Account ──", {0.0f, 0.0f})->set_color(
        Color(1.0f, 0.84f, 0.0f, 1.0f));

    // Current status
    std::string currentUser = getModConfig().BeatLeaderUsername.GetValue();
    std::string statusMsg = currentUser.empty()
        ? "Not logged in"
        : "Logged in as " + currentUser;

    auto* statusGO = CreateText(rt, statusMsg, {0.0f, 0.0f});
    g_statusText = statusGO->GetComponent<TMPro::TextMeshProUGUI*>();

    // Username / password fields
    g_loginField    = CreateStringSetting(rt, "BeatLeader Username", "",
        [](std::string) {});
    g_passwordField = CreateStringSetting(rt, "Password", "",
        [](std::string) {});

    // Login button
    CreateUIButton(rt, "Login", OnLoginPressed);

    // Logout button
    CreateUIButton(rt, "Logout", OnLogoutPressed);

    // ---- Section: Display Options ----
    CreateText(rt, "── Display Options ──", {0.0f, 0.0f})->set_color(
        Color(1.0f, 0.84f, 0.0f, 1.0f));

    CreateToggle(rt, "Show Friend Leaderboard",
        getModConfig().ShowFriendLeaderboard.GetValue(),
        [](bool val) {
            getModConfig().ShowFriendLeaderboard.SetValue(val);
        });

    CreateToggle(rt, "Snipe Notifications",
        getModConfig().SnipeNotifications.GetValue(),
        [](bool val) {
            getModConfig().SnipeNotifications.SetValue(val);
        });

    CreateIncrementSetting(rt, "UI Scale", 2, 0.05f,
        getModConfig().UIScale.GetValue(), 0.5f, 2.0f,
        [](float val) {
            getModConfig().UIScale.SetValue(val);
        });
}

// -----------------------------------------------------------------------
void ModSettings::Register() {
    QuestUI::Register::RegisterModSettingsViewController(
        modInfo,
        BuildSettingsUI
    );
    logger().info("ModSettings registered");
}
