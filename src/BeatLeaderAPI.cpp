#include "BeatLeaderAPI.hpp"
#include "Config.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/web/http.hpp"
#include "rapidjson-macros/shared/macros.hpp"
#include <thread>

using namespace FriendLeaderboard;

static modloader::ModInfo modInfo{"FriendLeaderboard", "0.1.0", 0};
static Logger& logger() {
    static auto* l = new Logger(modInfo);
    return *l;
}

const std::string BeatLeaderAPI::BASE_URL = "https://api.beatleader.com";

BeatLeaderAPI& BeatLeaderAPI::GetInstance() {
    static BeatLeaderAPI inst;
    return inst;
}

// -----------------------------------------------------------------------
// HTTP helpers (runs on background thread, calls callback on main thread)
// -----------------------------------------------------------------------
void BeatLeaderAPI::HttpGet(
    const std::string& url,
    const std::string& token,
    std::function<void(bool, std::string)> cb)
{
    std::thread([url, token, cb]() {
        WebUtils::GetAsync(
            url,
            [cb](long status, std::string body) {
                bool ok = (status >= 200 && status < 300);
                cb(ok, body);
            },
            [token](std::map<std::string,std::string>& headers) {
                if (!token.empty()) {
                    headers["Cookie"] = "login=" + token;
                }
            }
        );
    }).detach();
}

void BeatLeaderAPI::HttpPost(
    const std::string& url,
    const std::string& body,
    std::function<void(bool, std::string)> cb)
{
    std::thread([url, body, cb]() {
        WebUtils::PostAsync(
            url,
            body,
            [cb](long status, std::string resp) {
                bool ok = (status >= 200 && status < 300);
                cb(ok, resp);
            }
        );
    }).detach();
}

// -----------------------------------------------------------------------
// Auth — BeatLeader supports a username+password login endpoint
// POST /user/signin  { login, password }
// Returns a session cookie / token in the response body
// -----------------------------------------------------------------------
void BeatLeaderAPI::Authenticate(
    const std::string& login,
    const std::string& password,
    AuthCallback cb)
{
    std::string url  = BASE_URL + "/user/signin";
    // BeatLeader sign-in uses form data
    std::string body = "login=" + login + "&password=" + password;

    HttpPost(url, body, [cb, login](bool ok, std::string resp) {
        if (!ok) {
            cb({false, "", "", "", "Authentication failed: " + resp});
            return;
        }

        // Parse JSON: { "id": "...", "name": "...", "token": "..." }
        rapidjson::Document doc;
        doc.Parse(resp.c_str());

        if (doc.HasParseError() || !doc.IsObject()) {
            cb({false, "", "", "", "Invalid response from BeatLeader"});
            return;
        }

        AuthResult result;
        result.success = true;
        result.userId      = doc.HasMember("id")    ? doc["id"].GetString()    : "";
        result.username    = doc.HasMember("name")  ? doc["name"].GetString()  : login;
        result.token       = doc.HasMember("token") ? doc["token"].GetString() : "";

        cb(result);
    });
}

