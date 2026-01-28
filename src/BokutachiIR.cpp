#include <LR2_customir_api.h>

#include <print>
#include <format>
#include <filesystem>
#include <fstream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

// Technically hook version, but let's think of it as of API version :^)
struct version {
	int major = 2;
	int minor = 1;
	int patch = 2;
} version;

static std::filesystem::path path;
static std::string url;
static std::string urlDan;
static std::string apiKey;

constexpr const char* lamps[6] = { "NO PLAY", "FAIL", "EASY", "NORMAL", "HARD", "FULL COMBO" };
constexpr const char* gauges[6] = { "GROOVE", "HARD", "HAZARD", "EASY", "P-ATTACK", "G-ATTACK" };
constexpr const char* gameModes[8] = { "7K", "5K", "14K", "10K", "9K" };
constexpr const char* randomModes[6] = { "NORAN", "MIRROR", "RAN", "S-RAN", "H-RAN", "ALLSCR" };

static bool is_wine()
{
	auto* ntdll = GetModuleHandle("ntdll");
	return ntdll != nullptr && static_cast<void*>(GetProcAddress(ntdll, "wine_get_version")) != nullptr;
}

static void Logger(std::string message)
{
	std::println("[BokutachiHook] {}", message);
	fflush(stdout);

	std::ofstream logFile(path/"Bokutachi.log", std::ios_base::app);
	if (is_wine()) {
		logFile << std::format("[{:%d-%m-%Y %X}] {}\n", std::chrono::system_clock::now(), message);
	}
	else {
		auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		logFile << std::format("[{:%d-%m-%Y %X}] {}\n", time, message);
	}
}

static const char* __cdecl GetName() {
	return "BokutachiIR";
}

// \retval true Good
static bool CheckTachiApi() {
	std::string baseUrl = url.substr(0, url.find_first_of("/", 8));
	cpr::Response r = cpr::Get(cpr::Url{ baseUrl + "/api/v1/status" },
							   cpr::Timeout{ std::chrono::seconds(5) },
							   cpr::Bearer{ apiKey });

	if (r.error.code != cpr::ErrorCode::OK) {
		Logger(std::format("Couldn't GET: {}", r.error.message));
		return false;
	}

	try
	{
		json json = json::parse(r.text);
		if (json["body"]["whoami"] == nullptr) {
			Logger("Missing/Unknown API Key in 'BokutachiAuth.json'.");
			return false;
		}

		bool permissionsGood = false;
		for (auto& permission : json["body"]["permissions"]) {
			if (permission != "submit_score") continue;
			permissionsGood = true;
			break;
		}
		if (!permissionsGood) {
			Logger("API Key in BokutachiAuth.json is missing 'submit_score' permission.");
			return false;
		}
	}
	catch (json::exception& e)
	{
		Logger(std::format("JSON exception: {}", e.what()));
	}

	return true;
}

static bool __cdecl Login() {
	try {
		json config;
		{
			std::ifstream conf(path/"BokutachiAuth.json");
			config = json::parse(conf);
		}
		url = config.at("url");
		urlDan = url + "/course";
		apiKey = config.at("apiKey");
	}
	catch (const std::exception& e) {
		Logger("'BokutachiAuth.json' is missing or malformed.");
		return false;
	}
	return CheckTachiApi();
}

static const char* GetKeymode(int keymode) {
	switch (keymode) {
	case 7: return gameModes[0];
	case 5: return gameModes[1];
	case 14: return gameModes[2];
	case 10: return gameModes[3];
	case 9: return gameModes[4];
	default: return "INVALID";
	}
}

