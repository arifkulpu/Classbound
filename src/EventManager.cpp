#include "EventManager.h"
#include "XPManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include "ClassChangeManager.h"
#include "FollowerManager.h"
#include "AffinityManager.h"
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

	void EventManager::HandleAction(const char* actionKey, float customMultiplier)
	{
		if (!actionKey || actionKey[0] == '\0') return;
		uint32_t reward = 0;
		if (std::strcmp(actionKey, "Lock Picked") == 0) reward = 4;
		else if (std::strcmp(actionKey, "Spell Cast") == 0 || std::strcmp(actionKey, "Destruction Spell Cast") == 0 ||
			std::strcmp(actionKey, "Conjuration Spell Cast") == 0 || std::strcmp(actionKey, "Healing Spell Cast") == 0) reward = 2;
		if (!reward) return;
		auto now = std::chrono::steady_clock::now();
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		const uint64_t cooldown = reward == 2 ? 1500 : 750;
		auto& last = _actionCooldowns[actionKey];
		if (nowMs - last < cooldown) return;
		last = nowMs;
		XPManager::GetSingleton()->AddXP(static_cast<uint32_t>(static_cast<float>(reward) * customMultiplier), actionKey);
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

		// Check party health on combat hits regardless of who was hit.
		constexpr uint64_t kDruidHealCooldownMs = 8000;
		if ((nowMs - _druidHealCooldownMs) >= kDruidHealCooldownMs) {
			_druidHealCooldownMs = nowMs;
			FollowerManager::GetSingleton()->DruidHealingTick();
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

		if (isPlayerAttacker || isActiveFollowerAttacker) {
			DamageRecord rec;
			rec.timestamp = nowMs;
			rec.wasSneak = attacker->IsSneaking();

			// Power attack
			if (a_event->flags.any(RE::TESHitEvent::Flag::kPowerAttack)) {
				rec.wasPowerAttack = true;
				if (isPlayerAttacker) HandleAction("Power Attack Hit");
			}

			// Shield bash / counter
			if (a_event->flags.any(RE::TESHitEvent::Flag::kBashAttack)) {
				rec.wasBash = true;
				if (isPlayerAttacker) {
					HandleAction("Shield Bash Hit");
					HandleAction("Block & Counter");
				}
			}

			// Check weapon or spell source
			if (a_event->source != 0) {
				if (auto* sourceForm = RE::TESForm::LookupByID(a_event->source)) {
					if (auto* weap = sourceForm->As<RE::TESObjectWEAP>()) {
						if (weap->IsTwoHandedSword() || weap->IsTwoHandedAxe()) {
							rec.wasTwoHanded = true;
							if (isPlayerAttacker) HandleAction("Two-Handed Combat");
						} else if (weap->IsOneHandedSword() || weap->IsOneHandedAxe() || weap->IsOneHandedMace() || weap->IsOneHandedDagger()) {
							rec.wasOneHanded = true;
							if (isPlayerAttacker) HandleAction("One-Handed Combat");
						} else if (weap->IsBow() || weap->IsCrossbow()) {
							rec.wasBow = true;
							if (isPlayerAttacker) {
								HandleAction("Archery Combat");
								HandleAction("Long Distance Hit");
							}
						} else if (weap->IsStaff()) {
							rec.wasStaff = true;
							if (isPlayerAttacker) {
								HandleAction("Staff Strike");
								HandleAction("Magic Combat");
							}
						}
					} else if (auto* spell = sourceForm->As<RE::SpellItem>()) {
						rec.wasSpell = true;
						rec.spellSchool = static_cast<uint32_t>(spell->GetAssociatedSkill());
						if (isPlayerAttacker) HandleAction("Magic Combat");
					}
				}
			}

			if (auto* targetRef = a_event->target.get()) {
				if (auto* targetActor = targetRef->As<RE::Actor>()) {
					_partyDamagedActors[targetActor->GetFormID()] = rec;
				}
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
		if (!a_event || !a_event->object) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->object.get() == player) {
			auto& playerData = ClassManager::GetSingleton()->GetPlayerData();
			playerData.stats.spellsCast++;

			HandleAction("Spell Cast");
			HandleAction("Spell Infused Shot");

			if (a_event->spell != 0) {
				if (auto* spellForm = RE::TESForm::LookupByID(a_event->spell)) {
					if (auto* spell = spellForm->As<RE::SpellItem>()) {
						auto school = spell->GetAssociatedSkill();
						const char* spellName = spell->GetName();
						std::string sName = spellName ? spellName : "";
						for (auto& c : sName) c = static_cast<char>(tolower(c));

						if (school == RE::ActorValue::kDestruction) {
							HandleAction("Destruction Spell Cast");
							if (sName.find("fire") != std::string::npos || sName.find("flame") != std::string::npos) {
								HandleAction("Fire Cast");
							} else if (sName.find("frost") != std::string::npos || sName.find("ice") != std::string::npos) {
								HandleAction("Frost Cast");
							} else if (sName.find("shock") != std::string::npos || sName.find("spark") != std::string::npos || sName.find("lightning") != std::string::npos) {
								HandleAction("Shock Cast");
							}
						} else if (school == RE::ActorValue::kRestoration) {
							HandleAction("Healing in Combat");
							HandleAction("Healing Spell Cast");
						} else if (school == RE::ActorValue::kConjuration) {
							HandleAction("Conjuration Spell Cast");
							if (sName.find("zombie") != std::string::npos || sName.find("reanimate") != std::string::npos || sName.find("corpse") != std::string::npos || sName.find("revive") != std::string::npos) {
								HandleAction("Undead Raised");
							}
							if (sName.find("soul") != std::string::npos) {
								HandleAction("Soul Trapped");
							}
						} else if (school == RE::ActorValue::kIllusion) {
							HandleAction("Calm Spell Cast");
							HandleAction("Song Performed");
						}
					}
				}
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
		XPManager::GetSingleton()->AddXP(8, "NPC defeated");

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
		if (!_craftingMenuOpen || !a_event || a_event->itemCount <= 0) return RE::BSEventNotifyControl::kContinue;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || a_event->newContainer != player->GetFormID() || a_event->oldContainer == player->GetFormID())
			return RE::BSEventNotifyControl::kContinue;

		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_event->baseObj);
		if (!item) return RE::BSEventNotifyControl::kContinue;

		// Reward only outputs that appeared in the player's inventory while a
		// crafting station was open. This covers potions and crafted or modified gear.
		const uint32_t count = static_cast<uint32_t>((std::min)(a_event->itemCount, 5));
		if (item->As<RE::AlchemyItem>()) {
			XPManager::GetSingleton()->AddXP(12 * count, "Potion crafted");
		} else if (item->As<RE::TESObjectWEAP>() || item->As<RE::TESObjectARMO>()) {
			XPManager::GetSingleton()->AddXP(10 * count, "Gear crafted or improved");
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
