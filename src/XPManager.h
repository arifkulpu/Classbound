#pragma once
#include <cstdint>
#include "ClassData.h"

namespace Classbound
{
	class XPManager
	{
	public:
		static XPManager* GetSingleton()
		{
			static XPManager singleton;
			return &singleton;
		}

		// amount is the party's total reward; it is divided equally among the player
		// and active ClassBound followers. A small remainder stays with the player.
		void AddXP(uint32_t amount, const char* reason = nullptr, bool forceNotification = false);
		uint32_t GrantTestXP(uint32_t targetFormID, uint32_t amount);
		uint32_t GrantTestLevels(uint32_t targetFormID, uint32_t amount);
		uint32_t GetXPForNextLevel(uint32_t currentLevel) const;
		void CheckLevelUp();

	private:
		XPManager() = default;
		~XPManager() = default;
	};
}
