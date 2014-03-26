// AGS Steam stub plugin
// Steam-free replacement for AGSteam by monkey (see http://www.adventuregamestudio.co.uk/forums/index.php?topic=44712.0)
// No copyright. This is public domain.

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Do any Windows specific constructor/destructor work here.
// Do NOT use for plugin initialization code.
// DllMain will never be called on non-Windows platforms.
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReason, LPVOID lpReserved) {
	return TRUE;
}

#endif

#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <utility>

#define THIS_IS_THE_PLUGIN
#include "agsplugin.h"

#ifdef DEBUG
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

const char *AGSTEAM_PLUGIN_NAME = "AGSteamStub";

static IAGSEngine *AGSteam_engine;
static std::string AGSteam_language;
static std::string AGSteam_username;
static bool AGSteam_initialized;
static std::string AGSteam_curlbname;
static std::vector<std::string> AGSteam_lbnames;
static std::map<std::string, int> AGSteam_lbscores;
static std::map<std::string, bool> AGSteam_achievements;
static std::map<std::string, int> AGSteam_intstats;
static std::map<std::string, float> AGSteam_floatstats;
static std::map<std::string, std::pair<float, float> > AGSteam_ratestats;

static int AGSteam_New(IAGSEngine *engine) {
	if (!AGSteam_initialized) {
		AGSteam_engine = engine;
		AGSteam_language = "English";
		AGSteam_username = "User";
		AGSteam_curlbname = "";
		AGSteam_lbnames.clear();
		AGSteam_lbscores.clear();
		AGSteam_achievements.clear();
		AGSteam_intstats.clear();
		AGSteam_floatstats.clear();
		AGSteam_ratestats.clear();
		AGSteam_initialized = true;
	}
	debug_printf("AGSteam: New(%p): %d\n", engine, AGSteam_initialized);
	return AGSteam_initialized ? 1 : 0;
}

static void AGSteam_Delete() {
	debug_printf("AGSteam: Delete()\n");
	AGSteam_engine = 0;
	AGSteam_initialized = false;
}

