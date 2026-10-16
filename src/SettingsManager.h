#pragma once
#include <cstdint>

namespace Classbound
{
	class SettingsManager
	{
	public:
		static SettingsManager* GetSingleton()
		{
			static SettingsManager singleton;
			return &singleton;
		}

		// Settings variables
		float xpMultiplier{ 1.0f };
		
		bool notifyClassXP{ true };
		bool notifyClassLevel{ true };
		bool showDamageNumbers{ false };
		bool confirmClassChange{ true };
		bool automaticClassXP{ true };
		bool limitClassLevelToPlayerLevel{ false };
		bool enableCheatMenu{ false };


		void LoadSettings();
		void SaveSettings();

	private:
		SettingsManager() = default;
		~SettingsManager() = default;
	};
}
