#include "UniversalSkillManager.h"
#include "ClassManager.h"
#include "EffectManager.h"
#include "FollowerManager.h"
#include <RE/Skyrim.h>
#include <algorithm>
#include <cmath>

namespace Classbound
{
	const std::array<UniversalAttributeDefinition, kUniversalAttributeCount>& UniversalSkillManager::Definitions()
	{
		static const std::array<UniversalAttributeDefinition, kUniversalAttributeCount> definitions{{
			{"health", "Health", "Core", "♥", "Increases maximum health.", "+10 maximum Health per rank"},
			{"magicka", "Magicka", "Core", "✧", "Increases maximum magicka.", "+10 maximum Magicka per rank"},
			{"stamina", "Stamina", "Core", "⚡", "Increases maximum stamina.", "+10 maximum Stamina per rank"},
			{"health_regen", "Health Regeneration", "Regeneration", "✚", "Improves the native health regeneration multiplier.", "+1% regeneration per rank"},
			{"magicka_regen", "Magicka Regeneration", "Regeneration", "◈", "Improves the native magicka regeneration multiplier.", "+1% regeneration per rank"},
			{"stamina_regen", "Stamina Regeneration", "Regeneration", "➤", "Improves the native stamina regeneration multiplier.", "+1% regeneration per rank"},
			{"physical_power", "Physical Power", "Offense", "⚔", "Uses Skyrim's native attack damage multiplier actor value.", "+1% attack damage per rank"},
			{"spell_power", "Spell Power", "Offense", "✦", "Uses Skyrim's native Destruction power modifier actor value.", "+1% Destruction power per rank"},
			{"critical_chance", "Critical Chance", "Offense", "☄", "Raises the native critical chance actor value.", "+0.5 percentage points per rank"},
			{"critical_damage", "Critical Damage", "Offense", "✹", "Skyrim has no stable general critical-damage actor value; this compatible fallback raises attack damage.", "+2% attack damage per rank (applies to all physical hits)"},
			{"defense", "Defense", "Defense", "⬟", "Adds armor rating equivalent to about 1 percentage point of physical mitigation before Skyrim's armor cap.", "+8.33 armor rating per rank"},
			{"elemental_resistance", "Elemental Resistance", "Defense", "❖", "Raises native Fire, Frost, and Shock resistance actor values; Skyrim applies its resistance caps.", "+1 percentage point to Fire/Frost/Shock resistance per rank"},
			{"agility", "Agility", "Utility", "➶", "Raises Skyrim's native movement-speed actor value.", "+0.5% movement speed per rank"},
			{"luck", "Luck", "Utility", "☘", "No native loot-gold multiplier is exposed; this compatible fallback improves barter through Speech.", "+1 Speech per rank (better vendor prices)"},
			{"experience", "Experience Gain", "Utility", "✧", "Increases this character's ClassBound XP rewards, applied once before level progression.", "+1% ClassBound XP per rank"},
			{"carry_capacity", "Carry Capacity", "Utility", "▣", "Increases the amount of weight this character can carry.", "+10 Carry Weight per rank"}
		}};
		return definitions;
	}

	uint32_t UniversalSkillManager::Spent(const UniversalSkillProgress& progress)
	{
		uint32_t total = 0;
		for (const auto rank : progress.ranks) total += rank;
		return total;
	}

	uint32_t UniversalSkillManager::Available(const UniversalSkillProgress& progress)
	{
		const auto spent = Spent(progress);
		const auto total = progress.earnedPoints + progress.bonusPoints;
		return total > spent ? total - spent : 0;
	}

	void UniversalSkillManager::MigrateProgress(UniversalSkillProgress& progress, uint32_t level)
	{
		level = std::clamp(level, 1u, 100u);
		progress.earnedPoints = std::min(level * 3, kMaxUniversalSkillPoints);
		progress.bonusPoints = 0;
		progress.lastAwardedLevel = level;
		progress.ranks.fill(0);
		progress.appliedBonuses.clear();
		progress.bonusesApplied = false;
		progress.pendingRebuild = false;
	}

	void UniversalSkillManager::AwardLevel(UniversalSkillProgress& progress, uint32_t newLevel)
	{
		newLevel = std::clamp(newLevel, 1u, 100u);
		if (newLevel <= progress.lastAwardedLevel) return;
		const uint32_t levels = newLevel - progress.lastAwardedLevel;
		progress.earnedPoints = std::min(kMaxUniversalSkillPoints - progress.bonusPoints, progress.earnedPoints + levels * 3);
		progress.lastAwardedLevel = newLevel;
	}