static std::string FormJSONString(const IRScoreV1& score) {
	const bool isCourse = score.state.isCourse;
	const bool hashIsCourse = score.song.hash.length() > 32;

	const std::chrono::seconds unixTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());

	const std::string md5 = hashIsCourse
			? std::string(score.song.hash.begin() + 32, score.song.hash.end())
			: score.song.hash;

	json scorePacket = {
		{"version", {
			{"major", version.major},
			{"minor", version.minor},
			{"patch", version.patch}
		}},
		{"unixTimestamp", unixTimestamp.count()},
		{"md5", md5},
		{"playerData", {
					{"autoScr", score.settings.assist[0] | score.settings.assist[1]},
					{"gameMode", GetKeymode(score.state.keymode)},
					{"random", randomModes[score.settings.random[0]]},
					{"gauge", gauges[score.gaugeType]},
					{"rseed", score.state.randomseed}
		}},
		{"scoreData", {
					{"pgreat", score.judgements_total.epg + score.judgements_total.lpg},
					{"great", score.judgements_total.egr + score.judgements_total.lgr},
					{"good", score.judgements_total.egd + score.judgements_total.lgd},
					{"bad", score.judgements_total.ebd + score.judgements_total.lbd},
					{"poor", score.judgements_total.epr + score.judgements_total.lpr},
					{"maxCombo", score.max_combo},
					{"exScore", score.exscore},
					{"moneyScore", score.moneyscore},
					{"notesTotal", score.state.notes_total},
					{"notesPlayed", score.judgements_total.notes_played},
					{"lamp", lamps[score.clearType]},
					{"hpGraph", score.graphs.hp[score.gaugeType]},
					{"extendedJudgements", {
							{"epg", score.judgements_total.epg},
							{"lpg", score.judgements_total.lpg},
							{"egr", score.judgements_total.egr},
							{"lgr", score.judgements_total.lgr},
							{"egd", score.judgements_total.egd},
							{"lgd", score.judgements_total.lgd},
							{"ebd", score.judgements_total.ebd},
							{"lbd", score.judgements_total.lbd},
							{"epr", score.judgements_total.epr},
							{"lpr", score.judgements_total.lpr},
							{"cb", score.judgements_total.cb},
							{"fast", score.judgements_total.fast},
							{"slow", score.judgements_total.slow},
							{"notesPlayed", score.judgements_total.notes_played}
						}
					},
					{"extendedHpGraphs", nullptr}
		}}
	};

	// TODO: if (DP) {"randomr", randomModes[score.settings.random[1]]},

	if (!hashIsCourse && isCourse && score.clearType > 1)
	{
		scorePacket["scoreData"]["lamp"] = lamps[0];
	}

	if (score.settings.m_gas && !hashIsCourse) {
		scorePacket["scoreData"]["extendedHpGraphs"] = {
			{"groove", score.graphs.hp[0]},
			{"hard", score.graphs.hp[1]},
			{"hazard", score.graphs.hp[2]},
			{"easy", score.graphs.hp[3]},
			{"pattack", score.graphs.hp[4]},
			{"gattack", score.graphs.hp[5]}
		};
	}
	return scorePacket.dump(4);
}

static SendScoreStatus __cdecl SendScore(const IRScoreV1& score) {
	const std::string reqBody = FormJSONString(score);
	const std::string songName = std::format("{} {}", score.song.title, score.song.subtitle);
	const bool hashIsCourse = score.song.hash.length() > 32;

	if (score.settings.is_extra) {
		Logger("Score not sent - Extra mode enabled");
		return SendScoreStatus::Fail;
	}

	cpr::Response r = cpr::Post(cpr::Url{ hashIsCourse ? urlDan : url },
		cpr::Timeout{ std::chrono::seconds(30) },
		cpr::Header{ {"Content-Type", "application/json"} },
		cpr::Bearer{ apiKey },
		cpr::Body{ reqBody });

	if (r.error.code != cpr::ErrorCode::OK || r.status_code / 100 == 5) {
		Logger(std::format("Couldn't POST: {}", r.error.message));
		return SendScoreStatus::Retry;
	}

	try
	{
		json log = json::parse(r.text);
		if (!log["success"]) {
			Logger(std::format("Score for {} !success: {}", songName, std::string_view(log["description"])));
			Logger(reqBody);
			return SendScoreStatus::Fail;
		}
	}
	catch (json::exception& e)
	{
		Logger(std::format("JSON exception: {}", e.what()));
	}

	if (r.status_code != 200) {
		Logger(std::format("Score for {} !=200: {}", songName, r.status_line));
		Logger(reqBody);
		return SendScoreStatus::Fail;
	}

	return SendScoreStatus::Ok;
}

extern "C" __declspec(dllexport) void GetMethodTable(MethodTable& table) {
	table.GetName = &GetName;
	table.LoginV1 = &Login;
	table.SendScoreV1 = &SendScore;
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		wchar_t modulePath[MAX_PATH]{};
		GetModuleFileNameW(hModule, modulePath, MAX_PATH);
		path = modulePath;
		path = path.parent_path();
		break;
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		if (lpReserved != nullptr) {
			break;
		}
		break;
	}
	return TRUE;
}
