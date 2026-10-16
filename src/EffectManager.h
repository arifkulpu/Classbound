#pragma once
#include "ClassData.h"

namespace RE { class Actor; }

namespace Classbound
{
	class EffectManager
	{
	public:
		static EffectManager* GetSingleton()
		{
			static EffectManager singleton;
			return &singleton;
		}

		// Apply all avBonuses from the class definition at the given level.
		// Stores exactly what was applied into playerData.appliedAVBonuses.
		void ApplyClassEffects(ClassType type, uint32_t classLevel = 1);

		// Remove everything stored in playerData.appliedAVBonuses and clear it.
		void RemoveClassEffects(ClassType type);

		// Call on level-up: applies the perLevel bonus increment.
		void ApplyLevelUpBonus(ClassType type, uint32_t newLevel);

		// Re-apply effects after a save load (effects are stored in base AV,
		// this is a no-op but verifies the state).
		void ReApplyEffectsOnLoad(ClassType type, uint32_t classLevel);

		// Brief vanilla shader cue for player and companion level-ups.
		static void PlayLevelUpEffect(RE::Actor* actor);
		static void PlayLevelUpSound();
		static void InitializeAudio();

		// Low-level: directly modify a player AV base value by delta.
		static void ModifyAVBase(uint32_t avIndex, float delta);

	private:
		EffectManager() = default;
		~EffectManager() = default;

		// Compute the total AV bonus for a given AVBonus entry at a given level.
		static float ComputeBonus(const AVBonus& avb, uint32_t level);
	};
}
