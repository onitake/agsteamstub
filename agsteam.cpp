// AGS Steam stub plugin
// Steam-free replacement for the AGSteam plugin (see http://www.adventuregamestudio.co.uk/forums/index.php?topic=44712.0)
// No copyright. This is public domain.


// Windows DLL entry point

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


// Includes and macros

#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <cstdarg>

#define THIS_IS_THE_PLUGIN
#include "agsplugin.h"

#define AGSTEAM_PLUGIN_NAME "AGSteamStub"
#define AGSTEAM_MESSAGE_PREFIX AGSTEAM_PLUGIN_NAME ": "
#define AGSTEAM_PERSISTENT_DB "$SAVEGAMEDIR$/agsteam.db"

#ifdef DEBUG
#define debug_printf(args...) AGSteam_Printf("[DEBUG] " AGSTEAM_MESSAGE_PREFIX args)
#else
#define debug_printf(...)
#endif


// Declarations

// AGS internals
struct AGSFile;
enum FileMode {
	eFileRead = 1,
	eFileWrite = 2,
	eFileAppend = 3
};
typedef AGSFile *(*AGSFileOpen)(const char *filename, int mode);
typedef void (*AGSFileClose)(AGSFile *file);
typedef int (*AGSFileReadInt)(AGSFile *file);
typedef const char *(*AGSFileReadStringBack)(AGSFile *file);
typedef void (*AGSFileWriteInt)(AGSFile *file, int value);
typedef void (*AGSFileWriteString)(AGSFile *file, const char *value);

// Global state
static IAGSEngine *AGSteam_engine = 0;
static std::string AGSteam_language = "";
static std::string AGSteam_username = "";
static bool AGSteam_initialized = false;
static std::string AGSteam_curlbname = "";
static std::vector<std::string> AGSteam_lbnames;
static std::map<std::string, int> AGSteam_lbscores;
static std::map<std::string, bool> AGSteam_achievements;
static std::map<std::string, int> AGSteam_intstats;
static std::map<std::string, float> AGSteam_floatstats;
static std::map<std::string, std::pair<float, float> > AGSteam_ratestats;

// Helper function declarations
static void AGSteam_Printf(const char *format, ...);
static int AGSteam_New(IAGSEngine *engine);
static void AGSteam_Delete();
static bool AGSteam_LoadDb();
static bool AGSteam_SaveDb();

// Script interface declarations
static int AGSteam_IsAchievementAchieved(const char *steamAchievementID);
static int AGSteam_SetAchievementAchieved(const char *steamAchievementID);
static int AGSteam_ResetAchievement(const char *steamAchievementID);
static int AGSteam_SetIntStat(const char *steamStatName, int value);
static int AGSteam_SetFloatStat(const char *steamStatName, float value);
static int AGSteam_UpdateAverageRateStat(const char *steamStatName, float numerator, float denominator);
static int AGSteam_GetIntStat(const char *steamStatName);
static float AGSteam_GetFloatStat(const char *steamStatName);
static float AGSteam_GetAverageRateStat(const char *steamStatName);
static void AGSteam_ResetStats();
static void AGSteam_ResetStatsAndAchievements();
static int AGSteam_GetGlobalIntStat(const char *steamStatName);
static float AGSteam_GetGlobalFloatStat(const char *steamStatName);
static void AGSteam_FindLeaderboard(const char *leaderboardName);
static int AGSteam_UploadScore(int score);
static int AGSteam_DownloadScores(int type);
static const char *AGSteam_GetCurrentGameLanguage();
static const char *AGSteam_GetUserName();
static int AGSteam_Initialized(void *object);
static const char *AGSteam_CurrentLeaderboardName(void *object);
static const char *AGSteam_LeaderboardNames(void *object, int index);
static int AGSteam_LeaderboardScores(void *object, int index);
static int AGSteam_LeaderboardCount(void *object);


// Helper functions

static void AGSteam_Printf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *buffer = new char[256];
	if (buffer) {
		if (vsnprintf(buffer, 255, format, args) != -1) {
			if (AGSteam_engine) {
				AGSteam_engine->PrintDebugConsole(buffer);
			} else {
				puts(buffer);
			}
		}
		delete[] buffer;
	}
	va_end(args);
}

