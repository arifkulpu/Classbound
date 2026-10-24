#include "EventManager.h"
#include "XPManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include "ClassChangeManager.h"
#include "FollowerManager.h"
#include "AffinityManager.h"
#include "PrismaUIManager.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cctype>
#include <string>

namespace Classbound
{
	void EventManager::Register()
	{
		if (auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
			scripts->AddEventSink<RE::TESHitEvent>(this);
			scripts->AddEventSink<RE::TESSpellCastEvent>(this);
			scripts->AddEventSink<RE::TESDeathEvent>(this);
			scripts->AddEventSink<RE::TESLockChangedEvent>(this);
			scripts->AddEventSink<RE::TESActivateEvent>(this);
			scripts->AddEventSink<RE::TESQuestStageEvent>(this);
			scripts->AddEventSink<RE::TESEquipEvent>(this);
			scripts->AddEventSink<RE::TESContainerChangedEvent>(this);
			SKSE::log::info("Registered SKSE Event Sinks (Hit, SpellCast, Death, LockChanged, Activate, QuestStage, Equip, ContainerChanged).");
		}
		if (auto* source = RE::LocationDiscovery::GetEventSource()) {
			source->AddEventSink(this);
			SKSE::log::info("Registered location discovery XP event sink.");
		}
		
		if (auto ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
			SKSE::log::info("Registered UI Event Sink (MenuOpenClose).");
		}
	}

	bool EventManager::IsPracticeDummy(RE::TESObjectREFR* target)
	{
		if (!target) return false;
		auto* base = target->GetBaseObject();
		if (!base) return false;

		std::string edid = base->GetFormEditorID() ? base->GetFormEditorID() : "";
		std::string name = target->GetName() ? target->GetName() : (base->GetName() ? base->GetName() : "");
		for (auto& c : edid) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		for (auto& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (edid.find("dummy") != std::string::npos || edid.find("target") != std::string::npos ||
			edid.find("practice") != std::string::npos || edid.find("archerytarget") != std::string::npos) {
			return true;
		}
		if (name.find("dummy") != std::string::npos || name.find("target") != std::string::npos ||
			name.find("practice") != std::string::npos || name.find("kukla") != std::string::npos ||
			name.find("manken") != std::string::npos) {
			return true;
		}
		return false;
	}

	void EventManager::HandleAction(const char* actionKey, float customMultiplier, bool isPracticeDummy)
	{
		if (!actionKey || actionKey[0] == '\0') return;
		uint32_t reward = 0;
		if (std::strcmp(actionKey, "Lock Picked") == 0) reward = 4;
		else if (std::strcmp(actionKey, "Spell Cast") == 0 || std::strcmp(actionKey, "Destruction Spell Cast") == 0 ||
			std::strcmp(actionKey, "Conjuration Spell Cast") == 0 || std::strcmp(actionKey, "Healing Spell Cast") == 0 ||
			std::strcmp(actionKey, "Illusion Spell Cast") == 0 || std::strcmp(actionKey, "Alteration Spell Cast") == 0) reward = 2;
		else if (std::strcmp(actionKey, "One-Handed Combat") == 0 || std::strcmp(actionKey, "Two-Handed Combat") == 0 ||
			std::strcmp(actionKey, "Archery Combat") == 0 || std::strcmp(actionKey, "Staff Strike") == 0 ||
			std::strcmp(actionKey, "Block Successful Hit") == 0 || std::strcmp(actionKey, "Shield Bash Hit") == 0 ||
			std::strcmp(actionKey, "Training Dummy Hit") == 0 || std::strcmp(actionKey, "Power Attack Hit") == 0) reward = 2;
		if (!reward) return;

		auto now = std::chrono::steady_clock::now();
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		const uint64_t cooldown = 1200; // 1.2s cooldown between hit/spell combat XP ticks
		auto& last = _actionCooldowns[actionKey];
		if (nowMs - last < cooldown) return;
		last = nowMs;

		const uint32_t finalXP = static_cast<uint32_t>(static_cast<float>(reward) * customMultiplier);

		// If hit was on practice dummy: accumulate in dummy buffer without triggering notification spam.
		if (isPracticeDummy) {
			_batchedDummyXP += finalXP;
			_lastDummyHitTimeMs = nowMs;
			return;
		}

		// If player is in active combat: accumulate in combat buffer until combat ends.
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player && player->IsInCombat()) {
			_batchedCombatXP += finalXP;
			_combatHitsCount++;
			return;
		}

		// Non-combat utility actions (like lockpicking): grant normally
		XPManager::GetSingleton()->AddXP(finalXP, actionKey);
	}

