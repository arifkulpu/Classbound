#include "GlobalCharacterDB.h"
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>

using json = nlohmann::json;

namespace Classbound
{
	std::string GlobalCharacterDB::GetDBPath() const
	{
		auto path = SKSE::log::log_directory();
		if (!path) return "Data/SKSE/Plugins/ClassBound_Characters.json";
		
		std::filesystem::path dir = *path;
		dir = dir.parent_path(); // usually goes to Data/SKSE/Plugins
		dir /= "ClassBound_Characters.json";
		return dir.string();
	}

	void GlobalCharacterDB::LoadDB()
	{
		_entries.clear();
		std::string path = GetDBPath();
		
		if (!std::filesystem::exists(path)) return;

		try {
			std::ifstream ifs(path);
			if (!ifs.is_open()) return;

			json j;
			ifs >> j;

			if (j.contains("characters") && j["characters"].is_array()) {
				for (const auto& item : j["characters"]) {
					CharacterSaveEntry e;
					e.charName     = item.value("charName", "Unknown");
					e.saveFileName = item.value("saveFileName", "");
					e.classType    = item.value("classType", 0);
					e.classLevel   = item.value("classLevel", 1);
					e.lastUpdate   = item.value("lastUpdate", "");
					_entries.push_back(e);
				}
			}
			SKSE::log::info("GlobalCharacterDB loaded {} entries.", _entries.size());
			PruneStaleEntries();
		} catch (const std::exception& e) {
			SKSE::log::error("GlobalCharacterDB parse error: {}", e.what());
		}
	}

	void GlobalCharacterDB::PruneStaleEntries()
	{
		// Remove empty character entries or invalid duplicates
		auto initialSize = _entries.size();
		_entries.erase(
			std::remove_if(_entries.begin(), _entries.end(),
				[](const CharacterSaveEntry& e) {
				// Minor Fix: Added explicit parentheses — && binds tighter than ||, so without them
			// only entries with charName=="Unknown" AND empty saveFileName would be pruned,
			// but entries with just an empty charName would not. The intent is to prune either case.
			return e.charName.empty() || (e.charName == "Unknown" && e.saveFileName.empty());
				}),
			_entries.end()
		);

		if (_entries.size() != initialSize) {
			SKSE::log::info("GlobalCharacterDB pruned {} stale entries.", initialSize - _entries.size());
			SaveDB();
		}
	}

	void GlobalCharacterDB::SaveDB()
	{
		std::string path = GetDBPath();
		try {
			json j;
			j["characters"] = json::array();
			
			for (const auto& e : _entries) {
				json item;
				item["charName"]     = e.charName;
				item["saveFileName"] = e.saveFileName;
				item["classType"]    = e.classType;
				item["classLevel"]   = e.classLevel;
				item["lastUpdate"]   = e.lastUpdate;
				j["characters"].push_back(item);
			}

			std::ofstream ofs(path);
			if (ofs.is_open()) {
				ofs << j.dump(4);
			}
			SKSE::log::info("GlobalCharacterDB saved {} entries.", _entries.size());
		} catch (const std::exception& e) {
			SKSE::log::error("GlobalCharacterDB save error: {}", e.what());
		}
	}

	void GlobalCharacterDB::UpsertCurrentCharacter(const std::string& saveName, uint32_t classType, uint32_t classLevel)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		std::string charName = player ? player->GetName() : "Unknown";

		auto now = std::chrono::system_clock::now();
		auto time_t_now = std::chrono::system_clock::to_time_t(now);
		char timeStr[64];
		std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", std::localtime(&time_t_now));

		bool found = false;
		for (auto& e : _entries) {
			// Update if same character and save name, OR same character (heuristic)
			if (e.charName == charName && e.saveFileName == saveName) {
				e.classType  = classType;
				e.classLevel = classLevel;
				e.lastUpdate = timeStr;
				found = true;
				break;
			}
		}

		if (!found) {
			CharacterSaveEntry e;
			e.charName     = charName;
			e.saveFileName = saveName;
			e.classType    = classType;
			e.classLevel   = classLevel;
			e.lastUpdate   = timeStr;
			_entries.push_back(e);
		}

		SaveDB();
	}
}
