#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "Config.hpp"
#include "BeatLeaderAPI.hpp"
#include "FriendLeaderboardUI.hpp"
#include "SnipeNotification.hpp"
#include "ModSettings.hpp"

// Game classes we hook
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PlayerTransforms.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"

using namespace GlobalNamespace;
using namespace FriendLeaderboard;

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

Logger& getLogger() {
    static auto* logger = new Logger(modInfo);
    return *logger;
}

// -----------------------------------------------------------------------
// HOOK: ScoreController::Start — grab the map info when gameplay begins
// -----------------------------------------------------------------------
static std::string s_currentHash;
static std::string s_currentDiff;
static std::string s_currentChar;

MAKE_HOOK_MATCH(
    ScoreController_Start,
    &ScoreController::Start,
    void,
    ScoreController* self)
{
    ScoreController_Start(self);

    // Extract map hash from the difficultyBeatmap
    auto* diffBeatmap = self->____gameplayModifiers
        ? self->_difficultyBeatmap : nullptr;

    if (diffBeatmap) {
        auto* level = diffBeatmap->get_level();
        if (level) {
            auto levelId = il2cpp_utils::from_il2cppString(level->get_levelID());
            // Beat Saber levelID format: "custom_level_<HASH>"
            const std::string prefix = "custom_level_";
            if (levelId.rfind(prefix, 0) == 0) {
                s_currentHash = levelId.substr(prefix.size());
            } else {
                s_currentHash = levelId;
            }
        }

        auto diff = diffBeatmap->get_difficulty();
        switch (diff) {
            case BeatmapDifficulty::Easy:       s_currentDiff = "Easy";       break;
            case BeatmapDifficulty::Normal:     s_currentDiff = "Normal";     break;
            case BeatmapDifficulty::Hard:       s_currentDiff = "Hard";       break;
            case BeatmapDifficulty::Expert:     s_currentDiff = "Expert";     break;
            case BeatmapDifficulty::ExpertPlus: s_currentDiff = "ExpertPlus"; break;
            default:                            s_currentDiff = "Expert";     break;
        }

        auto* charSO = diffBeatmap->get_parentDifficultyBeatmapSet()
                                   ->get_beatmapCharacteristic();
        s_currentChar = charSO
            ? il2cpp_utils::from_il2cppString(charSO->get_serializedName())
            : "Standard";
    }

    getLogger().info("Map started: %s %s %s", s_currentHash.c_str(), s_currentDiff.c_str(), s_currentChar.c_str());

    FriendLeaderboardUI::GetInstance().OnMapStart(s_currentHash, s_currentDiff, s_currentChar);
}

// -----------------------------------------------------------------------
// HOOK: ScoreController::HandleNoteWasCut / LateUpdate — live score feed
// -----------------------------------------------------------------------
MAKE_HOOK_MATCH(
    ScoreController_LateUpdate,
    &ScoreController::LateUpdate,
    void,
    ScoreController* self)
{
    ScoreController_LateUpdate(self);

    int   score = self->_multipliedScore;
    // Compute a rough real-time accuracy from max possible vs actual
    int   maxScore = self->_immediateMaxPossibleMultipliedScore;
    float acc = (maxScore > 0) ? ((float)score / (float)maxScore) : 0.0f;

    FriendLeaderboardUI::GetInstance().OnScoreUpdated(score, acc);
}

// -----------------------------------------------------------------------
// HOOK: ResultsViewController::Init — map ended
// -----------------------------------------------------------------------
MAKE_HOOK_MATCH(
    ResultsViewController_Init,
    &ResultsViewController::Init,
    void,
    ResultsViewController* self,
    LevelCompletionResults* results,
    IDifficultyBeatmap* difficultyBeatmap,
    bool practice,
    bool newHighScore)
{
    ResultsViewController_Init(self, results, difficultyBeatmap, practice, newHighScore);
    FriendLeaderboardUI::GetInstance().OnMapEnd();
}

// -----------------------------------------------------------------------
// HOOK: PauseMenuManager::ShowMenu — hide UI on pause (optional)
// -----------------------------------------------------------------------
MAKE_HOOK_MATCH(
    PauseMenuManager_ShowMenu,
    &PauseMenuManager::ShowMenu,
    void,
    PauseMenuManager* self)
{
    PauseMenuManager_ShowMenu(self);
    // Optionally hide/show the leaderboard panel when paused
    // FriendLeaderboardUI::GetInstance().SetVisible(false);
}

// -----------------------------------------------------------------------
// Mod load / unload
// -----------------------------------------------------------------------
extern "C" void setup(ModInfo& info) {
    info.id      = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getModConfig().Init(modInfo);
    getLogger().info("FriendLeaderboard v%s setup complete", VERSION);
}

extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing hooks...");

    INSTALL_HOOK(getLogger(), ScoreController_Start);
    INSTALL_HOOK(getLogger(), ScoreController_LateUpdate);
    INSTALL_HOOK(getLogger(), ResultsViewController_Init);
    INSTALL_HOOK(getLogger(), PauseMenuManager_ShowMenu);

    ModSettings::Register();

    getLogger().info("FriendLeaderboard loaded!");
}