	void UniversalSkillManager::Validate(UniversalSkillProgress& progress, uint32_t level)
	{
		level = std::clamp(level, 1u, 100u);
		progress.lastAwardedLevel = std::clamp(progress.lastAwardedLevel, 1u, level);
		const uint32_t levelLimit = std::min(progress.lastAwardedLevel * 3, kMaxUniversalSkillPoints);
		progress.earnedPoints = std::min({ progress.earnedPoints, levelLimit, kMaxUniversalSkillPoints });
		progress.bonusPoints = std::min(progress.bonusPoints, kMaxUniversalSkillPoints - progress.earnedPoints);
		uint32_t remaining = progress.earnedPoints + progress.bonusPoints;
		for (auto& rank : progress.ranks) {
			rank = std::min(rank, kMaxUniversalRank);
			if (rank > remaining) rank = remaining;
			remaining -= rank;
		}
		progress.appliedBonuses.erase(std::remove_if(progress.appliedBonuses.begin(), progress.appliedBonuses.end(),
			[](const auto& bonus) { return !std::isfinite(bonus.second) || bonus.second == 0.0f; }), progress.appliedBonuses.end());
	}

	float UniversalSkillManager::XPMultiplier(const UniversalSkillProgress& progress)
	{
		return 1.0f + static_cast<float>(std::min(progress.ranks[14], kMaxUniversalRank)) * 0.01f;
	}

	std::vector<std::pair<uint32_t, float>> UniversalSkillManager::BonusDeltas(uint32_t attributeID, uint32_t ranks)
	{
		ranks = std::min(ranks, kMaxUniversalRank);
		if (ranks == 0 || attributeID >= kUniversalAttributeCount) return {};
		const float count = static_cast<float>(ranks);
		switch (attributeID) {
		case 0: return { {24, 10.0f * count} };
		case 1: return { {25, 10.0f * count} };
		case 2: return { {26, 10.0f * count} };
		case 3: return { {155, 1.0f * count} };
		case 4: return { {156, 1.0f * count} };
		case 5: return { {157, 1.0f * count} };
		case 6: return { {154, 1.0f * count} };
		case 7: return { {149, 1.0f * count} };
		case 8: return { {33, 0.5f * count} };
		case 9: return { {154, 2.0f * count} };
		case 10: return { {39, (100.0f / 12.0f) * count} };
		case 11: return { {41, count}, {43, count}, {42, count} };
		case 12: return { {30, 0.5f * count} };
		case 13: return { {17, count} };
		case 15: return { {32, 10.0f * count} };
		default: return {};
		}
	}

	void UniversalSkillManager::ApplyDeltaToPlayer(uint32_t av, float delta)
	{
		EffectManager::ModifyAVBase(av, delta);
	}

	void UniversalSkillManager::ApplyDeltaToActor(RE::Actor* actor, uint32_t av, float delta)
	{
		if (!actor || delta == 0.0f) return;
		auto* owner = actor->AsActorValueOwner();
		if (!owner) return;
		const auto value = static_cast<RE::ActorValue>(av);
		owner->SetActorValue(value, owner->GetBaseActorValue(value) + delta);
	}

	void UniversalSkillManager::ApplyPlayerRank(uint32_t attributeID, UniversalSkillProgress& progress)
	{
		for (const auto& [av, delta] : BonusDeltas(attributeID, 1)) {
			ApplyDeltaToPlayer(av, delta);
			auto found = std::find_if(progress.appliedBonuses.begin(), progress.appliedBonuses.end(), [av](const auto& item) { return item.first == av; });
			if (found == progress.appliedBonuses.end()) progress.appliedBonuses.emplace_back(av, delta);
			else found->second += delta;
		}
		progress.bonusesApplied = true;
	}

	void UniversalSkillManager::ApplyFollowerRank(uint32_t attributeID, UniversalSkillProgress& progress, RE::Actor* actor)
	{
		for (const auto& [av, delta] : BonusDeltas(attributeID, 1)) {
			ApplyDeltaToActor(actor, av, delta);
			auto found = std::find_if(progress.appliedBonuses.begin(), progress.appliedBonuses.end(), [av](const auto& item) { return item.first == av; });
			if (found == progress.appliedBonuses.end()) progress.appliedBonuses.emplace_back(av, delta);
			else found->second += delta;
		}
		progress.bonusesApplied = true;
		progress.pendingRebuild = false;
	}

