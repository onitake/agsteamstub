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

const char *AGSTEAM_PLUGIN_NAME = "AGSteamStub";
// This is empty, we don't provide any editor support.
const char *AGSTEAM_SCRIPT_HEADER = "";

static IAGSEditor *AGSteam_editor;
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
	printf("AGSteam: New(%p): %d\n", engine, AGSteam_initialized);
	return AGSteam_initialized ? 1 : 0;
}

static void AGSteam_Delete() {
	printf("AGSteam: Delete()\n");
	AGSteam_engine = 0;
	AGSteam_initialized = false;
}

static int AGSteam_IsAchievementAchieved(const char *steamAchievementID) {
	int ret = AGSteam_achievements[steamAchievementID] ? 1 : 0;
	printf("AGSteam: IsAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetAchievementAchieved(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = true;
	int ret = 1;
	printf("AGSteam: SetAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_ResetAchievement(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = false;
	int ret = 1;
	printf("AGSteam: ResetAchievement(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetIntStat(const char *steamStatName, int value) {
	AGSteam_intstats[steamStatName] = value;
	int ret = 1;
	printf("AGSteam: SetIntStat(%s, %d): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_SetFloatStat(const char *steamStatName, float value) {
	AGSteam_floatstats[steamStatName] = value;
	int ret = 1;
	printf("AGSteam: SetFloatStat(%s, %0.2f): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_UpdateAverageRateStat(const char *steamStatName, float numerator, float denominator) {
	AGSteam_ratestats[steamStatName] = std::make_pair(numerator, denominator);
	int ret = 1;
	printf("AGSteam: UpdateAverageRateStat(%s, %0.2f/%0.2f): %d\n", steamStatName, numerator, denominator, ret);
	return ret;
}

static int AGSteam_GetIntStat(const char *steamStatName) {
	int ret = AGSteam_intstats[steamStatName];
	printf("AGSteam: GetIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetFloatStat(const char *steamStatName) {
	float ret = AGSteam_floatstats[steamStatName];
	printf("AGSteam: GetFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetAverageRateStat(const char *steamStatName) {
	float ret = AGSteam_ratestats[steamStatName].first / AGSteam_ratestats[steamStatName].second;
	printf("AGSteam: GetAverageRateStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_ResetStats() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	printf("AGSteam: ResetStats()\n");
}

static void AGSteam_ResetStatsAndAchievements() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	AGSteam_achievements.clear();
	printf("AGSteam: ResetStatsAndAchievements()\n");
}

static int AGSteam_GetGlobalIntStat(const char *steamStatName) {
	int ret = 0;
	printf("AGSteam: GetGlobalIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetGlobalFloatStat(const char *steamStatName) {
	float ret = 0.0f;
	printf("AGSteam: GetGlobalFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_FindLeaderboard(const char *leaderboardName) {
	AGSteam_curlbname = leaderboardName;
	if (AGSteam_lbscores.count(AGSteam_curlbname) == 0) {
		AGSteam_lbnames.push_back(AGSteam_curlbname);
		AGSteam_lbscores[AGSteam_curlbname] = 0;
	}
	printf("AGSteam: FindLeaderboard(%s)\n", AGSteam_curlbname.c_str());
}

static int AGSteam_UploadScore(int score) {
	AGSteam_lbscores[AGSteam_curlbname] = score;
	int ret = 1;
	printf("AGSteam: UploadScore(%d): %d\n", score, ret);
	return ret;
}

static int AGSteam_DownloadScores(int type) {
	int ret = 1;
	printf("AGSteam: DownloadScores(%d): %d\n", type, ret);
	return ret;
}

static const char *AGSteam_GetCurrentGameLanguage() {
	printf("AGSteam: GetCurrentGameLanguage(): %s\n", AGSteam_language.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_language.c_str());
}

static const char *AGSteam_GetUserName() {
	printf("AGSteam: GetUserName(): %s\n", AGSteam_username.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_username.c_str());
}

static int AGSteam_Initialized(void *object) {
	int ret = AGSteam_initialized;
	printf("AGSteam: Initialized(%p): %d\n", object, ret);
	return ret;
}

static const char *AGSteam_CurrentLeaderboardName(void *object) {
	printf("AGSteam: CurrentLeaderboardName(%p): %s\n", object, AGSteam_curlbname.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_curlbname.c_str());
}

static const char *AGSteam_LeaderboardNames(void *object, int index) {
	std::string ret;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbnames[index];
	}
	printf("AGSteam: LeaderboardNames(%p, %d): %s\n", object, index, ret.c_str());
	return AGSteam_engine->CreateScriptString(ret.c_str());
}

static int AGSteam_LeaderboardScores(void *object, int index) {
	int ret = -1;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbscores[AGSteam_lbnames[index]];
	}
	printf("AGSteam: LeaderboardScores(%p, %d): %d\n", object, index, ret);
	return ret;
}

static int AGSteam_LeaderboardCount(void *object) {
	printf("AGSteam: LeaderboardCount(%p): %lu\n", object, AGSteam_lbnames.size());
	return AGSteam_lbnames.size();
}

const char *AGS_GetPluginName() {
	printf("AGSteam: AGS_GetPluginName()\n");
	return AGSTEAM_PLUGIN_NAME;
}

int AGS_EditorStartup(IAGSEditor *lpEditor) {
	int ret = -1;
	if (lpEditor->version >= 1) {
		AGSteam_editor = lpEditor;
		AGSteam_editor->RegisterScriptHeader(AGSTEAM_SCRIPT_HEADER);
	}
	printf("AGSteam: AGS_EditorStartup(%p): %d", lpEditor, ret);
	return ret;
}

void AGS_EditorShutdown() {
	printf("AGSteam: AGS_EditorShutdown()\n");
	AGSteam_editor->UnregisterScriptHeader(AGSTEAM_SCRIPT_HEADER);
	AGSteam_editor = 0;
}

void AGS_EditorProperties(HWND parent) {
	printf("AGSteam: AGS_EditorProperties(%d)\n", parent);
	printf("AGSteam stub plugin for use without Steam - Public domain\n");
}

int AGS_EditorSaveGame(char *buffer, int bufsize) {
	int ret = 0;
	printf("AGSteam: AGS_EditorSaveGame(%p, %d): %d\n", buffer, bufsize, ret);
	return ret;
}

void AGS_EditorLoadGame(char *buffer, int bufsize) {
	printf("AGSteam: AGS_EditorLoadGame(%p, %d)\n", buffer, bufsize);
}

void AGS_EngineStartup(IAGSEngine *lpGame) {
	printf("AGSteam: AGS_EngineStartup(%p)\n", lpGame);
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
	printf("AGSteam: AGS_EngineShutdown()\n");
	AGSteam_Delete();
	return;
}

int AGS_EngineOnEvent(int event, int data) {
	int ret = 0;
	printf("AGSteam: AGS_EngineOnEvent(%d, %d): %d\n", event, data, ret);
	return ret;
}
