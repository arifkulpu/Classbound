#pragma once
#include "ClassData.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <chrono>

namespace RE
{
	class Actor;
}

namespace Classbound
{
	class FollowerManager
	{
	public:
		static FollowerManager* GetSingleton()
		{
			static FollowerManager singleton;
			return &singleton;
		}

		void ScanActiveFollowers(bool force = false);
		static bool IsAnimalActor(RE::Actor* actor);

		std::vector<FollowerClassData>& GetAllFollowers();
		FollowerClassData* GetFollowerData(uint32_t formID);
		RE::Actor* FindFollowerActor(uint32_t formID);

		bool AssignClass(uint32_t formID, ClassType newClass);
		void AddFollowerXP(uint32_t formID, uint32_t amount);
		void GrantPartyCombatXP(uint32_t amount);

		void ApplyFollowerBonuses(RE::Actor* actor, FollowerClassData& data);
		void RemoveFollowerBonuses(RE::Actor* actor, FollowerClassData& data);
		void ReapplyAllFollowerBonuses();
		void ClearFollowers();

		// Optional target-priority directives. Companion combat abilities and formations are not managed here.
		void UpdateFollowerTargeting();

		void SetFollowerFocus(uint32_t formID, TacticalFocus focus);

		// Serialization helpers
		void AddLoadedFollower(const FollowerClassData& data);

	private:
		FollowerManager() = default;
		~FollowerManager() = default;

		std::vector<FollowerClassData> _followers;
		std::chrono::steady_clock::time_point _lastScan{};
		void ModifyActorAV(RE::Actor* actor, uint32_t avIndex, float delta);
	};
}
