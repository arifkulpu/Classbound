#include "FollowerManager.h"
#include "ClassRegistry.h"
#include "XPManager.h"
#include "SettingsManager.h"
#include "ClassChangeManager.h"
#include "StartingGearManager.h"
#include "KingCoreAdapter.h"
#include "UniversalSkillManager.h"
#include "EffectManager.h"
#include "PrismaUIManager.h"
#include <unordered_set>
#include <RE/Skyrim.h>
#include <RE/P/ProcessLists.h>
#include <SKSE/SKSE.h>

namespace Classbound
{
	bool FollowerManager::IsAnimalActor(RE::Actor* actor)
	{
		if (!actor) return false;

		// 1. Explicit keyword checks on Actor
		if (actor->HasKeywordString("ActorTypeAnimal") || actor->HasKeywordString("ActorTypeCreature")) {
			return true;
		}

		// 2. Keyword check on ActorBase
		auto* base = actor->GetActorBase();
		if (base) {
			if (base->HasKeywordString("ActorTypeAnimal") || base->HasKeywordString("ActorTypeCreature") ||
				base->HasKeywordID(0x00013795) || base->HasKeywordID(0x00013796)) {
				return true;
			}
		}

		auto* race = actor->GetRace();
		if (!race) return false;

		// 3. Keyword check on Race
		if (race->HasKeywordString("ActorTypeAnimal") || race->HasKeywordString("ActorTypeCreature") ||
			race->HasKeywordID(0x00013795) || race->HasKeywordID(0x00013796)) {
			return true;
		}

		// 4. Animal race flags: CantOpenDoors flag is standard on animals/creatures
		if (race->data.flags.all(RE::RACE_DATA::Flag::kCantOpenDoors)) {
			return true;
		}

		// 5. Name & EditorID pattern matching
		static std::unordered_set<RE::FormID> s_knownAnimalRaces;
		static std::unordered_set<RE::FormID> s_knownHumanoidRaces;

		RE::FormID raceID = race->GetFormID();
		if (s_knownAnimalRaces.find(raceID) != s_knownAnimalRaces.end()) {
			return true;
		}
		if (s_knownHumanoidRaces.find(raceID) != s_knownHumanoidRaces.end()) {
			return false;
		}

		std::string raceName = race->GetName();
		std::transform(raceName.begin(), raceName.end(), raceName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		std::string raceEdID = race->GetFormEditorID();
		std::transform(raceEdID.begin(), raceEdID.end(), raceEdID.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		static const std::vector<std::string> kAnimalKeywords = {
			"cat", "dog", "horse", "wolf", "fox", "bear", "cow", "goat", "chicken",
			"skeever", "spider", "mudcrab", "troll", "sabrecat", "deer", "elk", "rabbit",
			"chaurus", "dragon", "creature", "canine", "feline", "animal", "boar", "horker",
			"mammoth", "slaughterfish", "husky", "deathhound", "atronach", "zombie", "familiar",
			"gargoyle", "wisp", "pet"
		};

		for (const auto& kw : kAnimalKeywords) {
			if (!raceName.empty() && raceName.find(kw) != std::string::npos && raceName.find("khajiit") == std::string::npos) {
				s_knownAnimalRaces.insert(raceID);
				return true;
			}
			if (!raceEdID.empty() && raceEdID.find(kw) != std::string::npos && raceEdID.find("khajiit") == std::string::npos) {
				s_knownAnimalRaces.insert(raceID);
				return true;
			}
		}

		// Check ActorBase EditorID (e.g. ACLCatOwnedAnise)
		if (base) {
			std::string baseEdID = base->GetFormEditorID();
			std::transform(baseEdID.begin(), baseEdID.end(), baseEdID.begin(), ::tolower);
			for (const auto& kw : kAnimalKeywords) {
				if (!baseEdID.empty() && baseEdID.find(kw) != std::string::npos && baseEdID.find("khajiit") == std::string::npos) {
					return true;
				}
			}
		}

		// 6. Humanoid check: FaceGenHead is present on all humanoid races (including Khajiit, Argonian, Elves, Men)
		bool hasFaceGen = race->data.flags.all(RE::RACE_DATA::Flag::kFaceGenHead);
		if (!hasFaceGen) {
			s_knownAnimalRaces.insert(raceID);
			return true;
		}

		s_knownHumanoidRaces.insert(raceID);
		return false;
	}

	void FollowerManager::ScanActiveFollowers(bool force)
	{
		auto scanNow = std::chrono::steady_clock::now();
		if (!force && _lastScan.time_since_epoch().count() != 0 && scanNow - _lastScan < std::chrono::seconds(2)) return;
		_lastScan = scanNow;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) return;

		// Mark all current followers as temporarily not active until confirmed
		for (auto& f : _followers) {
			f.isActiveFollower = false;
		}

		// Cell references miss followers managed by multi-follower frameworks and
		// followers whose loaded references are attached to neighboring cells.
		// ProcessLists enumerates the loaded actors across those cells.
		processLists->ForAllActors([&](RE::Actor* actor) {
			if (!actor || actor == player || actor->IsDead()) return RE::BSContainer::ForEachResult::kContinue;

			// The follower faction also contains NPCs who are merely recruitable.
			// Only track actors Skyrim currently marks as the player's teammate.
			const bool isCompanion = actor->IsPlayerTeammate();

			if (isCompanion && !IsAnimalActor(actor)) {

				auto* base = actor->GetActorBase();
				if (!base) return RE::BSContainer::ForEachResult::kContinue;

				uint32_t baseId = base->GetFormID();
				std::string name = actor->GetName();
				if (name.empty()) name = "Companion";

				auto it = std::find_if(_followers.begin(), _followers.end(),
					[baseId](const FollowerClassData& f) { return f.formID == baseId; });

				if (it != _followers.end()) {
					it->isActiveFollower = true;
					it->name = name;
				} else {
					FollowerClassData newFollower;
					newFollower.formID = baseId;
					newFollower.name = name;
					newFollower.currentClass = ClassType::None;
					newFollower.classLevel = 1;
					newFollower.classXP = 0;
					newFollower.isActiveFollower = true;
					_followers.push_back(newFollower);

					SKSE::log::info("FollowerManager: Discovered active companion: {} (FormID: {:08X})", name, baseId);
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		// Kayıtlı listede eskiden kalmış hayvanlar varsa temizle
		_followers.erase(
			std::remove_if(_followers.begin(), _followers.end(), [this](const FollowerClassData& f) {
				auto* act = FindFollowerActor(f.formID);
				if (act && IsAnimalActor(act)) {
					SKSE::log::info("FollowerManager: Purged animal companion: {} [{:08X}]", f.name, f.formID);
					return true;
				}
				auto* form = RE::TESForm::LookupByID(f.formID);
				if (form) {
					if (auto* npc = form->As<RE::TESNPC>()) {
						if (npc->HasKeywordString("ActorTypeAnimal") || npc->HasKeywordString("ActorTypeCreature")) {
							return true;
						}
						auto* race = npc->GetRace();
						if (race && (race->HasKeywordString("ActorTypeAnimal") || race->HasKeywordString("ActorTypeCreature") ||
							race->data.flags.all(RE::RACE_DATA::Flag::kCantOpenDoors) || !race->data.flags.all(RE::RACE_DATA::Flag::kFaceGenHead))) {
							return true;
						}
					}
				}
				return false;
			}),
			_followers.end()
		);
	}

	std::vector<FollowerClassData>& FollowerManager::GetAllFollowers()
	{
		return _followers;
	}

	FollowerClassData* FollowerManager::GetFollowerData(uint32_t formID)
	{
		for (auto& f : _followers) {
			if (f.formID == formID) {
				return &f;
			}
		}
		return nullptr;
	}

	RE::Actor* FollowerManager::FindFollowerActor(uint32_t formID)
	{

		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* cell = player ? player->GetParentCell() : nullptr;
		if (cell) for (auto& ref : cell->GetRuntimeData().references) {
			if (!ref || !ref->Is(RE::FormType::ActorCharacter)) continue;
			auto* actor = ref->As<RE::Actor>();
			if (!actor) continue;
			const auto* base = actor->GetActorBase();
			if (actor->GetFormID() == formID || (base && base->GetFormID() == formID)) return actor;
		}

		if (auto* processLists = RE::ProcessLists::GetSingleton()) {
			RE::Actor* found = nullptr;
			processLists->ForAllActors([&](RE::Actor* actor) {
				if (!actor) return RE::BSContainer::ForEachResult::kContinue;
				const auto* base = actor->GetActorBase();
				if (actor->GetFormID() == formID || (base && base->GetFormID() == formID)) {
					found = actor;
					return RE::BSContainer::ForEachResult::kStop;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			if (found) return found;
		}

		return nullptr;
	}

	void FollowerManager::ModifyActorAV(RE::Actor* actor, uint32_t avIndex, float delta)
	{
		if (!actor || delta == 0.0f) return;
		auto av = static_cast<RE::ActorValue>(avIndex);
		auto* avo = actor->AsActorValueOwner();
		if (!avo) return;

		float currentBase = avo->GetBaseActorValue(av);
		avo->SetActorValue(av, currentBase + delta);

		SKSE::log::info("FollowerManager: Modified AV[{}] on {}: {} -> {}", avIndex,
			actor->GetName(), currentBase, currentBase + delta);
	}

	void FollowerManager::ApplyFollowerBonuses(RE::Actor* actor, FollowerClassData& data)
	{
		if (!actor || data.currentClass == ClassType::None) return;

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(data.currentClass);
		if (!defOpt) return;

		// Clean old bonuses first
		RemoveFollowerBonuses(actor, data);

		uint32_t tiers = data.classLevel / 5;
		for (const auto& avb : defOpt->avBonuses) {
			float total = avb.base + (avb.perLevel * static_cast<float>(tiers));
			if (total != 0.0f) {
				ModifyActorAV(actor, avb.avIndex, total);
				data.appliedAVBonuses.push_back({ avb.avIndex, total });
			}
		}
	}

	void FollowerManager::RemoveFollowerBonuses(RE::Actor* actor, FollowerClassData& data)
	{
		if (!actor) return;
		for (const auto& [avIndex, applied] : data.appliedAVBonuses) {
			ModifyActorAV(actor, avIndex, -applied);
		}
		data.appliedAVBonuses.clear();
	}

	bool FollowerManager::AssignClass(uint32_t formID, ClassType newClass)
	{
		auto* data = GetFollowerData(formID);
		if (!data) return false;

		auto classMgr = ClassChangeManager::GetSingleton();

		// If follower already has a class, changing class requires a Trainer NPC and Gold fee (identical to Player)
		if (data->currentClass != ClassType::None && data->currentClass != newClass) {
			if (!classMgr->HasTrainerAccess()) {
				PrismaUIManager::Notify("Trainer required to change companion's class! Talk to a trainer NPC first.", "warning");
				SKSE::log::warn("FollowerManager: Trainer required to change follower class.");
				return false;
			}

			uint32_t cost = ClassChangeManager::GetChangeCost(data->classLevel);
			if (!classMgr->DeductGold(cost)) {
				PrismaUIManager::Notify("Not enough gold to retrain companion!", "warning");
				SKSE::log::warn("FollowerManager: Not enough gold ({} needed) to change follower class.", cost);
				return false;
			}

			// Revoke trainer access after retraining
			classMgr->RevokeTrainerAccess();
		}

		bool isInitialPick = (data->currentClass == ClassType::None);
		ClassType oldClass = data->currentClass;

		auto* actor = FindFollowerActor(formID);
		if (actor && data->currentClass != ClassType::None) {
			RemoveFollowerBonuses(actor, *data);
		}

		data->currentClass = newClass;

		if (actor && newClass != ClassType::None) {
			ApplyFollowerBonuses(actor, *data);

			// Grant thematic starter items & spells on class assignment / change
			StartingGearManager::GetSingleton()->GrantStartingGearToActor(actor, newClass);
		}

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(newClass);
		std::string className = defOpt ? defOpt->name : "None";

		char msg[128];
		snprintf(msg, sizeof(msg), "%s is now a %s (Level %u)!", data->name.c_str(), className.c_str(), data->classLevel);
		PrismaUIManager::Notify(msg, "success");

		SKSE::log::info("FollowerManager: Assigned class {} to follower {} (InitialPick: {})",
			className, data->name, isInitialPick);

		// Notify King Core listeners of class change
		KingCoreAdapter::GetSingleton()->NotifyClassEvent(
			KingCore::ClassEventType::ClassChanged,
			actor,
			KingCoreAdapter::ToKingCoreClass(oldClass),
			KingCoreAdapter::ToKingCoreClass(newClass),
			data->classLevel
		);

		return true;
	}

	void FollowerManager::AddFollowerXP(uint32_t formID, uint32_t amount)
	{
		auto* data = GetFollowerData(formID);
		if (!data || data->classLevel >= 100) return;

		data->classXP += amount;
		const uint32_t startingLevel = data->classLevel;
		RE::Actor* levelUpActor = nullptr;
		uint32_t reqXP = XPManager::GetSingleton()->GetXPForNextLevel(data->classLevel);

		while (data->classXP >= reqXP && data->classLevel < 100) {
			data->classXP -= reqXP;
			data->classLevel++;
			UniversalSkillManager::AwardLevel(data->universalSkills, data->classLevel);

			levelUpActor = FindFollowerActor(formID);
			if (levelUpActor && data->currentClass != ClassType::None) {
				ApplyFollowerBonuses(levelUpActor, *data);
			}

			// Notify King Core listeners of level change
			if (data->currentClass != ClassType::None) KingCoreAdapter::GetSingleton()->NotifyClassEvent(
				KingCore::ClassEventType::LevelChanged,
				levelUpActor,
				KingCoreAdapter::ToKingCoreClass(data->currentClass),
				KingCoreAdapter::ToKingCoreClass(data->currentClass),
				data->classLevel
			);

			reqXP = XPManager::GetSingleton()->GetXPForNextLevel(data->classLevel);
		}

		if (data->classLevel > startingLevel) {
			EffectManager::PlayLevelUpEffect(levelUpActor);
			EffectManager::PlayLevelUpSound();
			if (SettingsManager::GetSingleton()->notifyClassLevel) {
				auto defOpt = ClassRegistry::GetSingleton()->GetClass(data->currentClass);
				char msg[160];
				const auto gained = data->classLevel - startingLevel;
				if (defOpt) snprintf(msg, sizeof(msg), "LEVEL UP! %s · %s %u · +%u skill points", data->name.c_str(), defOpt->name, data->classLevel, gained * 3);
				else snprintf(msg, sizeof(msg), "LEVEL UP! %s · Level %u · +%u skill points", data->name.c_str(), data->classLevel, gained * 3);
				PrismaUIManager::Notify(msg, "level");
			}
		}
	}

	void FollowerManager::GrantPartyCombatXP(uint32_t amount)
	{
		if (amount == 0) return;
		for (auto& f : _followers) {
			if (f.isActiveFollower && f.currentClass != ClassType::None) {
				AddFollowerXP(f.formID, amount);
			}
		}
	}

	void FollowerManager::ReapplyAllFollowerBonuses()
	{
		for (auto& f : _followers) {
			if (f.currentClass != ClassType::None) {
				auto* actor = FindFollowerActor(f.formID);
				if (actor && f.appliedAVBonuses.empty()) {
					// Actor values are restored by Skyrim's save. Reconstruct removal bookkeeping
					// without adding the class bonuses a second time.
					auto defOpt = ClassRegistry::GetSingleton()->GetClass(f.currentClass);
					if (!defOpt) continue;
					uint32_t tiers = f.classLevel / 5;
					for (const auto& avb : defOpt->avBonuses) {
						float amount = avb.base + avb.perLevel * static_cast<float>(tiers);
						if (amount != 0.0f) f.appliedAVBonuses.emplace_back(avb.avIndex, amount);
					}
				}
			}
			if (auto* actor = FindFollowerActor(f.formID)) UniversalSkillManager::GetSingleton()->EnsureFollowerBonuses(actor, f);
		}
	}

	void FollowerManager::DruidHealingTick()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		// Collect Druid followers and all follower actors in current cell
		struct LiveFollower {
			RE::Actor* actor;
			FollowerClassData* data;
		};

		std::vector<LiveFollower> druidFollowers;
		std::vector<RE::Actor*> allFollowerActors;

		auto* cell = player->GetParentCell();
		if (!cell) return;

		for (auto& f : _followers) {
			if (!f.isActiveFollower) continue;
			auto* actor = FindFollowerActor(f.formID);
			if (!actor || actor->IsDead()) continue;

			allFollowerActors.push_back(actor);

			if (f.currentClass == ClassType::Druid) {
				druidFollowers.push_back({ actor, &f });
			}
		}

		if (druidFollowers.empty()) return;

		auto healActor = [](RE::Actor* healer, RE::Actor* target, float healAmount, const char* reason) {
			auto* avo = target->AsActorValueOwner();
			if (!avo) return;
			float maxHP = avo->GetPermanentActorValue(RE::ActorValue::kHealth);
			float curHP = avo->GetActorValue(RE::ActorValue::kHealth);
			if (maxHP <= 0.0f) return;
			if ((curHP / maxHP) < 0.5f) {
				float newHP = curHP + healAmount;
				if (newHP > maxHP) newHP = maxHP;
				avo->SetActorValue(RE::ActorValue::kHealth, newHP);
				SKSE::log::info("FollowerManager(Druid): {} healed {} for {:.1f} HP ({})",
					healer->GetName(), target->GetName(), healAmount, reason);
			}
		};

		for (auto& [druidActor, druidData] : druidFollowers) {
			// Heal the player
			{
				auto* avo = player->AsActorValueOwner();
				if (avo) {
					float maxHP = avo->GetPermanentActorValue(RE::ActorValue::kHealth);
					float curHP = avo->GetActorValue(RE::ActorValue::kHealth);
					if (maxHP > 0.0f && (curHP / maxHP) < 0.5f) {
						float healAmt = 25.0f + (static_cast<float>(druidData->classLevel) * 0.5f);
						float newHP = curHP + healAmt;
						if (newHP > maxHP) newHP = maxHP;
						avo->SetActorValue(RE::ActorValue::kHealth, newHP);
						SKSE::log::info("FollowerManager(Druid): {} healed Player for {:.1f} HP",
							druidActor->GetName(), healAmt);
					}
				}
			}

			// Heal other followers below 60% HP
			for (auto* followerActor : allFollowerActors) {
				if (followerActor == druidActor) continue;
				auto* avo = followerActor->AsActorValueOwner();
				if (!avo) continue;
				float maxHP = avo->GetPermanentActorValue(RE::ActorValue::kHealth);
				float curHP = avo->GetActorValue(RE::ActorValue::kHealth);
				if (maxHP > 0.0f && (curHP / maxHP) < 0.6f) {
					float healAmt = 20.0f + (static_cast<float>(druidData->classLevel) * 0.4f);
					float newHP = curHP + healAmt;
					if (newHP > maxHP) newHP = maxHP;
					avo->SetActorValue(RE::ActorValue::kHealth, newHP);
					SKSE::log::info("FollowerManager(Druid): {} healed {} for {:.1f} HP (party heal)",
						druidActor->GetName(), followerActor->GetName(), healAmt);
				}
			}
		}
	}

	void FollowerManager::ClearFollowers()
	{
		_followers.clear();
	}

	void FollowerManager::AddLoadedFollower(const FollowerClassData& data)
	{
		auto it = std::find_if(_followers.begin(), _followers.end(),
			[&data](const FollowerClassData& f) { return f.formID == data.formID; });

		if (it != _followers.end()) {
			*it = data;
		} else {
			_followers.push_back(data);
		}
	}
}
