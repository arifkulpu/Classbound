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
		
		bool enableNotifications{ true };
		bool notifyClassXP{ true };
		bool notifyClassLevel{ true };
		bool showDamageNumbers{ false };
		bool confirmClassChange{ true };
		bool automaticClassXP{ true };
		bool limitClassLevelToPlayerLevel{ false };
		bool enableCheatMenu{ false };

		// Keybinding configuration for opening ClassBound menu
		// modifier: 0 = None, 1 = Shift, 2 = Ctrl, 3 = Alt
		// key: DirectX / BSKeyboardDevice scancode (e.g. 0x2F for 'V')
		uint32_t menuModifier{ 1 }; // Default: Shift
		uint32_t menuKey{ 0x2F };    // Default: 'V' (0x2F)


		void LoadSettings();
		void SaveSettings();

	private:
		SettingsManager() = default;
		~SettingsManager() = default;
	};
}