	bool UniversalSkillManager::UpgradePlayer(uint32_t attributeID)
	{
		auto& progress = ClassManager::GetSingleton()->GetPlayerData().universalSkills;
		Validate(progress, ClassManager::GetSingleton()->GetPlayerData().classLevel);
		if (attributeID >= kUniversalAttributeCount || progress.ranks[attributeID] >= kMaxUniversalRank || Available(progress) == 0) return false;
		++progress.ranks[attributeID];
		ApplyPlayerRank(attributeID, progress);
		return true;
	}

	bool UniversalSkillManager::ResetPlayer()
	{
		auto& progress = ClassManager::GetSingleton()->GetPlayerData().universalSkills;
		if (Spent(progress) == 0) return false;
		for (const auto& [av, amount] : progress.appliedBonuses) ApplyDeltaToPlayer(av, -amount);
		progress.appliedBonuses.clear();
		progress.ranks.fill(0);
		progress.bonusesApplied = false;
		return true;
	}

	bool UniversalSkillManager::UpgradeFollower(uint32_t formID, uint32_t attributeID)
	{
		auto* data = FollowerManager::GetSingleton()->GetFollowerData(formID);
		if (!data) return false;
		auto& progress = data->universalSkills;
		Validate(progress, data->classLevel);
		if (attributeID >= kUniversalAttributeCount || progress.ranks[attributeID] >= kMaxUniversalRank || Available(progress) == 0) return false;
		auto* actor = data->isActiveFollower ? FollowerManager::GetSingleton()->FindFollowerActor(formID) : nullptr;
		if (actor && !actor->IsDead()) EnsureFollowerBonuses(actor, *data);
		++progress.ranks[attributeID];
		if (actor && !actor->IsDead()) ApplyFollowerRank(attributeID, progress, actor);
		else if (progress.bonusesApplied) progress.pendingRebuild = true;
		return true;
	}

	bool UniversalSkillManager::ResetFollower(uint32_t formID)
	{
		auto* data = FollowerManager::GetSingleton()->GetFollowerData(formID);
		if (!data || Spent(data->universalSkills) == 0) return false;
		auto& progress = data->universalSkills;
		auto* actor = data->isActiveFollower ? FollowerManager::GetSingleton()->FindFollowerActor(formID) : nullptr;
		if (actor && !actor->IsDead()) {
			EnsureFollowerBonuses(actor, *data);
			for (const auto& [av, amount] : progress.appliedBonuses) ApplyDeltaToActor(actor, av, -amount);
			progress.appliedBonuses.clear();
			progress.bonusesApplied = true;
			progress.pendingRebuild = false;
		} else if (progress.bonusesApplied) {
			progress.pendingRebuild = true;
		}
		progress.ranks.fill(0);
		return true;
	}

	uint32_t UniversalSkillManager::GrantTestPoints(uint32_t formID, uint32_t amount)
	{
		if (amount == 0) return 0;
		UniversalSkillProgress* progress = nullptr;
		uint32_t level = 1;
		if (formID == 0) {
			auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
			progress = &playerData.universalSkills;
			level = playerData.classLevel;
		} else if (auto* follower = FollowerManager::GetSingleton()->GetFollowerData(formID)) {
			progress = &follower->universalSkills;
			level = follower->classLevel;
		}
		if (!progress) return 0;

		Validate(*progress, level);
		const uint32_t total = progress->earnedPoints + progress->bonusPoints;
		const uint32_t room = total < kMaxUniversalSkillPoints ? kMaxUniversalSkillPoints - total : 0;
		const uint32_t granted = std::min(amount, room);
		progress->bonusPoints += granted;
		return granted;
	}

	void UniversalSkillManager::EnsureFollowerBonuses(RE::Actor* actor, FollowerClassData& data)
	{
		if (!actor || actor->IsDead()) return;
		auto& progress = data.universalSkills;
		if (progress.pendingRebuild) {
			for (const auto& [av, amount] : progress.appliedBonuses) ApplyDeltaToActor(actor, av, -amount);
			progress.appliedBonuses.clear();
			progress.bonusesApplied = false;
			progress.pendingRebuild = false;
		}
		if (progress.bonusesApplied) return;
		for (uint32_t id = 0; id < kUniversalAttributeCount; ++id) {
			for (const auto& [av, delta] : BonusDeltas(id, progress.ranks[id])) {
				ApplyDeltaToActor(actor, av, delta);
				auto found = std::find_if(progress.appliedBonuses.begin(), progress.appliedBonuses.end(), [av](const auto& item) { return item.first == av; });
				if (found == progress.appliedBonuses.end()) progress.appliedBonuses.emplace_back(av, delta);
				else found->second += delta;
			}
		}
		progress.bonusesApplied = true;
	}
}
