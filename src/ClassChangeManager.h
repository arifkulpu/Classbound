#pragma once
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include "ClassData.h"

namespace Classbound
{
	class ClassChangeManager
	{
	public:
		static ClassChangeManager* GetSingleton()
		{
			static ClassChangeManager singleton;
			return &singleton;
		}

		// ── Cost calculation (based on current class level) ─────────────────
		static uint32_t GetChangeCost(uint32_t classLevel)
		{
			if (classLevel <= 5)  return 10000;
			if (classLevel <= 10) return 25000;
			if (classLevel <= 20) return 50000;
			// Level 20+ scales: 100k + 5k per extra level
			return 100000 + (classLevel - 20) * 5000;
		}

		// ── Trainer access gate ─────────────────────────────────────────────
		bool HasTrainerAccess() const { return _trainerAccessGranted; }
		void GrantTrainerAccess();
		void RevokeTrainerAccess() { _trainerAccessGranted = false; }

		// Check if a given base-form ID belongs to a known class trainer NPC
		static bool IsTrainerNPC(uint32_t baseFormID);

		// ── Gold helpers ────────────────────────────────────────────────────
		int32_t GetPlayerGold() const;
		bool    CanAfford(uint32_t cost) const;
		bool    DeductGold(uint32_t amount);  // returns false if not enough gold

		// ── The actual class change (call after gold deducted) ──────────────
		bool ExecuteClassChange(ClassType newClass);

	private:
		ClassChangeManager() = default;
		~ClassChangeManager() = default;

		bool _trainerAccessGranted{ false };
	};
}