static bool AGSteam_LoadDb() {
	bool loaded = false;
	AGSFileOpen fileopen = reinterpret_cast<AGSFileOpen>(AGSteam_engine->GetScriptFunctionAddress("File::Open^2"));
	AGSFileClose fileclose = reinterpret_cast<AGSFileClose>(AGSteam_engine->GetScriptFunctionAddress("File::Close^0"));
	debug_printf("File.Open=%p File.Close=%p\n", fileopen, fileclose);
	if (fileopen && fileclose) {
		AGSFile *file = fileopen(AGSteam_engine->CreateScriptString(AGSTEAM_PERSISTENT_DB), eFileRead);
		debug_printf("file=%p\n", file);
		if (file) {
			AGSFileReadInt readint = reinterpret_cast<AGSFileReadInt>(AGSteam_engine->GetScriptFunctionAddress("File::ReadInt^0"));
			AGSFileReadStringBack readstring = reinterpret_cast<AGSFileReadStringBack>(AGSteam_engine->GetScriptFunctionAddress("File::ReadStringBack^0"));
			debug_printf("File.ReadInt=%p File.ReadStringBack=%p\n", readint, readstring);
			if (readint && readstring) {
				std::string magic = readstring(file);
				if (magic == AGSTEAM_PLUGIN_NAME) {
					AGSteam_language = readstring(file);
					AGSteam_username = readstring(file);
					int leaderboards = readint(file);
					for (int i = 0; i < leaderboards; i++) {
						std::string name = readstring(file);
						AGSteam_lbnames.push_back(name);
						int score = readint(file);
						AGSteam_lbscores[name] = score;
					}
					int achievements = readint(file);
					for (int i = 0; i < achievements; i++) {
						std::string name = readstring(file);
						int achieved = readint(file);
						AGSteam_achievements[name] = achieved ? true : false;
					}
					int intstats = readint(file);
					for (int i = 0; i < intstats; i++) {
						std::string name = readstring(file);
						int stats = readint(file);
						AGSteam_intstats[name] = stats;
					}
					int floatstats = readint(file);
					for (int i = 0; i < floatstats; i++) {
						std::string name = readstring(file);
						union {
							int from;
							float to;
						} stats;
						stats.from = readint(file);
						AGSteam_floatstats[name] = stats.to;
					}
					int ratestats = readint(file);
					for (int i = 0; i < ratestats; i++) {
						std::string name = readstring(file);
						union {
							int from;
							float to;
						} num, den;
						num.from = readint(file);
						den.from = readint(file);
						AGSteam_ratestats[name] = std::make_pair(num.to, den.to);
					}
					loaded = true;
				} else {
					debug_printf("Error reading persistent data, invalid magic\n");
				}
			}
			fileclose(file);
		}
	}
	return loaded;
}