static int AGSteam_IsAchievementAchieved(const char *steamAchievementID) {
	int ret = AGSteam_achievements[steamAchievementID] ? 1 : 0;
	debug_printf("AGSteam: IsAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetAchievementAchieved(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = true;
	int ret = 1;
	debug_printf("AGSteam: SetAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_ResetAchievement(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = false;
	int ret = 1;
	debug_printf("AGSteam: ResetAchievement(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetIntStat(const char *steamStatName, int value) {
	AGSteam_intstats[steamStatName] = value;
	int ret = 1;
	debug_printf("AGSteam: SetIntStat(%s, %d): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_SetFloatStat(const char *steamStatName, float value) {
	AGSteam_floatstats[steamStatName] = value;
	int ret = 1;
	debug_printf("AGSteam: SetFloatStat(%s, %0.2f): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_UpdateAverageRateStat(const char *steamStatName, float numerator, float denominator) {
	AGSteam_ratestats[steamStatName] = std::make_pair(numerator, denominator);
	int ret = 1;
	debug_printf("AGSteam: UpdateAverageRateStat(%s, %0.2f/%0.2f): %d\n", steamStatName, numerator, denominator, ret);
	return ret;
}

static int AGSteam_GetIntStat(const char *steamStatName) {
	int ret = AGSteam_intstats[steamStatName];
	debug_printf("AGSteam: GetIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetFloatStat(const char *steamStatName) {
	float ret = AGSteam_floatstats[steamStatName];
	debug_printf("AGSteam: GetFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetAverageRateStat(const char *steamStatName) {
	float ret = AGSteam_ratestats[steamStatName].first / AGSteam_ratestats[steamStatName].second;
	debug_printf("AGSteam: GetAverageRateStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_ResetStats() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	debug_printf("AGSteam: ResetStats()\n");
}

static void AGSteam_ResetStatsAndAchievements() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	AGSteam_achievements.clear();
	debug_printf("AGSteam: ResetStatsAndAchievements()\n");
}

static int AGSteam_GetGlobalIntStat(const char *steamStatName) {
	int ret = 0;
	debug_printf("AGSteam: GetGlobalIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetGlobalFloatStat(const char *steamStatName) {
	float ret = 0.0f;
	debug_printf("AGSteam: GetGlobalFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_FindLeaderboard(const char *leaderboardName) {
	AGSteam_curlbname = leaderboardName;
	if (AGSteam_lbscores.count(AGSteam_curlbname) == 0) {
		AGSteam_lbnames.push_back(AGSteam_curlbname);
		AGSteam_lbscores[AGSteam_curlbname] = 0;
	}
	debug_printf("AGSteam: FindLeaderboard(%s)\n", AGSteam_curlbname.c_str());
}

static int AGSteam_UploadScore(int score) {
	AGSteam_lbscores[AGSteam_curlbname] = score;
	int ret = 1;
	debug_printf("AGSteam: UploadScore(%d): %d\n", score, ret);
	return ret;
}

static int AGSteam_DownloadScores(int type) {
	int ret = 1;
	debug_printf("AGSteam: DownloadScores(%d): %d\n", type, ret);
	return ret;
}

static const char *AGSteam_GetCurrentGameLanguage() {
	debug_printf("AGSteam: GetCurrentGameLanguage(): %s\n", AGSteam_language.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_language.c_str());
}

static const char *AGSteam_GetUserName() {
	debug_printf("AGSteam: GetUserName(): %s\n", AGSteam_username.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_username.c_str());
}

static int AGSteam_Initialized(void *object) {
	int ret = AGSteam_initialized;
	debug_printf("AGSteam: Initialized(%p): %d\n", object, ret);
	return ret;
}

static const char *AGSteam_CurrentLeaderboardName(void *object) {
	debug_printf("AGSteam: CurrentLeaderboardName(%p): %s\n", object, AGSteam_curlbname.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_curlbname.c_str());
}

static const char *AGSteam_LeaderboardNames(void *object, int index) {
	std::string ret;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbnames[index];
	}
	debug_printf("AGSteam: LeaderboardNames(%p, %d): %s\n", object, index, ret.c_str());
	return AGSteam_engine->CreateScriptString(ret.c_str());
}

static int AGSteam_LeaderboardScores(void *object, int index) {
	int ret = -1;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbscores[AGSteam_lbnames[index]];
	}
	debug_printf("AGSteam: LeaderboardScores(%p, %d): %d\n", object, index, ret);
	return ret;
}

static int AGSteam_LeaderboardCount(void *object) {
	debug_printf("AGSteam: LeaderboardCount(%p): %lu\n", object, AGSteam_lbnames.size());
	return AGSteam_lbnames.size();
}

const char *AGS_GetPluginName() {
	debug_printf("AGSteam: AGS_GetPluginName()\n");
	return AGSTEAM_PLUGIN_NAME;
}

void AGS_EngineStartup(IAGSEngine *lpGame) {
	debug_printf("AGSteam: AGS_EngineStartup(%p)\n", lpGame);
	AGSteam_New(lpGame);
	lpGame->RegisterScriptFunction("AGSteam::IsAchievementAchieved^1", reinterpret_cast<void *>(AGSteam_IsAchievementAchieved));
	lpGame->RegisterScriptFunction("AGSteam::SetAchievementAchieved^1", reinterpret_cast<void *>(AGSteam_SetAchievementAchieved));
	lpGame->RegisterScriptFunction("AGSteam::ResetAchievement^1", reinterpret_cast<void *>(AGSteam_ResetAchievement));
	lpGame->RegisterScriptFunction("AGSteam::GetIntStat^1", reinterpret_cast<void *>(AGSteam_GetIntStat));
	lpGame->RegisterScriptFunction("AGSteam::GetFloatStat^1", reinterpret_cast<void *>(AGSteam_GetFloatStat));
	lpGame->RegisterScriptFunction("AGSteam::GetAverageRateStat^1", reinterpret_cast<void *>(AGSteam_GetAverageRateStat));
	lpGame->RegisterScriptFunction("AGSteam::SetIntStat^2", reinterpret_cast<void *>(AGSteam_SetIntStat));
	lpGame->RegisterScriptFunction("AGSteam::SetFloatStat^2", reinterpret_cast<void *>(AGSteam_SetFloatStat));
	lpGame->RegisterScriptFunction("AGSteam::UpdateAverageRateStat^3", reinterpret_cast<void *>(AGSteam_UpdateAverageRateStat));
	lpGame->RegisterScriptFunction("AGSteam::ResetStats^0", reinterpret_cast<void *>(AGSteam_ResetStats));
	lpGame->RegisterScriptFunction("AGSteam::ResetStatsAndAchievements^0", reinterpret_cast<void *>(AGSteam_ResetStatsAndAchievements));
	lpGame->RegisterScriptFunction("AGSteam::get_Initialized", reinterpret_cast<void *>(AGSteam_Initialized));
	lpGame->RegisterScriptFunction("AGSteam::GetGlobalIntStat^1", reinterpret_cast<void *>(AGSteam_GetGlobalIntStat));
	lpGame->RegisterScriptFunction("AGSteam::GetGlobalFloatStat^1", reinterpret_cast<void *>(AGSteam_GetGlobalFloatStat));
	lpGame->RegisterScriptFunction("AGSteam::get_CurrentLeaderboardName", reinterpret_cast<void *>(AGSteam_CurrentLeaderboardName));
	lpGame->RegisterScriptFunction("AGSteam::FindLeaderboard^1", reinterpret_cast<void *>(AGSteam_FindLeaderboard));
	lpGame->RegisterScriptFunction("AGSteam::UploadScore^1", reinterpret_cast<void *>(AGSteam_UploadScore));
	lpGame->RegisterScriptFunction("AGSteam::DownloadScores^1", reinterpret_cast<void *>(AGSteam_DownloadScores));
	lpGame->RegisterScriptFunction("AGSteam::geti_LeaderboardNames", reinterpret_cast<void *>(AGSteam_LeaderboardNames));
	lpGame->RegisterScriptFunction("AGSteam::geti_LeaderboardScores", reinterpret_cast<void *>(AGSteam_LeaderboardScores));
	lpGame->RegisterScriptFunction("AGSteam::get_LeaderboardCount", reinterpret_cast<void *>(AGSteam_LeaderboardCount));
	lpGame->RegisterScriptFunction("AGSteam::GetCurrentGameLanguage^0", reinterpret_cast<void *>(AGSteam_GetCurrentGameLanguage));
	lpGame->RegisterScriptFunction("AGSteam::GetUserName^0", reinterpret_cast<void *>(AGSteam_GetUserName));
}

void AGS_EngineShutdown() {
	debug_printf("AGSteam: AGS_EngineShutdown()\n");
	AGSteam_Delete();
	return;
}

int AGS_EngineOnEvent(int event, int data) {
	int ret = 0;
	debug_printf("AGSteam: AGS_EngineOnEvent(%d, %d): %d\n", event, data, ret);
	return ret;
}
