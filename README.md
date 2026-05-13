# FriendLeaderboard

A Beat Saber Quest standalone mod that shows your **BeatLeader friends' scores** in a live leaderboard on the **right side of your screen** while you play.

When you beat a friend's score, the leaderboard updates in real-time and you get a **"Sniped [username]"** popup in HSV style.

---

## Features

| Feature | Description |
|---|---|
| 📊 **Live Friend Leaderboard** | Right-side HUD showing your score vs friends', updating every beat |
| 🎯 **Snipe Detection** | When you pass a friend's score, their row turns red |
| 💥 **Snipe Popup** | Animated "Sniped [username]" notification (hit-score-visualizer style) |
| 🔐 **BeatLeader Login** | Log in with your BL credentials in Mod Settings |
| ⚙️ **Settings Panel** | Toggle leaderboard, toggle snipe notifications, adjust UI scale |

---

## Building

### Prerequisites

- [QPM (Quest Package Manager)](https://github.com/RedBrumbler/QPM)
- Android NDK r26d
- CMake + Ninja
- A GitHub account (for Actions CI)

### Local Build

```bash
# Install dependencies
qpm restore

# Configure
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_TOOLCHAIN_FILE=$NDK_PATH/build/cmake/android.toolchain.cmake \
  -DEXTERN_INC_DIR=$(pwd)/extern/includes \
  -DEXTERN_LIB_DIR=$(pwd)/extern/libs \
  -G Ninja

# Build
cmake --build build --config Release --parallel

# Package
mkdir -p qmod_package/libs/arm64-v8a
cp build/libFriendLeaderboard.so qmod_package/libs/arm64-v8a/
cp mod.json qmod_package/
cd qmod_package && zip -r ../FriendLeaderboard.qmod .
```

### GitHub Actions (CI)

Push to `main` → `.qmod` is built automatically and uploaded as an artifact.
Push a tag (`v0.1.0`) → a GitHub Release is created with the `.qmod` attached.

---

## Installation

1. Download `FriendLeaderboard.qmod` from Releases
2. Side-load with **BMBF** or **QuestPatcher**
3. Launch Beat Saber
4. Open **Mod Settings → Friend Leaderboard**
5. Enter your **BeatLeader username & password** and press **Login**
6. Start any custom song — the leaderboard will appear on the right!

---

## How It Works

### Leaderboard HUD
- Positioned ~1.6m to the right of the player, angled inward
- Shows up to 10 entries (your score + friends' stored BL scores)
- Your row is **green**, sniped friends turn **red**
- Live score updates every frame via `ScoreController::LateUpdate`

### BeatLeader API
- `POST /user/signin` — authenticate and store session token
- `GET /v2/scores/{hash}/{diff}/{mode}?friends=true` — fetch friend scores
- `GET /score/{playerId}/{hash}/{diff}/{mode}` — fetch your existing PB

### Snipe System
- Each frame, your live score is compared against every friend entry
- First time you exceed a friend's score → "Sniped [username]" popup
- Popup rises and fades out over 3 seconds (coroutine animated)
- Each friend can only be sniped once per session

---

## Configuration

In **Mod Settings → Friend Leaderboard**:

| Setting | Default | Description |
|---|---|---|
| BeatLeader Username | — | Your BL login (stored in config) |
| Password | — | Used only to authenticate; token is stored, not password |
| Show Friend Leaderboard | ✅ | Toggle the HUD on/off |
| Snipe Notifications | ✅ | Toggle the popup on/off |
| UI Scale | 1.0 | Scale the HUD (0.5–2.0) |

---

## Dependencies

- [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook) ≥ 3.12.1
- [questui](https://github.com/RedBrumbler/questui) ≥ 2.0.0
- [bs-utils](https://github.com/darknight1050/bs-utils) ≥ 1.8.0
- [config-utils](https://github.com/RedBrumbler/config-utils)
- [custom-types](https://github.com/sc2ad/Il2CppQuestTypePatching)

All fetched automatically by `qpm restore`.

---

## Notes

- This mod uses the **BeatLeader public API** (`api.beatleader.com`)
- Your password is sent **only** to BeatLeader's login endpoint and is **not stored** — only the session token is saved
- Works on Beat Saber **1.28.0** (MBF / standalone Quest)