// -----------------------------------------------------------------------
// Friend leaderboard — GET /scores/friends/{hash}/{diff}/{mode}
// Returns { data: [ { player: {...}, score, accuracy, rank } ] }
// -----------------------------------------------------------------------
void BeatLeaderAPI::FetchFriendLeaderboard(
    const std::string& mapHash,
    const std::string& difficulty,
    const std::string& characteristic,
    const std::string& token,
    LeaderboardCallback cb)
{
    // BeatLeader leaderboard endpoint with friends filter
    std::string url = BASE_URL
        + "/v2/scores/"
        + mapHash + "/"
        + difficulty + "/"
        + characteristic
        + "?friends=true&count=50";

    HttpGet(url, token, [cb, mapHash, difficulty, characteristic](bool ok, std::string body) {
        if (!ok) {
            cb(std::nullopt);
            return;
        }

        rapidjson::Document doc;
        doc.Parse(body.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            cb(std::nullopt);
            return;
        }

        LeaderboardData data;
        data.mapHash        = mapHash;
        data.difficulty     = difficulty;
        data.characteristic = characteristic;

        if (doc.HasMember("data") && doc["data"].IsArray()) {
            for (auto& entry : doc["data"].GetArray()) {
                PlayerScore ps;

                if (entry.HasMember("player") && entry["player"].IsObject()) {
                    auto& p = entry["player"];
                    ps.playerId   = p.HasMember("id")     ? p["id"].GetString()     : "";
                    ps.playerName = p.HasMember("name")   ? p["name"].GetString()   : "Unknown";
                    ps.playerAvatar = p.HasMember("avatar") ? p["avatar"].GetString() : "";
                }

                ps.score    = entry.HasMember("baseScore")  ? entry["baseScore"].GetInt()  : 0;
                ps.accuracy = entry.HasMember("accuracy")   ? (float)entry["accuracy"].GetDouble() : 0.0f;
                ps.rank     = entry.HasMember("rank")       ? entry["rank"].GetInt()       : 0;
                ps.pp       = entry.HasMember("pp")         ? (int)entry["pp"].GetDouble() : 0;
                ps.isFriend = true;

                data.scores.push_back(ps);
            }
        }

        // Sort descending by score
        std::sort(data.scores.begin(), data.scores.end());

        cb(data);
    });
}

// -----------------------------------------------------------------------
// My score — GET /score/{playerId}/{hash}/{diff}/{mode}
// -----------------------------------------------------------------------
void BeatLeaderAPI::FetchMyScore(
    const std::string& mapHash,
    const std::string& difficulty,
    const std::string& characteristic,
    const std::string& playerId,
    const std::string& token,
    std::function<void(std::optional<PlayerScore>)> cb)
{
    std::string url = BASE_URL
        + "/score/" + playerId
        + "/" + mapHash
        + "/" + difficulty
        + "/" + characteristic;

    HttpGet(url, token, [cb, playerId](bool ok, std::string body) {
        if (!ok) { cb(std::nullopt); return; }

        rapidjson::Document doc;
        doc.Parse(body.c_str());
        if (doc.HasParseError() || !doc.IsObject()) { cb(std::nullopt); return; }

        PlayerScore ps;
        ps.playerId      = playerId;
        ps.isLocalPlayer = true;
        ps.score    = doc.HasMember("baseScore") ? doc["baseScore"].GetInt()         : 0;
        ps.accuracy = doc.HasMember("accuracy")  ? (float)doc["accuracy"].GetDouble(): 0.0f;
        ps.rank     = doc.HasMember("rank")      ? doc["rank"].GetInt()              : 0;

        if (doc.HasMember("player") && doc["player"].IsObject()) {
            auto& p = doc["player"];
            ps.playerName = p.HasMember("name") ? p["name"].GetString() : "";
        }

        cb(ps);
    });
}

// -----------------------------------------------------------------------
// Friends list — GET /player/{id}/friends
// -----------------------------------------------------------------------
void BeatLeaderAPI::FetchFriends(
    const std::string& playerId,
    const std::string& token,
    FriendsCallback cb)
{
    std::string url = BASE_URL + "/player/" + playerId + "/friends";

    HttpGet(url, token, [cb](bool ok, std::string body) {
        if (!ok) { cb({}); return; }

        rapidjson::Document doc;
        doc.Parse(body.c_str());
        if (doc.HasParseError() || !doc.IsArray()) { cb({}); return; }

        std::vector<PlayerScore> friends;
        for (auto& f : doc.GetArray()) {
            PlayerScore ps;
            ps.playerId   = f.HasMember("id")   ? f["id"].GetString()   : "";
            ps.playerName = f.HasMember("name")  ? f["name"].GetString()  : "Unknown";
            ps.isFriend   = true;
            friends.push_back(ps);
        }
        cb(friends);
    });
}