static bool AGSteam_SaveDb() {
	bool saved = false;
	AGSFileOpen fileopen = reinterpret_cast<AGSFileOpen>(AGSteam_engine->GetScriptFunctionAddress("File::Open^2"));
	AGSFileClose fileclose = reinterpret_cast<AGSFileClose>(AGSteam_engine->GetScriptFunctionAddress("File::Close^0"));
	debug_printf("File.Open=%p File.Close=%p\n", fileopen, fileclose);
	if (fileopen && fileclose) {
		AGSFile *file = fileopen(AGSteam_engine->CreateScriptString(AGSTEAM_PERSISTENT_DB), eFileWrite);
		debug_printf("file=%p\n", file);
		if (file) {
			AGSFileWriteInt writeint = reinterpret_cast<AGSFileWriteInt>(AGSteam_engine->GetScriptFunctionAddress("File::WriteInt^1"));
			AGSFileWriteString writestring = reinterpret_cast<AGSFileWriteString>(AGSteam_engine->GetScriptFunctionAddress("File::WriteString^1"));
			debug_printf("File.WriteInt=%p File.WriteString=%p\n", writeint, writestring);
			if (writeint && writestring) {
				writestring(file, AGSteam_engine->CreateScriptString(AGSTEAM_PLUGIN_NAME));
				writestring(file, AGSteam_engine->CreateScriptString(AGSteam_language.c_str()));
				writestring(file, AGSteam_engine->CreateScriptString(AGSteam_username.c_str()));
				writeint(file, AGSteam_lbnames.size());
				for (std::vector<std::string>::const_iterator it = AGSteam_lbnames.begin(); it != AGSteam_lbnames.end(); it++) {
					const std::string name = *it;
					writestring(file, AGSteam_engine->CreateScriptString(name.c_str()));
					writeint(file, AGSteam_lbscores[name]);
				}
				writeint(file, AGSteam_achievements.size());
				for (std::map<std::string, bool>::const_iterator it = AGSteam_achievements.begin(); it != AGSteam_achievements.end(); it++) {
					const std::string name = it->first;
					bool achieved = it->second;
					writestring(file, AGSteam_engine->CreateScriptString(name.c_str()));
					writeint(file, achieved ? 1 : 0);
				}
				writeint(file, AGSteam_intstats.size());
				for (std::map<std::string, int>::const_iterator it = AGSteam_intstats.begin(); it != AGSteam_intstats.end(); it++) {
					const std::string name = it->first;
					int stats = it->second;
					writestring(file, AGSteam_engine->CreateScriptString(name.c_str()));
					writeint(file, stats);
				}
				writeint(file, AGSteam_floatstats.size());
				for (std::map<std::string, float>::const_iterator it = AGSteam_floatstats.begin(); it != AGSteam_floatstats.end(); it++) {
					const std::string name = it->first;
					union {
						float from;
						int to;
					} stats;
					stats.from = it->second;
					writestring(file, AGSteam_engine->CreateScriptString(name.c_str()));
					writeint(file, stats.to);
				}
				writeint(file, AGSteam_ratestats.size());
				for (std::map<std::string, std::pair<float, float> >::const_iterator it = AGSteam_ratestats.begin(); it != AGSteam_ratestats.end(); it++) {
					const std::string name = it->first;
					union {
						float from;
						int to;
					} num, den;
					num.from = it->second.first;
					den.from = it->second.second;
					writestring(file, AGSteam_engine->CreateScriptString(name.c_str()));
					writeint(file, num.to);
					writeint(file, den.to);
				}
				saved = true;
			}
			fileclose(file);
		}
	}
	return saved;
}

