#include "FriendLeaderboardUI.hpp"
#include "SnipeNotification.hpp"
#include "BeatLeaderAPI.hpp"
#include "Config.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Color32.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/CanvasScaler.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"

using namespace FriendLeaderboard;
using namespace UnityEngine;

static modloader::ModInfo modInfo{"FriendLeaderboard", "0.1.0", 0};
static Logger& logger() {
    static auto* l = new Logger(modInfo);
    return *l;
}

// -----------------------------------------------------------------------
FriendLeaderboardUI& FriendLeaderboardUI::GetInstance() {
    static FriendLeaderboardUI inst;
    return inst;
}

// -----------------------------------------------------------------------
// Colours
// -----------------------------------------------------------------------
static Color HexColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return Color(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}
static const Color COL_BG_PANEL  = HexColor(10,  10,  20,  210);
static const Color COL_BG_LOCAL  = HexColor(80,  200, 120,  80);
static const Color COL_BG_FRIEND = HexColor(30,  30,  60,  140);
static const Color COL_BG_SNIPED = HexColor(200, 60,  60,   80);
static const Color COL_TEXT_HEAD = HexColor(255, 215,  0,  255); // gold
static const Color COL_TEXT_MAIN = HexColor(230, 230, 255,  255);
static const Color COL_TEXT_ACC  = HexColor(150, 200, 255,  255);

