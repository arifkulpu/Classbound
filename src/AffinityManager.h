#pragma once
#include "ClassData.h"
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

namespace Classbound
{
	struct AffinityProfile
	{
		std::string name{ "" };
		std::string requirementText{ "" };
		std::string effectDescription{ "" };
		std::vector<std::pair<uint32_t, float>> bonusAVs;
	};

	class AffinityManager
	{
	public:
		static AffinityManager* GetSingleton()
		{
			static AffinityManager singleton;
			return &singleton;
		}

		void EvaluateAffinity();
		bool IsAffinityActive() const { return _isAffinityActive; }
		const AffinityProfile& GetCurrentProfile() const { return _currentProfile; }
		const AffinityProfile& GetProfileForClass(ClassType type) const;

		void ClearBonuses();

		// BUG FIX #4: Called by LoadCallback. Marks affinity as having been active WITHOUT
		// touching AV values (which Skyrim's native save already persisted). This prevents
		// EvaluateAffinity() from calling ApplyBonuses() a second time on the first equip event.
		void ResetOnLoad(bool wasActive = false, bool stateKnown = false);

	private:
		AffinityManager() = default;
		~AffinityManager() = default;

		bool _isAffinityActive{ false };
		AffinityProfile _currentProfile;
		std::vector<std::pair<uint32_t, float>> _appliedBonuses;

		bool CheckCriteria(ClassType type);
		void ApplyBonuses(const AffinityProfile& profile);
		void ModifyPlayerAV(uint32_t avIndex, float delta);
	};
}