	void EventManager::UpdateCombatAndTrainingBatch()
	{
		auto now = std::chrono::steady_clock::now();
		uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		if (nowMs - _lastBatchCheckMs < 500) return;
		_lastBatchCheckMs = nowMs;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		// Check player combat state transition
		const bool currentlyInCombat = player->IsInCombat();
		if (_inCombatState && !currentlyInCombat) {
			// Combat ended! Award all batched combat XP in a single consolidated notification.
			if (_batchedCombatXP > 0) {
				char msg[128];
				snprintf(msg, sizeof(msg), "Combat Victory: +%u XP", _batchedCombatXP);
				XPManager::GetSingleton()->AddXP(_batchedCombatXP, msg, true);
				_batchedCombatXP = 0;
				_combatHitsCount = 0;
			}
		}
		_inCombatState = currentlyInCombat;

		// Drive companion tactics and druid heals if party or player is in combat
		if (currentlyInCombat) {
			constexpr uint64_t kCombatTacticsCooldownMs = 4000;
			if ((nowMs - _combatTacticsCooldownMs) >= kCombatTacticsCooldownMs) {
				_combatTacticsCooldownMs = nowMs;
				FollowerManager::GetSingleton()->UpdateCombatTactics();
			}
			constexpr uint64_t kDruidHealCooldownMs = 6000;
			if ((nowMs - _druidHealCooldownMs) >= kDruidHealCooldownMs) {
				_druidHealCooldownMs = nowMs;
				FollowerManager::GetSingleton()->DruidHealingTick();
			}
		}

		// Check practice dummy session: if player hit a dummy and 3s have passed since last hit, flush total.
		if (_batchedDummyXP > 0 && _lastDummyHitTimeMs > 0) {
			if (nowMs - _lastDummyHitTimeMs >= 3000) {
				char msg[128];
				snprintf(msg, sizeof(msg), "Training Finished: +%u XP", _batchedDummyXP);
				XPManager::GetSingleton()->AddXP(_batchedDummyXP, msg, true);
				_batchedDummyXP = 0;
				_lastDummyHitTimeMs = 0;
			}
		}
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event || !a_event->target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return RE::BSEventNotifyControl::kContinue;

		auto now = std::chrono::steady_clock::now();
		uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		// Periodic batch checks (combat end & practice dummy 3s timeout)
		if (nowMs - _lastBatchCheckMs >= 1000) {
			_lastBatchCheckMs = nowMs;
			UpdateCombatAndTrainingBatch();
		}

		// Record damage only when it was dealt by the player or an active companion.
		auto* attackerRef = a_event->cause.get();
		auto* attacker = attackerRef ? attackerRef->As<RE::Actor>() : nullptr;
		bool isPlayerAttacker = attacker == player;
		bool isActiveFollowerAttacker = false;
		if (attacker && !isPlayerAttacker && attacker->IsPlayerTeammate()) {
			auto* followerManager = FollowerManager::GetSingleton();
			followerManager->ScanActiveFollowers();
			if (auto* base = attacker->GetActorBase()) {
				for (const auto& follower : followerManager->GetAllFollowers()) {
					if (follower.isActiveFollower && follower.formID == base->GetFormID()) {
						isActiveFollowerAttacker = true;
						break;
					}
				}
			}
		}

		auto* targetRef = a_event->target.get();
		auto* targetActor = targetRef ? targetRef->As<RE::Actor>() : nullptr;
		const bool isTargetDummy = IsPracticeDummy(targetRef);

		// Only award XP if the target is an enemy/combat actor, or a recognized practice dummy.
		// Striking the empty air or random static objects does not award combat XP.
		const bool isValidTarget = (targetActor != nullptr) || isTargetDummy;

		if ((isPlayerAttacker || isActiveFollowerAttacker) && isValidTarget) {
			DamageRecord rec;
			rec.timestamp = nowMs;
			rec.wasSneak = attacker->IsSneaking();

			// Power attack
			if (a_event->flags.any(RE::TESHitEvent::Flag::kPowerAttack)) {
				rec.wasPowerAttack = true;
				if (isPlayerAttacker) HandleAction("Power Attack Hit", 1.0f, isTargetDummy);
			}

			// Shield bash / counter
			if (a_event->flags.any(RE::TESHitEvent::Flag::kBashAttack)) {
				rec.wasBash = true;
				if (isPlayerAttacker) {
					HandleAction("Shield Bash Hit", 1.0f, isTargetDummy);
					HandleAction("Block & Counter", 1.0f, isTargetDummy);
				}
			}

			// Check weapon or spell source
			if (a_event->source != 0) {
				if (auto* sourceForm = RE::TESForm::LookupByID(a_event->source)) {
					if (auto* weap = sourceForm->As<RE::TESObjectWEAP>()) {
						if (weap->IsTwoHandedSword() || weap->IsTwoHandedAxe() || weap->weaponData.animationType == RE::WEAPON_TYPE::kTwoHandSword || weap->weaponData.animationType == RE::WEAPON_TYPE::kTwoHandAxe) {
							rec.wasTwoHanded = true;
							if (isPlayerAttacker) HandleAction("Two-Handed Combat", 1.0f, isTargetDummy);
						} else if (weap->IsOneHandedSword() || weap->IsOneHandedAxe() || weap->IsOneHandedMace() || weap->IsOneHandedDagger()) {
							rec.wasOneHanded = true;
							if (isPlayerAttacker) HandleAction("One-Handed Combat", 1.0f, isTargetDummy);
						} else if (weap->IsBow() || weap->IsCrossbow()) {
							rec.wasBow = true;
							if (isPlayerAttacker) {
								HandleAction("Archery Combat", 1.0f, isTargetDummy);
								HandleAction("Long Distance Hit", 1.0f, isTargetDummy);
							}
						} else if (weap->IsStaff()) {
							rec.wasStaff = true;
							if (isPlayerAttacker) {
								HandleAction("Staff Strike", 1.0f, isTargetDummy);
								HandleAction("Magic Combat", 1.0f, isTargetDummy);
							}
						}
					} else if (auto* spell = sourceForm->As<RE::SpellItem>()) {
						rec.wasSpell = true;
						rec.spellSchool = static_cast<uint32_t>(spell->GetAssociatedSkill());
						if (isPlayerAttacker) {
							auto school = spell->GetAssociatedSkill();
							if (school == RE::ActorValue::kDestruction) HandleAction("Destruction Spell Cast", 1.0f, isTargetDummy);
							else if (school == RE::ActorValue::kConjuration) HandleAction("Conjuration Spell Cast", 1.0f, isTargetDummy);
							else if (school == RE::ActorValue::kRestoration) HandleAction("Healing Spell Cast", 1.0f, isTargetDummy);
							else if (school == RE::ActorValue::kIllusion) HandleAction("Illusion Spell Cast", 1.0f, isTargetDummy);
							else if (school == RE::ActorValue::kAlteration) HandleAction("Alteration Spell Cast", 1.0f, isTargetDummy);
							else HandleAction("Spell Cast", 1.0f, isTargetDummy);
						}
					}
				}
			}

			if (targetActor) {
				_partyDamagedActors[targetActor->GetFormID()] = rec;
			}
		}
		// 2. Player was the target (taking damage / blocking)
		else if (a_event->target.get() == player) {
			if (a_event->flags.any(RE::TESHitEvent::Flag::kHitBlocked)) {
				HandleAction("Block Successful Hit");
				HandleAction("Block & Counter");
			}
			HandleAction("Heavy Armor Combat");
			HandleAction("Heavy Armor Hit Taken");
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
	{
		if (!a_event || !a_event->object || a_event->spell == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->object.get() == player) {
			auto* spellForm = RE::TESForm::LookupByID(a_event->spell);
			auto* spell = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
			if (!spell) return RE::BSEventNotifyControl::kContinue;

			// TESSpellCastEvent can also be raised by abilities, powers, enchantments,
			// and other automatic effects. Only award activity XP for an explicitly
			// cast spell or a scroll; passive/constant effects are not player casts.
			const auto type = spell->GetSpellType();
			if (type != RE::MagicSystem::SpellType::kSpell &&
				type != RE::MagicSystem::SpellType::kScroll) {
				return RE::BSEventNotifyControl::kContinue;
			}
			if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConstantEffect) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
			playerData.stats.spellsCast++;

			auto school = spell->GetAssociatedSkill();
			const bool isHostile = spell->IsHostile();
			const bool inCombat = player->IsInCombat();

			// For hostile spells (Destruction, offensive spells), only award cast XP
			// if the player is in active combat. Casting hostile projectile/touch spells
			// into empty air outside combat does not award free spell XP.
			// Utility/buff/healing spells (Restoration, Conjuration summons, Alteration armor)
			// are allowed to award XP when cast intentionally.
			if (isHostile && !inCombat) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const char* spellName = spell->GetName();
			std::string sName = spellName ? spellName : "";
			for (auto& c : sName) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

			if (school == RE::ActorValue::kDestruction) {
				HandleAction("Destruction Spell Cast");
			} else if (school == RE::ActorValue::kRestoration) {
				HandleAction("Healing Spell Cast");
			} else if (school == RE::ActorValue::kConjuration) {
				HandleAction("Conjuration Spell Cast");
			} else if (school == RE::ActorValue::kIllusion) {
				HandleAction("Illusion Spell Cast");
			} else if (school == RE::ActorValue::kAlteration) {
				HandleAction("Alteration Spell Cast");
			} else {
				HandleAction("Spell Cast");
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_event) return RE::BSEventNotifyControl::kContinue;

		if (a_event->menuName == RE::DialogueMenu::MENU_NAME && a_event->opening) {
			auto checkActor = [](RE::Actor* actor) {
				if (actor && actor->GetActorBase()) {
					uint32_t baseId = actor->GetActorBase()->GetFormID();
					if (ClassChangeManager::IsTrainerNPC(baseId)) {
						ClassChangeManager::GetSingleton()->GrantTrainerAccess();
						SKSE::log::info("EventManager: Talked to Trainer NPC '{}' (0x{:X})",
							actor->GetName(), baseId);
					}
				}
			};

			// Method 1: MenuTopicManager target actor (most accurate for active dialogue)
			if (auto topicMgr = RE::MenuTopicManager::GetSingleton()) {
				if (auto speaker = topicMgr->speaker.get()) {
					checkActor(speaker->As<RE::Actor>());
				}
			}

			// Method 2: Crosshair pick data fallback
			if (!ClassChangeManager::GetSingleton()->HasTrainerAccess()) {
				auto crosshair = RE::CrosshairPickData::GetSingleton();
				if (crosshair && crosshair->target && crosshair->target.get()->Is(RE::FormType::ActorCharacter)) {
					checkActor(crosshair->target.get()->As<RE::Actor>());
				}
			}
		}
		else if (a_event->menuName == RE::CraftingMenu::MENU_NAME) {
			// The menu event only marks the transaction window. XP is awarded from
			// actual player inventory additions in TESContainerChangedEvent.
			_craftingMenuOpen = a_event->opening;
			if (!a_event->opening) {
				// Crafting station closed: flush any batched cooking / crafting XP in single consolidated notifications
				if (_batchedFoodCount > 0 && _batchedFoodXP > 0) {
					char msg[128];
					snprintf(msg, sizeof(msg), "%u food prepared (+%u XP)", _batchedFoodCount, _batchedFoodXP);
					XPManager::GetSingleton()->AddXP(_batchedFoodXP, msg, true);
					_batchedFoodCount = 0;
					_batchedFoodXP = 0;
				}
				if (_batchedPotionsCount > 0 && _batchedPotionsXP > 0) {
					char msg[128];
					snprintf(msg, sizeof(msg), "%u potions crafted (+%u XP)", _batchedPotionsCount, _batchedPotionsXP);
					XPManager::GetSingleton()->AddXP(_batchedPotionsXP, msg, true);
					_batchedPotionsCount = 0;
					_batchedPotionsXP = 0;
				}
				if (_batchedGearCount > 0 && _batchedGearXP > 0) {
					char msg[128];
					snprintf(msg, sizeof(msg), "%u gear crafted or improved (+%u XP)", _batchedGearCount, _batchedGearXP);
					XPManager::GetSingleton()->AddXP(_batchedGearXP, msg, true);
					_batchedGearCount = 0;
					_batchedGearXP = 0;
				}
			}
		}
		else if (a_event->menuName == RE::LockpickingMenu::MENU_NAME) {
			auto now = std::chrono::steady_clock::now();
			auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
			if (a_event->opening) {
				auto* target = RE::LockpickingMenu::GetTargetReference();
				_lockpickingTarget = target ? target->GetFormID() : 0;
				_lockpickingAttemptUntilMs = nowMs + 600000;
			} else if (_lockpickingTarget != 0) {
				// The lock-change callback can arrive during menu teardown.
				_lockpickingAttemptUntilMs = nowMs + 2000;
			}
		}
		else if (a_event->menuName == RE::BarterMenu::MENU_NAME) {
			auto getSafePlayerGold = []() -> int32_t {
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) return 0;
				// 0x0000000F is Skyrim's hardcoded Gold001 FormID.
				// GetItemCount calls Skyrim's native internal engine counter directly (RELOCATION_ID 19275)
				// instead of doing an unsafe manual container traversal via GetInventory() while the menu is transitioning.
				static auto* goldObj = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
				if (goldObj) {
					return player->GetItemCount(goldObj);
				}
				return 0;
			};

			if (a_event->opening) {
				_goldBeforeBarter = getSafePlayerGold();
			} else {
				// Barter menu closed; check if player actually earned gold from selling items
				int32_t goldAfter = getSafePlayerGold();
				if (goldAfter > _goldBeforeBarter) {
					int32_t goldEarned = goldAfter - _goldBeforeBarter;
					if (goldEarned >= 100) {
						HandleAction("Item Sold (100+ gold)");
					}
					if (goldAfter >= 1000) {
						HandleAction("Gold Milestone 1000g");
					}
				}
			}
		}
		else if (a_event->menuName == RE::ContainerMenu::MENU_NAME && !a_event->opening) {
			HandleAction("Chest Looted");
			HandleAction("Treasure Chest Found");
		}
		else if ((a_event->menuName == RE::LevelUpMenu::MENU_NAME || a_event->menuName == RE::StatsMenu::MENU_NAME) && !a_event->opening) {
			// Player leveled up their Skyrim character; check if pending class XP can now level up
			XPManager::GetSingleton()->CheckLevelUp();
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		if (!a_event || !a_event->actorDying) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return RE::BSEventNotifyControl::kContinue;

		auto* dyingRef = a_event->actorDying.get();
		if (!dyingRef || dyingRef == player) return RE::BSEventNotifyControl::kContinue;

		auto* dying = dyingRef->As<RE::Actor>();
		if (!dying) return RE::BSEventNotifyControl::kContinue;

		// Check whether the player or an active companion actually damaged this actor recently.
		uint32_t dyingFormId = dying->GetFormID();
		auto now = std::chrono::steady_clock::now();
		uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		bool wasDamagedByParty = false;
		DamageRecord damageInfo;
		auto it = _partyDamagedActors.find(dyingFormId);
		if (it != _partyDamagedActors.end()) {
			if (nowMs - it->second.timestamp <= 15000) { // 15 seconds window
				wasDamagedByParty = true;
				damageInfo = it->second;
			}
			_partyDamagedActors.erase(it);
		}

		if (now - _lastMaintenance >= std::chrono::seconds(30)) {
			_lastMaintenance = now;
			for (auto staleIt = _partyDamagedActors.begin(); staleIt != _partyDamagedActors.end();) { if (nowMs - staleIt->second.timestamp > 60000) staleIt = _partyDamagedActors.erase(staleIt); else ++staleIt; }
			for (auto stale = _deathRewards.begin(); stale != _deathRewards.end();) { if (nowMs - stale->second > 60000) stale = _deathRewards.erase(stale); else ++stale; }
		}

		if (!wasDamagedByParty) return RE::BSEventNotifyControl::kContinue;

		// A short form-ID cooldown prevents duplicate eligible party-kill callbacks.
		auto rewarded = _deathRewards.find(dyingFormId);
		if (rewarded != _deathRewards.end() && nowMs - rewarded->second < 1000)
			return RE::BSEventNotifyControl::kContinue;
		_deathRewards[dyingFormId] = nowMs;

		if (player->IsInCombat()) {
			_batchedCombatXP += 8;
			_combatHitsCount++;
		} else {
			XPManager::GetSingleton()->AddXP(8, "NPC defeated");
		}

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		playerData.stats.enemiesKilled++;

		auto* base = dying->GetActorBase();

		// Boss & Contract defeat check
		if (base && (base->IsUnique() || dying->GetLevel() >= 30)) {
			playerData.stats.bossesDefeated++;
			HandleAction("Boss Kill");
			HandleAction("Boss Defeated");
			HandleAction("Contract Target Kill");
			if (damageInfo.wasBow) HandleAction("Boss Kill (Bow)");
			if (damageInfo.wasSpell || damageInfo.wasStaff) HandleAction("Boss Kill (Magic)");
			if (damageInfo.wasBash) HandleAction("Boss Kill (Shielded)");
		}

		// Race / Keyword classification
		if (dying->HasKeywordString("ActorTypeUndead") || (base && base->GetRace() && base->GetRace()->HasKeywordString("ActorTypeUndead"))) {
			HandleAction("Undead Destroyed");
		}
		if (dying->HasKeywordString("ActorTypeAnimal") || dying->HasKeywordString("ActorTypeCreature")) {
			HandleAction("Animal Hunted");
			HandleAction("Large Beast Kill");
			HandleAction("Familiar / Beast Kill");
		}
		if (dying->HasKeywordString("Vampire") || (base && base->GetRace() && base->GetRace()->HasKeywordString("Vampire"))) {
			HandleAction("Vampire Kill");
		}
		if (dying->HasKeywordString("ActorTypeDragon")) {
			HandleAction("Dragon Kill");
			HandleAction("Dragon Defeated");
		}
		if (dying->HasKeywordString("ActorTypeSpriggan")) {
			HandleAction("Spriggan Defeated");
		}

		// Sneak kill actions
		if (player->IsSneaking() || damageInfo.wasSneak) {
			playerData.stats.sneakKills++;
			HandleAction("Sneak Kill");
			HandleAction("Sneak Takedown");
			HandleAction("Silent Kill");
			HandleAction("One-Shot Kill");
			HandleAction("Critical Strike");
		}

		// Power Attack & Bash Kills
		if (damageInfo.wasPowerAttack) {
			HandleAction("Power Attack Kill");
		}
		if (damageInfo.wasBash) {
			HandleAction("Shield Bash Kill");
		}

		// Precise weapon/spell specific kill dispatch
		if (damageInfo.wasTwoHanded) {
			playerData.stats.meleeKills++;
			HandleAction("Two-Handed Kill");
			HandleAction("Melee Kill");
		} else if (damageInfo.wasOneHanded) {
			playerData.stats.meleeKills++;
			HandleAction("One-Handed Kill");
			HandleAction("Melee Kill");
			if (damageInfo.wasSpell) {
				HandleAction("Spell & Sword Kill");
			}
		} else if (damageInfo.wasBow) {
			playerData.stats.archeryKills++;
			HandleAction("Archery Kill");
			HandleAction("Crit Kill");
		} else if (damageInfo.wasSpell || damageInfo.wasStaff) {
			playerData.stats.magicKills++;
			HandleAction("Magic Kill");
			if (damageInfo.spellSchool == static_cast<uint32_t>(RE::ActorValue::kDestruction)) {
				HandleAction("Destruction Kill");
				HandleAction("Elemental Kill");
			} else if (damageInfo.spellSchool == static_cast<uint32_t>(RE::ActorValue::kConjuration)) {
				HandleAction("Conjuration Kill");
			}
		} else {
			// Fallback if specific attack wasn't captured: award generic kill based on category
			auto defOpt = ClassRegistry::GetSingleton()->GetClass(playerData.currentClass);
			if (defOpt) {
				switch (defOpt->category) {
				case MainCategory::Melee:
					HandleAction("Melee Kill");
					break;
				case MainCategory::Ranged:
					HandleAction("Archery Kill");
					break;
				case MainCategory::Magic:
					HandleAction("Magic Kill");
					break;
				case MainCategory::Social:
					HandleAction("Melee Kill");
					break;
				default:
					HandleAction("Melee Kill");
					break;
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESLockChangedEvent* a_event, RE::BSTEventSource<RE::TESLockChangedEvent>*)
	{
		if (!a_event || !a_event->lockedObject || _lockpickingTarget == 0) return RE::BSEventNotifyControl::kContinue;
		auto* ref = a_event->lockedObject.get();
		auto now = std::chrono::steady_clock::now();
		auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
		if (!ref || nowMs > _lockpickingAttemptUntilMs || ref->GetFormID() != _lockpickingTarget || ref->IsLocked())
			return RE::BSEventNotifyControl::kContinue;

		_lockpickingTarget = 0;
		_lockpickingAttemptUntilMs = 0;
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
		playerData.stats.locksPicked++;
		HandleAction("Lock Picked");
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		if (!a_event || !a_event->actionRef || !a_event->objectActivated) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->actionRef.get() == player) {
			if (auto* obj = a_event->objectActivated.get()) {
				auto now = std::chrono::steady_clock::now();
				auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
				if (now - _lastActivityPrune >= std::chrono::seconds(60)) {
					_lastActivityPrune = now;
					for (auto it = _activityCooldowns.begin(); it != _activityCooldowns.end();) { if (nowMs - it->second > 300000) it = _activityCooldowns.erase(it); else ++it; }
				}
				auto& lastAward = _activityCooldowns[obj->GetFormID()];
				if (nowMs - lastAward < 60000) return RE::BSEventNotifyControl::kContinue;
				auto formType = obj->GetFormType();
				if (formType == RE::FormType::Flora || formType == RE::FormType::Tree) {
					lastAward = nowMs;
					XPManager::GetSingleton()->AddXP(12, "Harvesting");
				} else {
					const char* rawName = obj->GetName();
					std::string name = rawName ? rawName : "";
					std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (formType == RE::FormType::Furniture && (name.find("ore") != std::string::npos || name.find("vein") != std::string::npos || name.find("mine") != std::string::npos)) {
						lastAward = nowMs;
						XPManager::GetSingleton()->AddXP(12, "Mining");
					} else if (name.find("fish") != std::string::npos || name.find("fishing") != std::string::npos) {
						lastAward = nowMs;
						XPManager::GetSingleton()->AddXP(12, "Fishing");
					}
				}
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>*)
	{
		auto* quest = a_event ? RE::TESForm::LookupByID<RE::TESQuest>(a_event->formID) : nullptr;
		if (a_event && quest && quest->IsCompleted() && _completedQuests.insert(a_event->formID).second) {
			auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
			playerData.stats.dungeonsCleared++;
			XPManager::GetSingleton()->AddXP(400, "Quest complete", true);
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::LocationDiscovery::Event*, RE::BSTEventSource<RE::LocationDiscovery::Event>*)
	{
		XPManager::GetSingleton()->AddXP(20, "New location discovered");
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		if (!a_event || a_event->itemCount <= 0) return RE::BSEventNotifyControl::kContinue;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || a_event->newContainer != player->GetFormID() || a_event->oldContainer == player->GetFormID())
			return RE::BSEventNotifyControl::kContinue;

		// 1. Fortune Hunter (Gold Finder) Universal Perk:
		// If player looted gold (0x0000000F) from a chest, container, or corpse
		if (a_event->baseObj == 0x0000000F && a_event->oldContainer != 0) {
			auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
			const uint32_t goldRank = std::min(playerData.universalSkills.ranks[16], kMaxUniversalRank);
			if (goldRank > 0) {
				// Bonus gold = original gold * (rank * 1%)
				float bonusFloat = static_cast<float>(a_event->itemCount) * (static_cast<float>(goldRank) * 0.01f);
				uint32_t bonusGold = static_cast<uint32_t>(bonusFloat);
				// Random fractional chance for remaining decimal
				float remainder = bonusFloat - static_cast<float>(bonusGold);
				if (remainder > 0.0f) {
					if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < remainder) {
						bonusGold += 1;
					}
				}
				if (bonusGold > 0) {
					static auto* goldObj = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
					if (goldObj) {
						player->AddObjectToContainer(goldObj, nullptr, static_cast<int32_t>(bonusGold), nullptr);
						char toast[96];
						snprintf(toast, sizeof(toast), "Fortune Hunter: +%u bonus gold", bonusGold);
						PrismaUIManager::Notify(toast, "info");
					}
				}
			}
		}

		if (!_craftingMenuOpen) return RE::BSEventNotifyControl::kContinue;

		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_event->baseObj);
		if (!item) return RE::BSEventNotifyControl::kContinue;

		// Reward only outputs that appeared in the player's inventory while a
		// crafting station was open. This covers food, potions and crafted or modified gear.
		// Batch all creations silently until the crafting menu closes.
		const uint32_t count = static_cast<uint32_t>((std::min)(a_event->itemCount, 50));
		if (auto* alch = item->As<RE::AlchemyItem>()) {
			if (alch->IsFood()) {
				_batchedFoodCount += count;
				_batchedFoodXP += 10 * count;
			} else {
				_batchedPotionsCount += count;
				_batchedPotionsXP += 12 * count;
			}
		} else if (item->As<RE::TESObjectWEAP>() || item->As<RE::TESObjectARMO>()) {
			_batchedGearCount += count;
			_batchedGearXP += 10 * count;
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventManager::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (a_event && a_event->actor.get() == RE::PlayerCharacter::GetSingleton()) {
			AffinityManager::GetSingleton()->EvaluateAffinity();
		}
		return RE::BSEventNotifyControl::kContinue;
	}
}