// -----------------------------------------------------------------------
void FriendLeaderboardUI::CreateUI() {
    if (m_root) return;

    // World-space canvas anchored to the right of the HMD view
    auto* goCanvas = GameObject::New_ctor(il2cpp_utils::newcsstr("FriendLBCanvas"));
    Object::DontDestroyOnLoad(goCanvas);

    auto* canvas = goCanvas->AddComponent<Canvas*>();
    canvas->set_renderMode(RenderMode::WorldSpace);
    canvas->set_sortingOrder(100);

    auto* scaler = goCanvas->AddComponent<CanvasScaler*>();
    scaler->set_dynamicPixelsPerUnit(10.0f);

    // Position: right side of view, slightly forward
    auto* rt = goCanvas->GetComponent<RectTransform*>();
    rt->set_sizeDelta({80.0f, 180.0f});
    // Place it in world space at roughly right side of player view
    goCanvas->get_transform()->set_position({1.6f, 1.2f, 2.5f});
    goCanvas->get_transform()->set_eulerAngles({0.0f, -20.0f, 0.0f});
    goCanvas->get_transform()->set_localScale({0.01f, 0.01f, 0.01f});

    m_root = goCanvas;

    // Panel background
    auto* panelGO = GameObject::New_ctor(il2cpp_utils::newcsstr("FriendLBPanel"));
    panelGO->get_transform()->SetParent(goCanvas->get_transform(), false);

    auto* panelRT = panelGO->AddComponent<RectTransform*>();
    panelRT->set_anchorMin({0.0f, 0.0f});
    panelRT->set_anchorMax({1.0f, 1.0f});
    panelRT->set_offsetMin({0.0f, 0.0f});
    panelRT->set_offsetMax({0.0f, 0.0f});

    auto* panelImg = panelGO->AddComponent<Image*>();
    panelImg->set_color(COL_BG_PANEL);

    // Header
    auto* headerGO = QuestUI::BeatSaberUI::CreateText(
        panelRT, "🏆 Friend Leaderboard", {0.0f, 82.0f});
    m_header = headerGO->GetComponent<TMPro::TextMeshProUGUI*>();
    m_header->set_fontSize(5.5f);
    m_header->set_alignment(TMPro::TextAlignmentOptions::Center);
    m_header->set_color(COL_TEXT_HEAD);

    // Pre-create up to 10 rows
    m_rows.clear();
    m_rows.resize(10);
    for (int i = 0; i < 10; i++) {
        float y = 68.0f - i * 16.0f;

        auto& row = m_rows[i];

        // Row background
        auto* rowBGGO = GameObject::New_ctor(il2cpp_utils::newcsstr("Row" + std::to_string(i)));
        rowBGGO->get_transform()->SetParent(panelRT, false);
        auto* rowRT = rowBGGO->AddComponent<RectTransform*>();
        rowRT->set_anchorMin({0.02f, 0.0f});
        rowRT->set_anchorMax({0.98f, 0.0f});
        rowRT->set_anchoredPosition({0.0f, y});
        rowRT->set_sizeDelta({0.0f, 14.0f});
        row.bg = rowBGGO->AddComponent<Image*>();
        row.bg->set_color(COL_BG_FRIEND);
        row.go = rowBGGO;

        // Rank
        row.rankText = QuestUI::BeatSaberUI::CreateText(rowRT, "#-", {-34.0f, 0.0f})->GetComponent<TMPro::TextMeshProUGUI*>();
        row.rankText->set_fontSize(4.5f);
        row.rankText->set_color(COL_TEXT_ACC);

        // Name
        row.nameText = QuestUI::BeatSaberUI::CreateText(rowRT, "------", {-10.0f, 0.0f})->GetComponent<TMPro::TextMeshProUGUI*>();
        row.nameText->set_fontSize(4.5f);
        row.nameText->set_color(COL_TEXT_MAIN);
        row.nameText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);

        // Score
        row.scoreText = QuestUI::BeatSaberUI::CreateText(rowRT, "0", {22.0f, 3.0f})->GetComponent<TMPro::TextMeshProUGUI*>();
        row.scoreText->set_fontSize(4.0f);
        row.scoreText->set_color(COL_TEXT_MAIN);
        row.scoreText->set_alignment(TMPro::TextAlignmentOptions::Right);

        // Accuracy
        row.accText = QuestUI::BeatSaberUI::CreateText(rowRT, "0.00%", {22.0f, -3.5f})->GetComponent<TMPro::TextMeshProUGUI*>();
        row.accText->set_fontSize(3.5f);
        row.accText->set_color(COL_TEXT_ACC);
        row.accText->set_alignment(TMPro::TextAlignmentOptions::Right);

        rowBGGO->SetActive(false);
    }

    m_visible = true;
    logger().info("FriendLeaderboardUI created");
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::DestroyUI() {
    if (m_root) {
        Object::Destroy(m_root);
        m_root = nullptr;
    }
    m_rows.clear();
    m_visible    = false;
    m_dataLoaded = false;
    m_snipedIds.clear();
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::OnMapStart(
    const std::string& mapHash,
    const std::string& difficulty,
    const std::string& characteristic)
{
    m_currentHash = mapHash;
    m_currentDiff = difficulty;
    m_currentChar = characteristic;
    m_dataLoaded  = false;
    m_snipedIds.clear();
    m_localScore  = PlayerScore{};
    m_localScore.isLocalPlayer = true;
    m_localScore.playerName    = getModConfig().BeatLeaderUsername.GetValue();
    m_localScore.playerId      = getModConfig().BeatLeaderUserId.GetValue();

    if (!getModConfig().ShowFriendLeaderboard.GetValue()) return;

    CreateUI();

    auto token    = getModConfig().BeatLeaderToken.GetValue();
    auto playerId = getModConfig().BeatLeaderUserId.GetValue();

    if (token.empty() || playerId.empty()) {
        logger().warning("Not logged in to BeatLeader — skipping leaderboard fetch");
        return;
    }

    // Fetch friend leaderboard
    BeatLeaderAPI::GetInstance().FetchFriendLeaderboard(
        mapHash, difficulty, characteristic, token,
        [this, token, playerId](std::optional<LeaderboardData> data) {
            if (!data) return;
            m_data = *data;

            // Also fetch my existing score on this map
            BeatLeaderAPI::GetInstance().FetchMyScore(
                m_currentHash, m_currentDiff, m_currentChar, playerId, token,
                [this](std::optional<PlayerScore> myScore) {
                    if (myScore) {
                        m_localScore.score    = myScore->score;
                        m_localScore.accuracy = myScore->accuracy;
                        m_localScore.rank     = myScore->rank;
                    }
                    m_dataLoaded = true;
                    RefreshDisplay();
                }
            );
        }
    );
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::OnScoreUpdated(int newScore, float newAccuracy) {
    if (!m_visible || !m_dataLoaded) return;

    int oldScore = m_localScore.score;
    m_localScore.score    = newScore;
    m_localScore.accuracy = newAccuracy;

    if (newScore > oldScore) {
        CheckForSnipes(newScore);
        RefreshDisplay();
    }
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::CheckForSnipes(int newScore) {
    if (!getModConfig().SnipeNotifications.GetValue()) return;

    for (auto& ps : m_data.scores) {
        if (ps.isLocalPlayer) continue;
        if (newScore > ps.score) {
            bool alreadySniped = std::find(m_snipedIds.begin(), m_snipedIds.end(), ps.playerId)
                                 != m_snipedIds.end();
            if (!alreadySniped) {
                m_snipedIds.push_back(ps.playerId);
                SnipeNotification::GetInstance().ShowSnipe(ps.playerName);
                logger().info("Sniped: %s", ps.playerName.c_str());
            }
        }
    }
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::RefreshDisplay() {
    if (!m_root || m_rows.empty()) return;

    // Merge local score into list
    std::vector<PlayerScore> display;
    display.push_back(m_localScore);
    for (auto& ps : m_data.scores) {
        display.push_back(ps);
    }

    // Sort descending
    std::sort(display.begin(), display.end());

    // Assign display ranks
    for (int i = 0; i < (int)display.size(); i++) {
        display[i].rank = i + 1;
    }

    // Update rows (limit to 10 visible)
    int rowCount = std::min((int)display.size(), 10);
    for (int i = 0; i < 10; i++) {
        auto& row = m_rows[i];
        if (i >= rowCount) {
            row.go->SetActive(false);
            continue;
        }
        row.go->SetActive(true);
        auto& ps = display[i];
        row.playerId = ps.playerId;

        row.rankText->set_text(il2cpp_utils::newcsstr("#" + std::to_string(ps.rank)));
        row.nameText->set_text(il2cpp_utils::newcsstr(ps.playerName));
        row.scoreText->set_text(il2cpp_utils::newcsstr(std::to_string(ps.score)));

        char accBuf[16];
        snprintf(accBuf, sizeof(accBuf), "%.2f%%", ps.accuracy * 100.0f);
        row.accText->set_text(il2cpp_utils::newcsstr(std::string(accBuf)));

        // Colour coding
        if (ps.isLocalPlayer) {
            row.bg->set_color(COL_BG_LOCAL);
            row.nameText->set_color(HexColor(180, 255, 180, 255));
        } else {
            bool sniped = std::find(m_snipedIds.begin(), m_snipedIds.end(), ps.playerId)
                          != m_snipedIds.end();
            row.bg->set_color(sniped ? COL_BG_SNIPED : COL_BG_FRIEND);
            row.nameText->set_color(COL_TEXT_MAIN);
        }
    }
}

// -----------------------------------------------------------------------
void FriendLeaderboardUI::OnMapEnd() {
    DestroyUI();
}
