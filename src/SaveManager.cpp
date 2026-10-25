#include "SaveManager.h"
#include "ClassManager.h"
#include "SkillManager.h"
#include "EffectManager.h"
#include "ClassRegistry.h"
#include "ClassChangeManager.h"
#include "FollowerManager.h"
#include "GlobalCharacterDB.h"
#include "AffinityManager.h"
#include "UniversalSkillManager.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace Classbound
{
	namespace
	{
		void WriteUniversalProgress(SKSE::SerializationInterface* io, const UniversalSkillProgress& source)
		{
			auto progress = source;
			UniversalSkillManager::Validate(progress, 100);
			io->WriteRecordData(progress.earnedPoints);
			io->WriteRecordData(progress.bonusPoints);
			io->WriteRecordData(progress.lastAwardedLevel);
			for (auto rank : progress.ranks) io->WriteRecordData(rank);
			size_t count = progress.appliedBonuses.size(); io->WriteRecordData(count);
			for (const auto& [av, amount] : progress.appliedBonuses) { io->WriteRecordData(av); io->WriteRecordData(amount); }
			io->WriteRecordData(progress.bonusesApplied);
			io->WriteRecordData(progress.pendingRebuild);
		}

		void ReadUniversalProgress(SKSE::SerializationInterface* io, UniversalSkillProgress& progress, uint32_t level, uint32_t version)
		{
			io->ReadRecordData(progress.earnedPoints);
			if (version >= 13) io->ReadRecordData(progress.bonusPoints);
			else progress.bonusPoints = 0;
			io->ReadRecordData(progress.lastAwardedLevel);
			size_t savedRankCount = kUniversalAttributeCount;
			if (version < 12) savedRankCount = 15;
			else if (version < 15) savedRankCount = 16;
			for (size_t i = 0; i < savedRankCount; ++i) io->ReadRecordData(progress.ranks[i]);
			for (size_t i = savedRankCount; i < kUniversalAttributeCount; ++i) progress.ranks[i] = 0;
			size_t count = 0; io->ReadRecordData(count);
			count = std::min<size_t>(count, 64);
			progress.appliedBonuses.clear();
			for (size_t i = 0; i < count; ++i) { uint32_t av=0; float amount=0; io->ReadRecordData(av); io->ReadRecordData(amount); progress.appliedBonuses.emplace_back(av, amount); }
			io->ReadRecordData(progress.bonusesApplied);
			io->ReadRecordData(progress.pendingRebuild);
			UniversalSkillManager::Validate(progress, level);
		}
	}

	void SaveManager::Register()
	{
		auto serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID('CLBD');
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);
		serialization->SetRevertCallback(RevertCallback);
		SKSE::log::info("Registered SKSE Serialization Callbacks.");
	}

	void SaveManager::SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		SKSE::log::info("Saving Classbound data...");

		if (!a_intfc->OpenRecord(kClassDataRecord, kSerializationVersion)) {
			SKSE::log::error("Failed to open record for saving!");
			return;
		}

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

		// Save currentClass
		uint32_t classType = static_cast<uint32_t>(playerData.currentClass);
		a_intfc->WriteRecordData(classType);

		// Save classLevel
		a_intfc->WriteRecordData(playerData.classLevel);

		// Save classXP
		a_intfc->WriteRecordData(playerData.classXP);
		WriteUniversalProgress(a_intfc, playerData.universalSkills);

		
		// Save stats (v5)
		a_intfc->WriteRecordData(playerData.stats.enemiesKilled);
		a_intfc->WriteRecordData(playerData.stats.meleeKills);
		a_intfc->WriteRecordData(playerData.stats.archeryKills);
		a_intfc->WriteRecordData(playerData.stats.magicKills);
		a_intfc->WriteRecordData(playerData.stats.sneakKills);
		a_intfc->WriteRecordData(playerData.stats.bossesDefeated);
		a_intfc->WriteRecordData(playerData.stats.spellsCast);
		a_intfc->WriteRecordData(playerData.stats.itemsSold);
		a_intfc->WriteRecordData(playerData.stats.itemsCrafted);
		a_intfc->WriteRecordData(playerData.stats.locksPicked);
		a_intfc->WriteRecordData(playerData.stats.dungeonsCleared);

		// The class-specific skill point tree was removed from serialization.

		// BUG FIX #3: Save active tempBuffs so expired buffs don't become permanent on reload.
		// Temp buffs written to SetActorValue are persisted by Skyrim's native save — we must
		// expire them on reload if their time has elapsed, or restore them if still active.
		auto& tempBuffs = SkillManager::GetSingleton()->GetTempBuffs();
		size_t tempBuffCount = tempBuffs.size();
		a_intfc->WriteRecordData(tempBuffCount);
		for (const auto& buff : tempBuffs) {
			a_intfc->WriteRecordData(buff.avIndex);
			a_intfc->WriteRecordData(buff.amount);
			a_intfc->WriteRecordData(buff.expireTime);
		}

		// v8: track permanent class bonuses already present in the native save.
		size_t appliedAVCount = playerData.appliedAVBonuses.size();
		a_intfc->WriteRecordData(appliedAVCount);
		for (const auto& [av, amount] : playerData.appliedAVBonuses) {
			a_intfc->WriteRecordData(av);
			a_intfc->WriteRecordData(amount);
		}
		bool affinityActive = AffinityManager::GetSingleton()->IsAffinityActive();
		a_intfc->WriteRecordData(affinityActive);

		// Save Followers Record
		if (!a_intfc->OpenRecord(kFollowerDataRecord, kSerializationVersion)) {
			SKSE::log::error("Failed to open follower record for saving!");
			return;
		}
		auto& followers = FollowerManager::GetSingleton()->GetAllFollowers();
		size_t followersCount = followers.size();
		a_intfc->WriteRecordData(followersCount);
		for (const auto& f : followers) {
			a_intfc->WriteRecordData(f.formID);
			size_t nameLen = f.name.size();
			a_intfc->WriteRecordData(nameLen);
			if (nameLen > 0) {
				a_intfc->WriteRecordData(f.name.data(), static_cast<uint32_t>(nameLen));
			}
			uint32_t cType = static_cast<uint32_t>(f.currentClass);
			a_intfc->WriteRecordData(cType);
			a_intfc->WriteRecordData(f.classLevel);
			a_intfc->WriteRecordData(f.classXP);
			WriteUniversalProgress(a_intfc, f.universalSkills);
			size_t appliedCount = f.appliedAVBonuses.size();
			a_intfc->WriteRecordData(appliedCount);
			for (const auto& [av, amount] : f.appliedAVBonuses) {
				a_intfc->WriteRecordData(av);
				a_intfc->WriteRecordData(amount);
			}
			uint32_t fFocus = static_cast<uint32_t>(f.tacticalFocus);
			// Reserved for the removed follower formation setting; keep the serialized record layout stable.
			uint32_t fPos = 0;
			a_intfc->WriteRecordData(fFocus);
			a_intfc->WriteRecordData(fPos);
		}

		// Update Global Character DB
		// SKSE doesn't expose the *exact* save file name string easily here without hooking internal Skyrim logic.
		// As a fallback for the save file name, we'll just use a generic "Save" label or let the DB index by char name.
		// A common trick is to use "Save" + character name.
		std::string saveName = "Save - " + std::string(RE::PlayerCharacter::GetSingleton()->GetName());
		GlobalCharacterDB::GetSingleton()->UpsertCurrentCharacter(saveName, classType, playerData.classLevel);

		SKSE::log::info("Classbound data saved successfully.");
	}

	void SaveManager::LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		SKSE::log::info("Loading Classbound data...");
		uint32_t type;
		uint32_t version;
		uint32_t length;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version > kSerializationVersion || version < 1) {
				SKSE::log::error("Failed to load data! Unknown version ({})", version);
				continue;
			}

			if (type == kClassDataRecord) {
				auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

				uint32_t classType;
				a_intfc->ReadRecordData(classType);
				playerData.currentClass = static_cast<ClassType>(classType);

				a_intfc->ReadRecordData(playerData.classLevel);
				a_intfc->ReadRecordData(playerData.classXP);
				if (version >= 11) ReadUniversalProgress(a_intfc, playerData.universalSkills, playerData.classLevel, version);
				else UniversalSkillManager::MigrateProgress(playerData.universalSkills, playerData.classLevel);
				uint32_t legacySkillPoints = 0;
				if (version <= 9) a_intfc->ReadRecordData(legacySkillPoints);

				if (version <= 8) {
					size_t specLength = 0; a_intfc->ReadRecordData(specLength);
					std::string legacySpec(specLength, '\0');
					if (specLength) a_intfc->ReadRecordData(legacySpec.data(), static_cast<uint32_t>(specLength));
				}

				// Legacy skillsCount in version < 7
				if (version < 7) {
					size_t skillsCount = 0;
					a_intfc->ReadRecordData(skillsCount);
					for (size_t i = 0; i < skillsCount; ++i) {
						uint32_t skillId;
						a_intfc->ReadRecordData(skillId);
					}
				}

				if (version >= 5) {
					a_intfc->ReadRecordData(playerData.stats.enemiesKilled);
					a_intfc->ReadRecordData(playerData.stats.meleeKills);
					a_intfc->ReadRecordData(playerData.stats.archeryKills);
					a_intfc->ReadRecordData(playerData.stats.magicKills);
					a_intfc->ReadRecordData(playerData.stats.sneakKills);
					a_intfc->ReadRecordData(playerData.stats.bossesDefeated);
					a_intfc->ReadRecordData(playerData.stats.spellsCast);
					a_intfc->ReadRecordData(playerData.stats.itemsSold);
					a_intfc->ReadRecordData(playerData.stats.itemsCrafted);
					a_intfc->ReadRecordData(playerData.stats.locksPicked);
					a_intfc->ReadRecordData(playerData.stats.dungeonsCleared);
				} else if (version >= 2) {
					a_intfc->ReadRecordData(playerData.stats.enemiesKilled);
					uint32_t legacyTwoHanded = 0;
					a_intfc->ReadRecordData(legacyTwoHanded);
					playerData.stats.meleeKills = legacyTwoHanded;
					a_intfc->ReadRecordData(playerData.stats.bossesDefeated);
					a_intfc->ReadRecordData(playerData.stats.spellsCast);
					a_intfc->ReadRecordData(playerData.stats.itemsSold);
				} else {
					playerData.stats = PlayerStatistics{};
				}

				std::vector<uint32_t> legacyClassRanks;
				if (version >= 3 && version <= 9) {
					size_t upgradesCount = 0; a_intfc->ReadRecordData(upgradesCount);
					legacyClassRanks.resize(upgradesCount);
					for (auto& rank : legacyClassRanks) a_intfc->ReadRecordData(rank);

					// Legacy cooldowns in version < 7
					if (version < 7) {
						size_t cooldownsCount = 0;
						a_intfc->ReadRecordData(cooldownsCount);
						for (size_t i = 0; i < cooldownsCount; ++i) {
							uint32_t id;
							uint64_t ts;
							a_intfc->ReadRecordData(id);
							a_intfc->ReadRecordData(ts);
						}
					}
				}

				if (version >= 4 && version <= 8) { size_t n = 0; a_intfc->ReadRecordData(n); for (size_t i=0;i<n;++i) { uint32_t ignoredRank=0; a_intfc->ReadRecordData(ignoredRank); } }

				// Discard and reverse old specialization AV changes from saved Skyrim native AVs.
				if (version >= 6 && version <= 8) { size_t n=0; a_intfc->ReadRecordData(n); for(size_t i=0;i<n;++i){uint32_t av=0; float val=0; a_intfc->ReadRecordData(av); a_intfc->ReadRecordData(val); EffectManager::ModifyAVBase(av,-val);} }
				if (version >= 6) {

					// BUG FIX #3: Restore tempBuffs — expire ones that have already timed out.
					// Skyrim's native save already has the AV value written, so expired buffs
					// that we don't find here would otherwise be permanent.
					size_t tempBuffCount = 0;
					a_intfc->ReadRecordData(tempBuffCount);
					auto& tempBuffs = SkillManager::GetSingleton()->GetTempBuffs();
					tempBuffs.clear();
					double nowSec = SkillManager::GetSingleton()->NowSecondsPublic();
					for (size_t i = 0; i < tempBuffCount; ++i) {
						SkillManager::TempBuff buff;
						a_intfc->ReadRecordData(buff.avIndex);
						a_intfc->ReadRecordData(buff.amount);
						a_intfc->ReadRecordData(buff.expireTime);
						if (buff.expireTime <= nowSec) {
							// Already expired while we were offline — revert it now
							SkillManager::GetSingleton()->ExpireBuffPublic(buff);
							SKSE::log::info("LoadCallback: Expired stale TempBuff (av={}, amount={})", buff.avIndex, buff.amount);
						} else {
							// Still valid — restore to tracking list
							tempBuffs.push_back(buff);
						}
					}
				}

				playerData.appliedAVBonuses.clear();
				bool affinityWasActive = false;
				if (version >= 8) {
					size_t appliedCount = 0;
					a_intfc->ReadRecordData(appliedCount);
					for (size_t i = 0; i < appliedCount; ++i) {
						uint32_t av = 0;
						float amount = 0.0f;
						a_intfc->ReadRecordData(av);
						a_intfc->ReadRecordData(amount);
						playerData.appliedAVBonuses.emplace_back(av, amount);
					}
					a_intfc->ReadRecordData(affinityWasActive);
				}

				SKSE::log::info("Loaded class data: Class {}, Level {}", classType, playerData.classLevel);

				// Before v8, native actor values contained class-tree upgrades but saves did not
				// track their exact applied total. Rebuild only the removal bookkeeping.
				if (version < 8 && !legacyClassRanks.empty()) {
					if (auto legacyClass = ClassRegistry::GetSingleton()->GetClass(playerData.currentClass)) {
						playerData.appliedAVBonuses.clear();
						const uint32_t tiers = playerData.classLevel / 5;
						for (size_t i = 0; i < legacyClass->avBonuses.size(); ++i) {
							const auto& bonus = legacyClass->avBonuses[i];
							float amount = bonus.base + bonus.perLevel * static_cast<float>(tiers);
							if (i < legacyClassRanks.size()) amount += bonus.perLevel * static_cast<float>(legacyClassRanks[i]);
							if (amount != 0.0f) playerData.appliedAVBonuses.emplace_back(bonus.avIndex, amount);
						}
					}
				}

				// BUG FIX #4: Reset affinity state on load WITHOUT touching AV values.
				// On load, Skyrim's native save already has any active affinity bonus baked in.
				// _isAffinityActive starts as false, so the next TESEquipEvent would call
				// ApplyBonuses() again → double-apply. We mark it active here so EvaluateAffinity
				// skips re-applying if the criteria are still met.
				AffinityManager::GetSingleton()->ResetOnLoad(affinityWasActive, version >= 8);

				// Re-apply effects on load
				EffectManager::GetSingleton()->ReApplyEffectsOnLoad(playerData.currentClass, playerData.classLevel);

				// NOTE: Trainer access (_trainerAccessGranted) is intentionally session-only / dialogue-scoped
				// and is NOT serialized across saves. The player must speak with an official trainer in-game.
				ClassChangeManager::GetSingleton()->RevokeTrainerAccess();
			} else if (type == kFollowerDataRecord) {
				size_t followersCount = 0;
				a_intfc->ReadRecordData(followersCount);
				for (size_t i = 0; i < followersCount; ++i) {
					FollowerClassData f;
					uint32_t savedFormID = 0;
					a_intfc->ReadRecordData(savedFormID);
					size_t nameLen = 0;
					a_intfc->ReadRecordData(nameLen);
					if (nameLen > 0) {
						f.name.resize(nameLen);
						a_intfc->ReadRecordData(f.name.data(), static_cast<uint32_t>(nameLen));
					}
					uint32_t cType = 0;
					a_intfc->ReadRecordData(cType);
					f.currentClass = static_cast<ClassType>(cType);
					a_intfc->ReadRecordData(f.classLevel);
					a_intfc->ReadRecordData(f.classXP);
					if (version >= 11) ReadUniversalProgress(a_intfc, f.universalSkills, f.classLevel, version);
					else UniversalSkillManager::MigrateProgress(f.universalSkills, f.classLevel);
					if (version >= 8) {
						size_t appliedCount = 0;
						a_intfc->ReadRecordData(appliedCount);
						for (size_t j = 0; j < appliedCount; ++j) {
							uint32_t av = 0;
							float amount = 0.0f;
							a_intfc->ReadRecordData(av);
							a_intfc->ReadRecordData(amount);
							f.appliedAVBonuses.emplace_back(av, amount);
						}
					}
					if (version >= 14) {
						uint32_t fFocus = 0, fPos = 0;
						a_intfc->ReadRecordData(fFocus);
						a_intfc->ReadRecordData(fPos);
						f.tacticalFocus = static_cast<TacticalFocus>(fFocus);
						(void)fPos;
					}

					uint32_t resolvedFormID = 0;
					if (a_intfc->ResolveFormID(savedFormID, resolvedFormID) && resolvedFormID != 0) {
						f.formID = resolvedFormID;
						FollowerManager::GetSingleton()->AddLoadedFollower(f);
					} else {
						SKSE::log::warn("SaveManager: Follower form {:08X} ({}) could not be resolved in current load order. Skipping.", savedFormID, f.name);
					}
				}
				FollowerManager::GetSingleton()->ReapplyAllFollowerBonuses();
				SKSE::log::info("Loaded {} follower class records.", followersCount);
			}
		}
	}

	void SaveManager::RevertCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		SKSE::log::info("Reverting Classbound data to default state...");
		
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		playerData.currentClass = ClassType::None;
		playerData.classLevel = 1;
		playerData.classXP = 0;
		playerData.universalSkills = UniversalSkillProgress{};
		playerData.stats = PlayerStatistics{};
		playerData.appliedAVBonuses.clear();

		// Clear follower records
		FollowerManager::GetSingleton()->ClearFollowers();
		AffinityManager::GetSingleton()->ClearBonuses();

		// Ensure trainer access is revoked on game load / revert
		ClassChangeManager::GetSingleton()->RevokeTrainerAccess();
	}
}
