#include "XPManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include "EffectManager.h"
#include "SkillManager.h"
#include "SettingsManager.h"
#include "FollowerManager.h"
#include "UniversalSkillManager.h"
#include "PrismaUIManager.h"
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace Classbound
{
	// Route all ClassBound feedback through the non-interactive PrismaUI toast stream.
	static void Notify(const char* msg, const char* category = "xp")
	{
		PrismaUIManager::Notify(msg ? msg : "", category);
	}

	// ── AddXP ─────────────────────────────────────────────────────────────────
	void XPManager::AddXP(uint32_t amount, const char* reason, bool forceNotification)
	{
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		// Do not accumulate or announce progression before the player has chosen
		// a ClassBound class. This also suppresses forced quest XP notifications.
		if (playerData.currentClass == ClassType::None || amount == 0) return;

		auto* settings = SettingsManager::GetSingleton();
		if (!settings->automaticClassXP && !forceNotification) return;

		float mult = settings->xpMultiplier;

		// Shared XP is class-agnostic.
		float scaled = static_cast<float>(amount) * mult;
		uint32_t finalAmount = static_cast<uint32_t>(scaled);
		if (finalAmount == 0) finalAmount = 1;

		auto* followers = FollowerManager::GetSingleton();
		followers->ScanActiveFollowers();
		std::vector<uint32_t> recipients;
		for (const auto& follower : followers->GetAllFollowers()) {
			// ScanActiveFollowers has already refreshed this cached active state (at most every 2s).
			if (follower.isActiveFollower) recipients.push_back(follower.formID);
		}
		const uint32_t partySize = static_cast<uint32_t>(recipients.size()) + 1;
		const uint32_t eachShare = finalAmount / partySize;
		const uint32_t remainder = finalAmount % partySize;
		const uint32_t playerShare = static_cast<uint32_t>(std::lround(static_cast<float>(eachShare + remainder) * UniversalSkillManager::XPMultiplier(playerData.universalSkills)));
		playerData.classXP += playerShare;
		for (const auto formID : recipients) {
			auto* follower = followers->GetFollowerData(formID);
			const float followerMultiplier = follower ? UniversalSkillManager::XPMultiplier(follower->universalSkills) : 1.0f;
			followers->AddFollowerXP(formID, static_cast<uint32_t>(std::lround(static_cast<float>(eachShare) * followerMultiplier)));
		}
		SKSE::log::info("Shared XP reward {} (player {}, {} followers) | player total {}",
			finalAmount, playerShare, recipients.size(), playerData.classXP);

		// XP notification
		if (settings->notifyClassXP || forceNotification) {
			char msg[128];
			if (reason && reason[0]) snprintf(msg, sizeof(msg), "%s: +%u XP", reason, playerShare);
			else snprintf(msg, sizeof(msg), "Shared XP +%u", playerShare);
			Notify(msg);
		}

		CheckLevelUp();
	}

	uint32_t XPManager::GrantTestXP(uint32_t targetFormID, uint32_t amount)
	{
		if (amount == 0) return 0;
		if (targetFormID != 0) {
			auto* follower = FollowerManager::GetSingleton()->GetFollowerData(targetFormID);
			if (!follower || follower->classLevel >= 100) return 0;
			const uint32_t granted = std::min(amount, std::numeric_limits<uint32_t>::max() - follower->classXP);
			FollowerManager::GetSingleton()->AddFollowerXP(targetFormID, granted);
			char message[128];
			snprintf(message, sizeof(message), "Test XP: +%u to %s", granted, follower->name.c_str());
			Notify(message, "success");
			return granted;
		}

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		if (playerData.classLevel >= 100) return 0;
		const uint32_t granted = std::min(amount, std::numeric_limits<uint32_t>::max() - playerData.classXP);
		playerData.classXP += granted;
		char message[96];
		snprintf(message, sizeof(message), "Test XP: +%u", granted);
		Notify(message, "success");
		CheckLevelUp();
		return granted;
	}

	uint32_t XPManager::GrantTestLevels(uint32_t targetFormID, uint32_t amount)
	{
		if (amount == 0) return 0;
		if (targetFormID != 0) {
			auto* follower = FollowerManager::GetSingleton()->GetFollowerData(targetFormID);
			if (!follower || follower->classLevel >= 100) return 0;
			const uint32_t levels = std::min(amount, 100u - follower->classLevel);
			uint64_t required = 0;
			for (uint32_t level = follower->classLevel; level < follower->classLevel + levels; ++level)
				required += GetXPForNextLevel(level);
			const uint64_t needed = required > follower->classXP ? required - follower->classXP : 0;
			const uint32_t xp = static_cast<uint32_t>((std::min)(needed, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max() - follower->classXP)));
			const auto startingLevel = follower->classLevel;
			FollowerManager::GetSingleton()->AddFollowerXP(targetFormID, xp);
			return follower->classLevel - startingLevel;
		}

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		if (playerData.classLevel >= 100) return 0;
		const uint32_t levels = std::min(amount, 100u - playerData.classLevel);
		const uint32_t startingLevel = playerData.classLevel;
		for (uint32_t i = 0; i < levels; ++i) {
			++playerData.classLevel;
			UniversalSkillManager::AwardLevel(playerData.universalSkills, playerData.classLevel);
			if (playerData.currentClass != ClassType::None)
				EffectManager::GetSingleton()->ApplyLevelUpBonus(playerData.currentClass, playerData.classLevel);
		}
		const uint32_t gained = playerData.classLevel - startingLevel;
		if (gained > 0) {
			if (playerData.classLevel >= 100) playerData.classXP = 0;
			EffectManager::PlayLevelUpEffect(RE::PlayerCharacter::GetSingleton());
			EffectManager::PlayLevelUpSound();
			char message[128];
			snprintf(message, sizeof(message), "TEST LEVEL UP! Level %u · +%u skill points", playerData.classLevel, gained * 3);
			Notify(message, "level");
		}
		return gained;
	}

	// ── GetXPForNextLevel ──────────────────────────────────────────────────────
	uint32_t XPManager::GetXPForNextLevel(uint32_t currentLevel) const
	{
		// Max level is 100, if at max level, return a very high unreachable number or 0
		if (currentLevel >= 100) return 9999999;
		
		// Curve: 100 * level^1.5  (level 1→100, 2→283, 5→1118, 10→3162... 99->98503)
		// This achieves what the user asked: starts at 100 and scales up per level.
		return static_cast<uint32_t>(100.0 * std::pow(static_cast<double>(currentLevel), 1.5));
	}

	// ── CheckLevelUp ──────────────────────────────────────────────────────────
	void XPManager::CheckLevelUp()
	{
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		auto* settings = SettingsManager::GetSingleton();
		bool  leveledUp = false;
		const uint32_t startingLevel = playerData.classLevel;

		uint32_t requiredXP = GetXPForNextLevel(playerData.classLevel);

		uint32_t maxAllowedLevel = 100;
		if (settings->limitClassLevelToPlayerLevel) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				maxAllowedLevel = player->GetLevel();
			}
		}

		while (playerData.classLevel < maxAllowedLevel && playerData.classXP >= requiredXP) {
			playerData.classXP -= requiredXP;
			playerData.classLevel++;
			leveledUp = true;
			UniversalSkillManager::AwardLevel(playerData.universalSkills, playerData.classLevel);

			// Level-up stat bonus (every 5 levels)
			if (playerData.currentClass != ClassType::None) {
				EffectManager::GetSingleton()->ApplyLevelUpBonus(playerData.currentClass, playerData.classLevel);
			}

			SKSE::log::info("Class Level Up! Level: {} | XP left: {} | Next: {}",
				playerData.classLevel, playerData.classXP,
				GetXPForNextLevel(playerData.classLevel));

			requiredXP = GetXPForNextLevel(playerData.classLevel);
		}

		// Cap XP if max level reached
		if (playerData.classLevel >= 100) {
			playerData.classXP = 0;
		}

		if (leveledUp) {
			EffectManager::PlayLevelUpEffect(RE::PlayerCharacter::GetSingleton());
			EffectManager::PlayLevelUpSound();
		}

		if (leveledUp && settings->notifyClassLevel) {
			// Get class name for the message
			auto defOpt = ClassRegistry::GetSingleton()->GetClass(playerData.currentClass);
			const char* className = defOpt ? defOpt->name : "Adventurer";

			char msg[128];
			const uint32_t levelsGained = playerData.classLevel - startingLevel;
			snprintf(msg, sizeof(msg), "LEVEL UP! %s · Level %u · +%u skill points", className, playerData.classLevel, levelsGained * 3);
			Notify(msg, "level");

		}
	}
}
