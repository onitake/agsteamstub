// AGS Steam stub plugin
// Steam-free replacement for AGSteam by monkey (see http://www.adventuregamestudio.co.uk/forums/index.php?topic=44712.0)
// No copyright. This is public domain.

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Do any Windows specific constructor/destructor work here.
// Do NOT use for plugin initialization code.
// DllMain will never be called on non-Windows platforms.
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call)	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
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

const char *AGSTEAM_PLUGIN_NAME = "AGSteam";
const char *AGSTEAM_SCRIPT_HEADER =
	"#define AGSteam_VERSION 3.0\n"
	"enum AGSteamStatType\n"
	"  eAGSteamStatInt = 0,\n"
	"  eAGSteamStatFloat = 1,\n"
	"  eAGSteamStatAverageRate = 2\n"
	"enum AGSteamScoresRequestType\n"
	"  eAGSteamScoresRequestGlobal = 0,\n"
	"  eAGSteamScoresRequestAroundUser = 1,\n"
	"  eAGSteamScoresRequestFriends = 2\n"
	"managed struct AGSteam\n"
	"  ///AGSteam: Returns whether the specified Steam achievement has been achieved\n"
	"  import static int IsAchievementAchieved(const string steamAchievementID);\n"
	"  ///AGSteam: Sets a Steam achievement as achieved\n"
	"  import static int SetAchievementAchieved(const string steamAchievementID);\n"
	"  ///AGSteam: Resets the specified Steam achievement, so it can be achieved again\n"
	"  import static int ResetAchievement(const string steamAchievementID);\n"
	"  ///AGSteam: Sets the value of a Steam INT stat\n"
	"  import static int SetIntStat(const string steamStatName, int value);\n"
	"  ///AGSteam: Sets the value of a Steam FLOAT stat\n"
	"  import static int SetFloatStat(const string steamStatName, float value);\n"
	"  ///AGSteam: Updates a Steam AVGRATE stat\n"
	"  import static int UpdateAverageRateStat(const string steamStatName, float numerator, float denominator);\n"
	"  ///AGSteam: Returns the value of a Steam INT stat\n"
	"  import static int GetIntStat(const string steamStatName);\n"
	"  ///AGSteam: Returns the value of a Steam FLOAT stat\n"
	"  import static float GetFloatStat(const string steamStatName);\n"
	"  ///AGSteam: Returns the value of a Steam AVGRATE stat\n"
	"  import static float GetAverageRateStat(const string steamStatName);\n"
	"  ///AGSteam: Resets all Steam stats to their default values\n"
	"  import static void ResetStats();\n"
	"  ///AGSteam: Resets all Steam stats and achievements\n"
	"  import static void ResetStatsAndAchievements();\n"
	"  ///AGSteam: Returns whether the Steam client is loaded and the user logged in\n"
	"  readonly import static attribute int Initialized;\n"
	"  ///AGSteam: Returns the value of a global Steam INT stat\n"
	"  import static int GetGlobalIntStat(const string steamStatName);\n"
	"  ///AGSteam: Returns the value of a global Steam FLOAT stat\n"
	"  import static float GetGlobalFloatStat(const string steamStatName);\n"
	"  ///AGSteam: Returns the name of the current leaderboard (call FindLeadboard first)\n"
	"  readonly import static attribute String CurrentLeaderboardName;\n"
	"  ///AGSteam: Requests to load the specified Steam leaderboard. This call is asynchronous and does not return the data immediately, check for results in repeatedly_execute.\n"
	"  import static void FindLeaderboard(const string leaderboardName);\n"
	"  ///AGSteam: Uploads the score to the current Steam leaderboard. Returns false if an error occurred.\n"
	"  import static int UploadScore(int score);\n"
	"  ///AGSteam: Downloads a list of ten scores from the current Steam leaderboard.\n"
	"  import static int DownloadScores(AGSteamScoresRequestType);\n"
	"  ///AGSteam: Returns the name associated with a downloaded score. Call DownloadScores first.\n"
	"  readonly import static attribute String LeaderboardNames[];\n"
	"  ///AGSteam: Returns a downloaded score. Call DownloadScores first.\n"
	"  readonly import static attribute int LeaderboardScores[];\n"
	"  ///AGSteam: Returns the number of downloaded scores (if any). Call DownloadScores first. Max is 10 scores.\n"
	"  readonly import static attribute int LeaderboardCount;\n"
	"  ///AGSteam: Returns the current game language as registered by the Steam client.\n"
	"  import static String GetCurrentGameLanguage();\n"
	"  ///AGSteam: Returns the Steam user's username.\n"
	"  import static String GetUserName();\n"
;

enum AGSteamStatType {
	eAGSteamStatInt = 0,
	eAGSteamStatFloat = 1,
	eAGSteamStatAverageRate = 2
};
enum AGSteamScoresRequestType {
	eAGSteamScoresRequestGlobal = 0,
	eAGSteamScoresRequestAroundUser = 1,
	eAGSteamScoresRequestFriends = 2
};

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

static int AGSteam_DownloadScores(AGSteamScoresRequestType type) {
	int ret = 1;
	printf("AGSteam: DownloadScores(%s): %d\n", type == eAGSteamScoresRequestGlobal ? "global" : type == eAGSteamScoresRequestAroundUser ? "around" : type == eAGSteamScoresRequestFriends ? "friends" : "unknown", ret);
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
	// Disable all Steam functionality for now to hide brokenness of this plugin
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