static int AGSteam_New(IAGSEngine *engine) {
	if (!AGSteam_initialized) {
		if (engine->version < 17) {
			engine->AbortGame(AGSTEAM_MESSAGE_PREFIX "Plugin needs a newer version of AGS to run (engine interface version 17)");
		} else {
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
			AGSteam_engine->RegisterScriptFunction("AGSteam::IsAchievementAchieved^1", reinterpret_cast<void *>(AGSteam_IsAchievementAchieved));
			AGSteam_engine->RegisterScriptFunction("AGSteam::SetAchievementAchieved^1", reinterpret_cast<void *>(AGSteam_SetAchievementAchieved));
			AGSteam_engine->RegisterScriptFunction("AGSteam::ResetAchievement^1", reinterpret_cast<void *>(AGSteam_ResetAchievement));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetIntStat^1", reinterpret_cast<void *>(AGSteam_GetIntStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetFloatStat^1", reinterpret_cast<void *>(AGSteam_GetFloatStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetAverageRateStat^1", reinterpret_cast<void *>(AGSteam_GetAverageRateStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::SetIntStat^2", reinterpret_cast<void *>(AGSteam_SetIntStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::SetFloatStat^2", reinterpret_cast<void *>(AGSteam_SetFloatStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::UpdateAverageRateStat^3", reinterpret_cast<void *>(AGSteam_UpdateAverageRateStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::ResetStats^0", reinterpret_cast<void *>(AGSteam_ResetStats));
			AGSteam_engine->RegisterScriptFunction("AGSteam::ResetStatsAndAchievements^0", reinterpret_cast<void *>(AGSteam_ResetStatsAndAchievements));
			AGSteam_engine->RegisterScriptFunction("AGSteam::get_Initialized", reinterpret_cast<void *>(AGSteam_Initialized));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetGlobalIntStat^1", reinterpret_cast<void *>(AGSteam_GetGlobalIntStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetGlobalFloatStat^1", reinterpret_cast<void *>(AGSteam_GetGlobalFloatStat));
			AGSteam_engine->RegisterScriptFunction("AGSteam::get_CurrentLeaderboardName", reinterpret_cast<void *>(AGSteam_CurrentLeaderboardName));
			AGSteam_engine->RegisterScriptFunction("AGSteam::FindLeaderboard^1", reinterpret_cast<void *>(AGSteam_FindLeaderboard));
			AGSteam_engine->RegisterScriptFunction("AGSteam::UploadScore^1", reinterpret_cast<void *>(AGSteam_UploadScore));
			AGSteam_engine->RegisterScriptFunction("AGSteam::DownloadScores^1", reinterpret_cast<void *>(AGSteam_DownloadScores));
			AGSteam_engine->RegisterScriptFunction("AGSteam::geti_LeaderboardNames", reinterpret_cast<void *>(AGSteam_LeaderboardNames));
			AGSteam_engine->RegisterScriptFunction("AGSteam::geti_LeaderboardScores", reinterpret_cast<void *>(AGSteam_LeaderboardScores));
			AGSteam_engine->RegisterScriptFunction("AGSteam::get_LeaderboardCount", reinterpret_cast<void *>(AGSteam_LeaderboardCount));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetCurrentGameLanguage^0", reinterpret_cast<void *>(AGSteam_GetCurrentGameLanguage));
			AGSteam_engine->RegisterScriptFunction("AGSteam::GetUserName^0", reinterpret_cast<void *>(AGSteam_GetUserName));
			if (AGSteam_LoadDb()) {
				AGSteam_Printf(AGSTEAM_MESSAGE_PREFIX "Loaded persistent data\n");
			}
			AGSteam_initialized = true;
			debug_printf("New(%p): %d\n", engine, AGSteam_initialized);
			AGSteam_Printf(AGSTEAM_MESSAGE_PREFIX "Loaded plugin\n");
		}
	}
	return AGSteam_initialized ? 1 : 0;
}

static void AGSteam_Delete() {
	debug_printf("Delete()\n");
	if (AGSteam_initialized) {
		if (AGSteam_SaveDb()) {
			AGSteam_Printf(AGSTEAM_MESSAGE_PREFIX "Wrote persistent data\n");
		}
	}
	AGSteam_Printf(AGSTEAM_MESSAGE_PREFIX "Unloaded plugin\n");
	AGSteam_engine = 0;
	AGSteam_initialized = false;
}


// Script interface

static int AGSteam_IsAchievementAchieved(const char *steamAchievementID) {
	int ret = AGSteam_achievements[steamAchievementID] ? 1 : 0;
	debug_printf("IsAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetAchievementAchieved(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = true;
	int ret = 1;
	debug_printf("SetAchievementAchieved(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_ResetAchievement(const char *steamAchievementID) {
	AGSteam_achievements[steamAchievementID] = false;
	int ret = 1;
	debug_printf("ResetAchievement(%s): %d\n", steamAchievementID, ret);
	return ret;
}

static int AGSteam_SetIntStat(const char *steamStatName, int value) {
	AGSteam_intstats[steamStatName] = value;
	int ret = 1;
	debug_printf("SetIntStat(%s, %d): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_SetFloatStat(const char *steamStatName, float value) {
	AGSteam_floatstats[steamStatName] = value;
	int ret = 1;
	debug_printf("SetFloatStat(%s, %0.2f): %d\n", steamStatName, value, ret);
	return ret;
}

static int AGSteam_UpdateAverageRateStat(const char *steamStatName, float numerator, float denominator) {
	AGSteam_ratestats[steamStatName] = std::make_pair(numerator, denominator);
	int ret = 1;
	debug_printf("UpdateAverageRateStat(%s, %0.2f/%0.2f): %d\n", steamStatName, numerator, denominator, ret);
	return ret;
}

static int AGSteam_GetIntStat(const char *steamStatName) {
	int ret = AGSteam_intstats[steamStatName];
	debug_printf("GetIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetFloatStat(const char *steamStatName) {
	float ret = AGSteam_floatstats[steamStatName];
	debug_printf("GetFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetAverageRateStat(const char *steamStatName) {
	float ret = AGSteam_ratestats[steamStatName].first / AGSteam_ratestats[steamStatName].second;
	debug_printf("GetAverageRateStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_ResetStats() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	debug_printf("ResetStats()\n");
}

static void AGSteam_ResetStatsAndAchievements() {
	AGSteam_intstats.clear();
	AGSteam_floatstats.clear();
	AGSteam_ratestats.clear();
	AGSteam_achievements.clear();
	debug_printf("ResetStatsAndAchievements()\n");
}

static int AGSteam_GetGlobalIntStat(const char *steamStatName) {
	int ret = 0;
	debug_printf("GetGlobalIntStat(%s): %d\n", steamStatName, ret);
	return ret;
}

static float AGSteam_GetGlobalFloatStat(const char *steamStatName) {
	float ret = 0.0f;
	debug_printf("GetGlobalFloatStat(%s): %0.2f\n", steamStatName, ret);
	return ret;
}

static void AGSteam_FindLeaderboard(const char *leaderboardName) {
	AGSteam_curlbname = leaderboardName;
	if (AGSteam_lbscores.count(AGSteam_curlbname) == 0) {
		AGSteam_lbnames.push_back(AGSteam_curlbname);
		AGSteam_lbscores[AGSteam_curlbname] = 0;
	}
	debug_printf("FindLeaderboard(%s)\n", AGSteam_curlbname.c_str());
}

static int AGSteam_UploadScore(int score) {
	AGSteam_lbscores[AGSteam_curlbname] = score;
	int ret = 1;
	debug_printf("UploadScore(%d): %d\n", score, ret);
	return ret;
}

static int AGSteam_DownloadScores(int type) {
	int ret = 1;
	debug_printf("DownloadScores(%d): %d\n", type, ret);
	return ret;
}

static const char *AGSteam_GetCurrentGameLanguage() {
	debug_printf("GetCurrentGameLanguage(): %s\n", AGSteam_language.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_language.c_str());
}

static const char *AGSteam_GetUserName() {
	debug_printf("GetUserName(): %s\n", AGSteam_username.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_username.c_str());
}

static int AGSteam_Initialized(void *object) {
	int ret = AGSteam_initialized;
	debug_printf("Initialized(%p): %d\n", object, ret);
	return ret;
}

static const char *AGSteam_CurrentLeaderboardName(void *object) {
	debug_printf("CurrentLeaderboardName(%p): %s\n", object, AGSteam_curlbname.c_str());
	return AGSteam_engine->CreateScriptString(AGSteam_curlbname.c_str());
}

static const char *AGSteam_LeaderboardNames(void *object, int index) {
	std::string ret;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbnames[index];
	}
	debug_printf("LeaderboardNames(%p, %d): %s\n", object, index, ret.c_str());
	return AGSteam_engine->CreateScriptString(ret.c_str());
}

static int AGSteam_LeaderboardScores(void *object, int index) {
	int ret = -1;
	if (static_cast<unsigned int>(index) < AGSteam_lbnames.size()) {
		ret = AGSteam_lbscores[AGSteam_lbnames[index]];
	}
	debug_printf("LeaderboardScores(%p, %d): %d\n", object, index, ret);
	return ret;
}

static int AGSteam_LeaderboardCount(void *object) {
	debug_printf("LeaderboardCount(%p): %lu\n", object, AGSteam_lbnames.size());
	return AGSteam_lbnames.size();
}


// AGS engine interface

const char *AGS_GetPluginName() {
	debug_printf("AGS_GetPluginName()\n");
	return AGSTEAM_PLUGIN_NAME;
}

void AGS_EngineStartup(IAGSEngine *lpGame) {
	AGSteam_New(lpGame);
	debug_printf("AGS_EngineStartup(%p)\n", lpGame);
}

void AGS_EngineShutdown() {
	debug_printf("AGS_EngineShutdown()\n");
	AGSteam_Delete();
	return;
}

int AGS_EngineOnEvent(int event, int data) {
	int ret = 0;
	debug_printf("AGS_EngineOnEvent(%d, %d): %d\n", event, data, ret);
	return ret;
}
