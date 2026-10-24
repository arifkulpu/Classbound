#include "SettingsManager.h"
#include <SKSE/SKSE.h>
#include <fstream>
#include <sstream>

namespace Classbound
{
	void SettingsManager::LoadSettings()
	{
		std::ifstream file("Data/SKSE/Plugins/Classbound.ini");
		if (!file.is_open()) {
			SaveSettings(); // Create default
			return;
		}

		std::string line;
		while (std::getline(file, line)) {
			if (line.empty() || line[0] == ';' || line[0] == '#') continue;
			
			auto delimPos = line.find('=');
			if (delimPos != std::string::npos) {
				std::string key = line.substr(0, delimPos);
				std::string value = line.substr(delimPos + 1);

				key.erase(key.find_last_not_of(" \t\r\n") + 1);
				value.erase(0, value.find_first_not_of(" \t\r\n"));

				// Minor Fix: stof/stoul throw on malformed values — wrap with try/catch to
				// prevent a crash at mod startup from a corrupted or hand-edited INI file.
				try {
					if (key == "fXPMultiplier") xpMultiplier = std::stof(value);
					else if (key == "bEnableNotifications") enableNotifications = (value == "true" || value == "1");
					else if (key == "bNotifyClassXP") notifyClassXP = (value == "true" || value == "1");
					else if (key == "bNotifyClassLevel") notifyClassLevel = (value == "true" || value == "1");
					else if (key == "bShowDamageNumbers") showDamageNumbers = (value == "true" || value == "1");
					else if (key == "bConfirmClassChange") confirmClassChange = (value == "true" || value == "1");
					else if (key == "bAutomaticClassXP") automaticClassXP = (value == "true" || value == "1");
					else if (key == "bLimitClassLevelToPlayerLevel") limitClassLevelToPlayerLevel = (value == "true" || value == "1");
					else if (key == "bEnableCheatMenu") enableCheatMenu = (value == "true" || value == "1");
					else if (key == "uMenuModifier") menuModifier = static_cast<uint32_t>(std::stoul(value));
					else if (key == "uMenuKey") menuKey = static_cast<uint32_t>(std::stoul(value));
				} catch (const std::exception& ex) {
					SKSE::log::warn("SettingsManager: Failed to parse key '{}' = '{}': {}", key, value, ex.what());
				}
			}
		}
		
		SKSE::log::info("Settings loaded.");
	}

	void SettingsManager::SaveSettings()
	{
		std::ofstream file("Data/SKSE/Plugins/Classbound.ini");
		if (file.is_open()) {
			file << "[Main]\n";
			file << "fXPMultiplier=" << xpMultiplier << "\n";
			file << "bEnableNotifications=" << (enableNotifications ? "true" : "false") << "\n";
			file << "bNotifyClassXP=" << (notifyClassXP ? "true" : "false") << "\n";
			file << "bNotifyClassLevel=" << (notifyClassLevel ? "true" : "false") << "\n";
			file << "bShowDamageNumbers=" << (showDamageNumbers ? "true" : "false") << "\n";
			file << "bConfirmClassChange=" << (confirmClassChange ? "true" : "false") << "\n";
			file << "bAutomaticClassXP=" << (automaticClassXP ? "true" : "false") << "\n";
			file << "bLimitClassLevelToPlayerLevel=" << (limitClassLevelToPlayerLevel ? "true" : "false") << "\n";
			file << "bEnableCheatMenu=" << (enableCheatMenu ? "true" : "false") << "\n";
			file << "uMenuModifier=" << menuModifier << "\n";
			file << "uMenuKey=" << menuKey << "\n";
			SKSE::log::info("Settings saved.");
		}
	}
}
