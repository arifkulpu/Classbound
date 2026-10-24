#pragma once
#include "ClassData.h"
#include <vector>
#include <string>

namespace RE
{
	class Actor;
}

namespace Classbound
{
	struct StartingItem
	{
		uint32_t formID;
		uint32_t count;
		const char* name;
		const char* sourcePlugin{ nullptr }; // nullptr means Skyrim.esm / base game
		const char* fallbackPlugin{ nullptr };
		uint32_t fallbackFormID{ 0 };
	};

	class StartingGearManager
	{
	public:
		static StartingGearManager* GetSingleton()
		{
			static StartingGearManager singleton;
			return &singleton;
		}

		void GrantStartingGear(ClassType classType);
		void GrantStartingGearToActor(RE::Actor* actor, ClassType classType);

	private:
		StartingGearManager() = default;
		~StartingGearManager() = default;

		std::vector<StartingItem> GetGearForClass(ClassType type) const;
		void AddItemToActor(RE::Actor* actor, uint32_t formID, uint32_t count, const char* sourcePlugin = nullptr, const char* fallbackPlugin = nullptr, uint32_t fallbackFormID = 0);
		void AddSpellToActor(RE::Actor* actor, uint32_t spellFormID);
		void SpawnHorseForPlayer();
	};
}
