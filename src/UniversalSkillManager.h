#pragma once
#include "ClassData.h"
#include <array>
#include <string_view>
#include <vector>

namespace RE { class Actor; }

namespace Classbound
{
	struct UniversalAttributeDefinition
	{
		const char* id;
		const char* name;
		const char* category;
		const char* icon;
		const char* description;
		const char* effect;
	};

	class UniversalSkillManager
	{
	public:
		static UniversalSkillManager* GetSingleton()
		{
			static UniversalSkillManager singleton;
			return &singleton;
		}

		static const std::array<UniversalAttributeDefinition, kUniversalAttributeCount>& Definitions();
		static uint32_t Spent(const UniversalSkillProgress& progress);
		static uint32_t Available(const UniversalSkillProgress& progress);
		static void MigrateProgress(UniversalSkillProgress& progress, uint32_t level);
		static void AwardLevel(UniversalSkillProgress& progress, uint32_t newLevel);
		static void Validate(UniversalSkillProgress& progress, uint32_t level);
		static float XPMultiplier(const UniversalSkillProgress& progress);

		bool UpgradePlayer(uint32_t attributeID);
		bool ResetPlayer();
		bool UpgradeFollower(uint32_t formID, uint32_t attributeID);
		bool ResetFollower(uint32_t formID);
		uint32_t GrantTestPoints(uint32_t formID, uint32_t amount);
		void EnsureFollowerBonuses(RE::Actor* actor, FollowerClassData& data);

	private:
		UniversalSkillManager() = default;
		static std::vector<std::pair<uint32_t, float>> BonusDeltas(uint32_t attributeID, uint32_t ranks);
		static void ApplyDeltaToPlayer(uint32_t av, float delta);
		static void ApplyDeltaToActor(RE::Actor* actor, uint32_t av, float delta);
		static void ApplyPlayerRank(uint32_t attributeID, UniversalSkillProgress& progress);
		static void ApplyFollowerRank(uint32_t attributeID, UniversalSkillProgress& progress, RE::Actor* actor);
	};
}
