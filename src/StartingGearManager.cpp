#include "StartingGearManager.h"
#include "ClassManager.h"
#include "PrismaUIManager.h"
#include "FollowerManager.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <algorithm>
#include <cmath>

namespace Classbound
{
	namespace
	{
		constexpr std::uint32_t kSaddledBrownHorseFormID = 0x00023AB2;
		constexpr std::uint32_t kPlayerHorseFactionFormID = 0x00068D78;
	constexpr std::uint32_t kCurrentFollowerFactionFormID = 0x0005C84D;
	constexpr std::uint32_t kAHLFollowPackageLocalFormID = 0x00000B13;
	constexpr std::uint32_t kAHLOwnedHorsesListLocalFormID = 0x00000B11;
	constexpr std::uint32_t kAHLHorseFactionFirstLocalFormID = 0x00000D42;
	constexpr const char* kAHLPlugin = "AHorsesLife.esp";
	constexpr std::uint32_t kADLCurrentDogFactionLocalFormID = 0x0000AA16;
	constexpr std::uint32_t kMaleMerchantGuardLocalFormID = 0x00000800;
	constexpr std::uint32_t kFemaleMerchantGuardLocalFormID = 0x00000801;
	constexpr const char* kMerchantGuardPlugin = "ClassBoundGuards.esp";
	constexpr std::uint32_t kADogLifeFollowerDogFormID = 0x00109487;
	constexpr std::uint32_t kADogLifeDogFollowPackageLocalFormID = 0x00033268;
	constexpr const char* kADogLifePlugin = "ADogsLife.esp";

	template <class T>
	T* LookupLightPluginForm(RE::TESDataHandler* dataHandler, std::uint32_t localFormID, const char* pluginName)
	{
		if (!dataHandler) return nullptr;
		if (auto* form = dataHandler->LookupForm<T>(localFormID, pluginName)) return form;

		// ESL-flagged plugins use the FE:light-index:local-ID layout at runtime.
		if (const auto lightIndex = dataHandler->GetLoadedLightModIndex(pluginName)) {
			const RE::FormID runtimeFormID = 0xFE000000u |
				(static_cast<RE::FormID>(*lightIndex) << 12) | (localFormID & 0xFFFu);
			if (auto* form = RE::TESForm::LookupByID(runtimeFormID)) return form->As<T>();
		}
		return nullptr;
	}

	RE::TESNPC* FindNPCByEditorID(RE::TESDataHandler* dataHandler, std::string_view editorID)
	{
		if (!dataHandler) return nullptr;
		for (auto* npc : dataHandler->GetFormArray<RE::TESNPC>()) {
			if (npc && editorID == npc->GetFormEditorID()) return npc;
		}
		return nullptr;
	}

	bool IntegrateHorseWithAHL(RE::TESDataHandler* dataHandler, RE::Actor* horse)
	{
		if (!dataHandler || !horse ||
			!(dataHandler->LookupLoadedModByName(kAHLPlugin) || dataHandler->LookupLoadedLightModByName(kAHLPlugin))) return false;

		auto* followPackage = LookupLightPluginForm<RE::TESPackage>(dataHandler, kAHLFollowPackageLocalFormID, kAHLPlugin);
		auto* ownedHorses = LookupLightPluginForm<RE::BGSListForm>(dataHandler, kAHLOwnedHorsesListLocalFormID, kAHLPlugin);
		if (!followPackage || !ownedHorses) {
			SKSE::log::warn("StartingGearManager: A Horse's Life follow package or owned-horse list could not be resolved.");
			return false;
		}

		const bool alreadyOwned = ownedHorses->HasForm(horse);
		const auto ownedCount = static_cast<std::uint32_t>(ownedHorses->forms.size());
		std::uint32_t slot = ownedCount < 10 ? ownedCount : 9;
		RE::TESFaction* ahlHorseFaction = nullptr;
		for (std::uint32_t i = 0; i < 10; ++i) {
			auto* candidate = LookupLightPluginForm<RE::TESFaction>(dataHandler, kAHLHorseFactionFirstLocalFormID + i, kAHLPlugin);
			if (candidate && horse->IsInFaction(candidate)) {
				slot = i;
				ahlHorseFaction = candidate;
				break;
			}
		}
		if (!ahlHorseFaction) {
			ahlHorseFaction = LookupLightPluginForm<RE::TESFaction>(
				dataHandler, kAHLHorseFactionFirstLocalFormID + slot, kAHLPlugin);
			if (ahlHorseFaction) horse->AddToFaction(ahlHorseFaction, 0);
		}
		if (auto* followerFaction = dataHandler->LookupForm<RE::TESFaction>(kCurrentFollowerFactionFormID, "Skyrim.esm")) {
			horse->AddToFaction(followerFaction, 0);
		}
		if (auto* playerHorseFaction = dataHandler->LookupForm<RE::TESFaction>(kPlayerHorseFactionFormID, "Skyrim.esm")) {
			horse->AddToFaction(playerHorseFaction, 0);
		}
		horse->GetActorRuntimeData().boolBits.set(RE::Actor::BOOL_BITS::kPlayerTeammate);
		if (!alreadyOwned) {
			horse->PutCreatedPackage(followPackage, true, true, false);
			ownedHorses->AddForm(horse);
			SKSE::log::info("StartingGearManager: Registered class horse {:08X} in AHLFormOwnedHorses and AHLFactionHorse{:02}.",
				horse->GetFormID(), slot + 1);
		} else {
			SKSE::log::info("StartingGearManager: Reconciled existing A Horse's Life horse {:08X} (slot {}).",
				horse->GetFormID(), slot + 1);
		}
		horse->EvaluatePackage(true, true);
		return true;
	}

	void RegisterDogAsADLFollower(RE::TESDataHandler* dataHandler, RE::Actor* dog)
	{
		if (!dataHandler || !dog) return;
		if (auto* followerFaction = dataHandler->LookupForm<RE::TESFaction>(kCurrentFollowerFactionFormID, "Skyrim.esm")) {
			dog->AddToFaction(followerFaction, 0);
		}
		auto* adlCurrentDogFaction = dataHandler->LookupForm<RE::TESFaction>(kADLCurrentDogFactionLocalFormID, kADogLifePlugin);
		const bool alreadyAdopted = adlCurrentDogFaction && dog->IsInFaction(adlCurrentDogFaction);
		if (adlCurrentDogFaction && !alreadyAdopted) dog->AddToFaction(adlCurrentDogFaction, 0);
		if (!alreadyAdopted) {
			if (auto* followPackage = dataHandler->LookupForm<RE::TESPackage>(kADogLifeDogFollowPackageLocalFormID, kADogLifePlugin)) {
				dog->PutCreatedPackage(followPackage, true, true, false);
			} else {
				SKSE::log::warn("StartingGearManager: ADLPackageDogFollow could not be resolved.");
			}
		}
		dog->GetActorRuntimeData().boolBits.set(RE::Actor::BOOL_BITS::kPlayerTeammate);
		dog->EvaluatePackage(true, true);
		SKSE::log::info("StartingGearManager: Registered class dog {:08X} as an A Dog's Life follower (ADLCurrentDogFaction={}).",
			dog->GetFormID(), adlCurrentDogFaction != nullptr);
	}
	}

	std::vector<StartingItem> StartingGearManager::GetGearForClass(ClassType type) const
	{
		switch (type) {
		// ── MELEE CLASSES ───────────────────────────────────────────────────
		// Design rules: exactly ONE starter weapon per class (Assassin is the
		// sole dual-wield exception) plus a complete apparel set: body armor /
		// robes, boots, handwear and headwear.
		case ClassType::Warrior:
			return {
				{ 0x0001359D, 1, "Iron Greatsword" },     // 2H Iron Greatsword
				{ 0x00012E49, 1, "Iron Armor" },          // Heavy Iron Armor
				{ 0x00012E4B, 1, "Iron Boots" },          // Iron Boots
				{ 0x00012E46, 1, "Iron Gauntlets" },      // Iron Gauntlets
				{ 0x00012E4D, 1, "Iron Helmet" },         // Iron Helmet
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
			};

		case ClassType::Knight:
			return {
				{ 0x00153BA6, 1, "Iron Pike", "NewArmoury.esp", "Skyrim.esm", 0x00012EB7 }, // Iron Pike (fallback: Iron Sword)
				{ 0x00012EB7, 1, "Iron Sword" },          // 1H Iron Sword
				{ 0x00012EB6, 1, "Iron Shield" },         // Iron Shield
				{ 0x00012E49, 1, "Iron Armor" },          // Heavy Iron Armor
				{ 0x00012E4B, 1, "Iron Boots" },          // Iron Boots
				{ 0x00012E46, 1, "Iron Gauntlets" },      // Iron Gauntlets
				{ 0x00012E4D, 1, "Iron Helmet" },         // Iron Helmet
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
			};

		case ClassType::Paladin:
			return {
				{ 0x00013790, 1, "Iron War Axe" },        // 1H Iron War Axe
				{ 0x00012EB6, 1, "Iron Shield" },         // Iron Shield
				{ 0x00012E49, 1, "Iron Armor" },          // Heavy Iron Armor
				{ 0x00012E4B, 1, "Iron Boots" },          // Iron Boots
				{ 0x00012E46, 1, "Iron Gauntlets" },      // Iron Gauntlets
				{ 0x00012E4D, 1, "Iron Helmet" },         // Iron Helmet
				{ 0x0003EADD, 5, "Potion of Minor Healing" },
				{ 0x0003EAE0, 3, "Potion of Minor Magicka" }
			};

		case ClassType::Thief:
			return {
				{ 0x0001397E, 1, "Iron Dagger" },          // 1x Iron Dagger
				{ 0x0003619E, 1, "Leather Armor" },        // Leather Armor
				{ 0x00013920, 1, "Leather Boots" },        // Leather Boots
				{ 0x00013921, 1, "Leather Bracers" },      // Leather Bracers (handwear)
				{ 0x00013922, 1, "Leather Helmet" },       // Leather Helmet
				{ 0x0000000A, 30, "Lockpick" },            // 30x Lockpick
				{ 0x0003EAE5, 5, "Potion of Minor Stamina" }, // 5x Stamina Potion
				{ 0x00039BE5, 3, "Potion of Sneaking" }    // 3x Sneak Potion (Light Foot)
			};

		case ClassType::Assassin:
			// Dual-wield exception: the only class granted two weapons.
			return {
				{ 0x0019AAA5, 1, "Iron Claws", "NewArmoury.esp", "Skyrim.esm", 0x0001397E },       // Right Claw (fallback: Iron Dagger)
				{ 0x0019AAE2, 1, "Iron Claws Left", "NewArmoury.esp", "Skyrim.esm", 0x0001397E },  // Left Claw (fallback: Iron Dagger)
				{ 0x00013911, 1, "Hide Armor" },           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },          // Hide Helmet
				{ 0x00073F38, 10, "Frostbite Venom" },     // 10x Poison
				{ 0x0003EB3F, 3, "Potion of Invisibility" } // 3x Invisibility Potion
			};

		// ── RANGED CLASSES ──────────────────────────────────────────────────
		case ClassType::Archer:
			return {
				{ 0x0003B562, 1, "Long Bow" },             // Long Bow
				{ 0x0001397D, 75, "Steel Arrow" },         // 75x Steel Arrow
				{ 0x00013911, 1, "Hide Armor" },           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },          // Hide Helmet
				{ 0x0003EAE5, 3, "Potion of Minor Stamina" }
			};

		case ClassType::Hunter:
			return {
				{ 0x00013985, 1, "Hunting Bow" },          // Hunting Bow
				{ 0x0001397D, 60, "Steel Arrow" },         // 60x Steel Arrow
				{ 0x00013911, 1, "Hide Armor" },           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },          // Hide Helmet
				{ 0x00034CDD, 3, "Torch" },                // Torches
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
			};

		case ClassType::Marksman:
			return {
				{ 0x00013985, 1, "Hunting Bow" },          // Hunting Bow (Sniper setup)
				{ 0x0001397D, 100, "Steel Arrow" },        // 100x Steel Arrow
				{ 0x0003619E, 1, "Leather Armor" },        // Leather Armor
				{ 0x00013920, 1, "Leather Boots" },        // Leather Boots
				{ 0x00013921, 1, "Leather Bracers" },      // Leather Bracers (handwear)
				{ 0x00013922, 1, "Leather Helmet" },       // Leather Helmet
				{ 0x0003EAE5, 5, "Potion of Minor Stamina" }
			};

		case ClassType::Gunslinger:
			return {
				{ 0x00000809, 1, "Iron Handgun", "HandgunsOfSkyrim.esp", "Skyrim.esm", 0x00012EB7 }, // Iron Handgun (fallback: Iron Sword)
				{ 0x00000812, 100, "Common Lead Shot", "HandgunsOfSkyrim.esp" }, // 100x Common Lead Shot
				{ 0x00013911, 1, "Hide Armor" },                           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },                           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },                         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },                          // Hide Helmet
				{ 0x0003EADD, 3, "Potion of Minor Healing" },
				{ 0x0003EAE5, 3, "Potion of Minor Stamina" }
			};

		case ClassType::Crossbowman:
			// Dawnguard.esm local FormIDs: Crossbow (0x000801) & Steel Bolt (0x000BB3)
			return {
				{ 0x00000801, 1, "Crossbow", "Dawnguard.esm" },
				{ 0x00000BB3, 60, "Steel Bolt", "Dawnguard.esm" },
				{ 0x00013911, 1, "Hide Armor" },           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },          // Hide Helmet
				{ 0x0003EAE5, 4, "Potion of Minor Stamina" }
			};

		// ── MAGIC CLASSES ───────────────────────────────────────────────────
		case ClassType::Mage:
			return {
				{ 0x0004DEE2, 1, "Staff of Sparks" },      // Starter Magic Staff: Sparks (Shock Damage)
				{ 0x0010D667, 1, "Novice Robes" },         // Novice Robes (Mage Clothes)
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000D3DE8, 1, "Mage Hood" },            // Mage Hood (headwear)
				{ 0x0003EAE0, 5, "Potion of Minor Magicka" },
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
			};

		case ClassType::Necromancer:
			return {
				{ 0x0001397E, 1, "Iron Dagger" },          // Ritual Dagger
				{ 0x000C36EA, 1, "Necromancer Robes" },    // Necromancer Robes
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000D3DE8, 1, "Mage Hood" },            // Mage Hood (headwear)
				{ 0x0002E4E2, 3, "Petty Soul Gem" },       // Soul gems
				{ 0x0003EAE0, 5, "Potion of Minor Magicka" }
			};

		case ClassType::Battlemage:
			return {
				{ 0x00012EB7, 1, "Iron Sword" },           // 1H Sword
				{ 0x00012E49, 1, "Iron Armor" },           // Heavy Armor
				{ 0x00012E4B, 1, "Iron Boots" },           // Heavy Boots
				{ 0x00012E46, 1, "Iron Gauntlets" },       // Iron Gauntlets (handwear)
				{ 0x00012E4D, 1, "Iron Helmet" },          // Iron Helmet
				{ 0x0003EAE0, 4, "Potion of Minor Magicka" },
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
			};

		case ClassType::Druid:
			return {
				{ 0x00013790, 1, "Iron War Axe" },         // Nature Axe
				{ 0x0010D667, 1, "Novice Robes" },         // Robes
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000D3DE8, 1, "Mage Hood" },            // Mage Hood (headwear)
				{ 0x0003EAE0, 4, "Potion of Minor Magicka" },
				{ 0x0003EADD, 4, "Potion of Minor Healing" }
			};

		case ClassType::Elementalist:
			return {
				{ 0x0010D667, 1, "Novice Robes" },         // Novice Robes (no weapon: pure caster)
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000D3DE8, 1, "Mage Hood" },            // Mage Hood (headwear)
				{ 0x0003EAE0, 8, "Potion of Minor Magicka" }
			};

		// ── SOCIAL & LIFESTYLE CLASSES ──────────────────────────────────────
		case ClassType::Merchant:
			return {
				{ 0x0001397E, 1, "Iron Dagger" },          // Self-defense Dagger
				{ 0x000CEE80, 1, "Fine Clothes" },         // Fine Clothes
				{ 0x00086993, 1, "Fine Boots" },           // Fine Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000CEE84, 1, "Fine Hat" },             // Fine Hat (headwear)
				{ 0x0000000F, 1000, "Gold" },              // 1000 Starting Gold
				{ 0x00063B46, 2, "Garnet" },               // Gems
				{ 0x00063B47, 2, "Amethyst" },
				{ 0x00063B45, 1, "Ruby" },
				{ 0x00063B43, 1, "Sapphire" }
			};

		case ClassType::Bard:
			return {
				{ 0x0001397E, 1, "Iron Dagger" },          // Dagger (flute/lute are instruments, not weapons)
				{ 0x000D191F, 1, "Tavern Clothes" },       // Bard/Tavern Attire
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000330B3, 1, "Hat" },                  // Hat (headwear)
				{ 0x000DABA7, 1, "Flute" },                // Musical Instrument
				{ 0x000DABAB, 1, "Lute" },                 // Musical Instrument
				{ 0x0000000F, 100, "Gold" }                // Performance Tips
			};

		case ClassType::Alchemist:
			return {
				{ 0x0001397E, 1, "Iron Dagger" },          // Dagger
				{ 0x000CEE80, 1, "Fine Clothes" },         // Fine Clothes
				{ 0x00086993, 1, "Fine Boots" },           // Fine Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x000D3DE8, 1, "Mage Hood" },            // Mage Hood (headwear)
				{ 0x0003EADD, 4, "Potion of Minor Healing" },
				{ 0x0003EAE0, 4, "Potion of Minor Magicka" },
				{ 0x0003EAE5, 4, "Potion of Minor Stamina" },
				{ 0x00073F38, 3, "Frostbite Venom" },      // Poisons
				{ 0x00073F34, 1, "Poison Recipe Note 1" }, // Poison Recipes
				{ 0x00073F35, 1, "Poison Recipe Note 2" }
			};

		case ClassType::Blacksmith:
			return {
				{ 0x00013982, 1, "Iron Mace" },            // Heavy Iron Mace
				{ 0x0005B69F, 1, "Blacksmith's Apron" },   // Apron
				{ 0x0001BE1B, 1, "Boots" },                // Plain Boots
				{ 0x000261C1, 1, "Gloves" },               // Plain Gloves (handwear)
				{ 0x00013922, 1, "Leather Helmet" },       // Leather Helmet (headwear)
				{ 0x0005ACE4, 10, "Iron Ingot" },          // 10x Iron Ingot
				{ 0x0005ACE5, 5, "Steel Ingot" },          // 5x Steel Ingot
				{ 0x000800E4, 5, "Leather Strips" },       // Leather Strips
				{ 0x000DB5D2, 5, "Leather" },              // 5x Leather
				{ 0x0006F993, 6, "Firewood" }              // 6x Firewood
			};

		case ClassType::Explorer:
			return {
				{ 0x00012EB7, 1, "Iron Sword" },           // 1H Sword
				{ 0x0006F393, 1, "Fur Armor" },            // Fur Armor (rugged travel look)
				{ 0x000A6D7F, 1, "Fur Boots" },            // Fur Boots
				{ 0x000A6D7D, 1, "Fur Gauntlets" },        // Fur Gauntlets (handwear)
				{ 0x0006F39E, 1, "Fur Helmet" },           // Fur Helmet
				{ 0x00034CDD, 5, "Torch" },                // 5x Torch
				{ 0x0000000A, 10, "Lockpick" },            // 10x Lockpick
				{ 0x0003EADD, 3, "Potion of Minor Healing" },
				{ 0x0000080A, 1, "Adventurer Backpack with Bedroll", "ccfsvsse001-backpacks.esl" } // CC Backpack (fallback to null if CC missing)
			};

		default:
			return {};
		}
	}

	void StartingGearManager::AddItemToActor(RE::Actor* actor, uint32_t formID, uint32_t count,
		const char* sourcePlugin, const char* fallbackPlugin, uint32_t fallbackFormID)
	{
		if (!actor || count == 0) return;

		RE::TESForm* form = nullptr;
		auto* dataHandler = RE::TESDataHandler::GetSingleton();

		if (sourcePlugin != nullptr) {
			if (dataHandler) {
				form = dataHandler->LookupForm(formID, sourcePlugin);
			}
		} else {
			form = RE::TESForm::LookupByID(formID);
			// Fallback: try looking up in Skyrim.esm if LookupByID didn't find it
			if (!form && dataHandler) {
				form = dataHandler->LookupForm(formID, "Skyrim.esm");
			}
		}

		// Secondary fallback: try fallbackPlugin/fallbackFormID if primary lookup failed
		if (!form && fallbackPlugin != nullptr && fallbackFormID != 0 && dataHandler) {
			SKSE::log::warn("StartingGearManager: Primary lookup failed for 0x{:08X} ({}), trying fallback 0x{:08X} ({})",
				formID, sourcePlugin ? sourcePlugin : "Skyrim.esm", fallbackFormID, fallbackPlugin);
			form = dataHandler->LookupForm(fallbackFormID, fallbackPlugin);
		}

		if (!form) {
			SKSE::log::warn("StartingGearManager: FAILED to find FormID 0x{:08X} (plugin: {}) for {}",
				formID, sourcePlugin ? sourcePlugin : "Skyrim.esm", actor->GetName());
			return;
		}

		auto* boundObj = form->As<RE::TESBoundObject>();
		if (!boundObj) {
			SKSE::log::warn("StartingGearManager: FormID 0x{:08X} is not a TESBoundObject for {}",
				formID, actor->GetName());
			return;
		}

		actor->AddObjectToContainer(boundObj, nullptr, static_cast<int32_t>(count), nullptr);
		SKSE::log::info("StartingGearManager: Added {}x {} (0x{:08X}) to {}",
			count, boundObj->GetName(), formID, actor->GetName());
	}

	void StartingGearManager::AddSpellToActor(RE::Actor* actor, uint32_t spellFormID)
	{
		if (!actor) return;

		RE::TESForm* form = RE::TESForm::LookupByID(spellFormID);
		if (!form) {
			if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
				form = dataHandler->LookupForm(spellFormID, "Skyrim.esm");
			}
		}

		if (!form) {
			SKSE::log::warn("StartingGearManager: FAILED to find Spell FormID 0x{:08X} for {}",
				spellFormID, actor->GetName());
			return;
		}

		if (auto* spell = form->As<RE::SpellItem>()) {
			actor->AddSpell(spell);
			SKSE::log::info("StartingGearManager: Added spell {} (0x{:08X}) to {}",
				spell->GetName(), spellFormID, actor->GetName());
		}
	}

	void StartingGearManager::GrantStartingGearToActor(RE::Actor* actor, ClassType classType)
	{
		if (!actor) return;

		auto gear = GetGearForClass(classType);
		if (gear.empty()) return;

		for (const auto& item : gear) {
			AddItemToActor(actor, item.formID, item.count, item.sourcePlugin,
				item.fallbackPlugin, item.fallbackFormID);
		}

		// Class-specific starter spells (if not already known)
		switch (classType) {
		case ClassType::Mage:
		case ClassType::Elementalist:
			AddSpellToActor(actor, 0x00012FD0); // Flames
			AddSpellToActor(actor, 0x0002B96B); // Frostbite
			AddSpellToActor(actor, 0x0002DD2A); // Sparks
			AddItemToActor(actor, 0x000A26FD, 1); // Spell Tome: Firebolt (attack)
			AddItemToActor(actor, 0x0009E2A8, 1); // Spell Tome: Oakflesh (defense)
			break;

		case ClassType::Necromancer:
			AddSpellToActor(actor, 0x000211EB); // Raise Zombie
			AddSpellToActor(actor, 0x0004E3CF); // Conjure Familiar
			AddItemToActor(actor, 0x0009E2AA, 1); // Spell Tome: Raise Zombie (attack/summon)
			AddItemToActor(actor, 0x0009E2A8, 1); // Spell Tome: Oakflesh (defense)
			break;

		case ClassType::Battlemage:
			AddItemToActor(actor, 0x0009E2A9, 1); // Spell Tome: Bound Sword (attack)
			AddItemToActor(actor, 0x0009E2A8, 1); // Spell Tome: Oakflesh (defense)
			break;

		case ClassType::Paladin:
			AddSpellToActor(actor, 0x00012FCC); // Healing
			AddSpellToActor(actor, 0x0004D3F2); // Lesser Ward
			AddItemToActor(actor, 0x000A26FD, 1); // Spell Tome: Firebolt (attack)
			AddItemToActor(actor, 0x0009E2AE, 1); // Spell Tome: Lesser Ward (defense)
			break;

		case ClassType::Druid:
			AddSpellToActor(actor, 0x00012FCC); // Healing
			AddSpellToActor(actor, 0x00012FC6); // Healing Hands (heal others)
			AddSpellToActor(actor, 0x0004E3CF); // Conjure Familiar
			AddItemToActor(actor, 0x0009E2AB, 1); // Spell Tome: Conjure Familiar (attack/summon)
			AddItemToActor(actor, 0x000A271E, 1); // Spell Tome: Healing Hands (defense)
			break;

		case ClassType::Bard:
			AddSpellToActor(actor, 0x0004DEE8); // Courage
			AddSpellToActor(actor, 0x0005AD5C); // Clairvoyance
			break;

		default:
			break;
		}

		SKSE::log::info("StartingGearManager: Distributed starter pack for class {} to {}", 
			static_cast<uint32_t>(classType), actor->GetName());
	}

	void StartingGearManager::SpawnHorseForPlayer()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			SKSE::log::warn("StartingGearManager: Cannot summon Knight horse; player is unavailable.");
			return;
		}

		auto* cell = player->GetParentCell();
		if (!cell) {
			SKSE::log::warn("StartingGearManager: Cannot summon Knight horse; player has no parent cell yet.");
			return;
		}

		// Vanilla saddled brown horse base form.
		auto* horseForm = RE::TESForm::LookupByID(kSaddledBrownHorseFormID);
		auto* horseBase = horseForm ? horseForm->As<RE::TESNPC>() : nullptr;
		if (!horseBase) {
			if (auto* dh = RE::TESDataHandler::GetSingleton()) {
				horseBase = dh->LookupForm<RE::TESNPC>(kSaddledBrownHorseFormID, "Skyrim.esm");
			}
		}

		if (!horseBase) {
			SKSE::log::warn("StartingGearManager: Could not find saddled horse base form 0x{:08X} in Skyrim.esm.", kSaddledBrownHorseFormID);
			return;
		}

		auto placedRefr = player->PlaceObjectAtMe(horseBase, false);
		if (!placedRefr) {
			SKSE::log::warn("StartingGearManager: PlaceObjectAtMe failed for Knight horse.");
			return;
		}

		auto* horseActor = placedRefr->As<RE::Actor>();
		if (!horseActor) {
			SKSE::log::warn("StartingGearManager: Placed Knight horse reference is not an actor.");
			return;
		}

		const bool ahlIntegrated = IntegrateHorseWithAHL(RE::TESDataHandler::GetSingleton(), horseActor);
		if (!ahlIntegrated) {
			SKSE::log::info("StartingGearManager: Spawned a class horse without A Horse's Life integration.");
		}
		PrismaUIManager::Notify("Your horse has arrived!", "success");
	}

	void StartingGearManager::SpawnMerchantGuards()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!player || !dataHandler || !player->GetParentCell()) {
			SKSE::log::warn("StartingGearManager: Cannot spawn Merchant guards; player or cell is unavailable.");
			return;
		}

		auto* maleBase = LookupLightPluginForm<RE::TESNPC>(dataHandler, kMaleMerchantGuardLocalFormID, kMerchantGuardPlugin);
		auto* femaleBase = LookupLightPluginForm<RE::TESNPC>(dataHandler, kFemaleMerchantGuardLocalFormID, kMerchantGuardPlugin);
		if (!maleBase) maleBase = FindNPCByEditorID(dataHandler, "ClassBoundMaleGuard");
		if (!femaleBase) femaleBase = FindNPCByEditorID(dataHandler, "ClassBoundFemaleGuard");
		if (!maleBase || !femaleBase) {
			const auto lightIndex = dataHandler->GetLoadedLightModIndex(kMerchantGuardPlugin);
			const auto fullIndex = dataHandler->GetLoadedModIndex(kMerchantGuardPlugin);
			SKSE::log::error("StartingGearManager: Guard forms unresolved (loaded={}, lightIndex={}, fullIndex={}, male={}, female={}). Check plugin records in {}.",
				dataHandler->LookupLoadedLightModByName(kMerchantGuardPlugin) != nullptr || dataHandler->LookupLoadedModByName(kMerchantGuardPlugin) != nullptr,
				lightIndex ? std::to_string(*lightIndex) : "none",
				fullIndex ? std::to_string(*fullIndex) : "none",
				maleBase != nullptr, femaleBase != nullptr, kMerchantGuardPlugin);
			return;
		}

		bool maleAlreadySpawned = false;
		bool femaleAlreadySpawned = false;
		if (auto* processLists = RE::ProcessLists::GetSingleton()) {
			processLists->ForAllActors([&](RE::Actor* actor) {
				if (!actor) return RE::BSContainer::ForEachResult::kContinue;
				const auto* base = actor->GetActorBase();
				if (base == maleBase) maleAlreadySpawned = true;
				if (base == femaleBase) femaleAlreadySpawned = true;
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

		auto* followerFaction = RE::TESForm::LookupByID<RE::TESFaction>(kCurrentFollowerFactionFormID);
		if (!followerFaction) followerFaction = dataHandler->LookupForm<RE::TESFaction>(kCurrentFollowerFactionFormID, "Skyrim.esm");

		auto spawnGuard = [&](RE::TESNPC* base, bool alreadySpawned, float sideOffset, const char* label) {
			if (alreadySpawned) return;
			auto placed = player->PlaceObjectAtMe(base, true);
			if (!placed) {
				SKSE::log::warn("StartingGearManager: Failed to spawn {} Merchant guard.", label);
				return;
			}
			auto* guard = placed->As<RE::Actor>();
			if (!guard) {
				SKSE::log::warn("StartingGearManager: Spawned {} guard reference is not an actor.", label);
				return;
			}

			const auto playerPosition = player->GetPosition();
			const float angle = player->GetAngleZ();
			RE::NiPoint3 guardPosition = playerPosition;
			guardPosition.x += sideOffset * std::cos(angle) + 110.0f * std::sin(angle);
			guardPosition.y -= sideOffset * std::sin(angle) - 110.0f * std::cos(angle);
			guard->SetPosition(guardPosition, true);

			// Register the actor as a vanilla-style teammate so Skyrim's built-in
			// DefaultFollowPlayer package and ClassBound's follower scanner recognize it.
			if (followerFaction) guard->AddToFaction(followerFaction, 0);
			guard->GetActorRuntimeData().boolBits.set(RE::Actor::BOOL_BITS::kPlayerTeammate);
			guard->EvaluatePackage(true, true);

			// Equip weapons & armor on the guard using ActorEquipManager
			auto* equipManager = RE::ActorEquipManager::GetSingleton();
			if (equipManager) {
				// Steel armor set: Cuirass, Gauntlets, Boots, Helmet
				const std::uint32_t steelArmor[] = { 0x00013952, 0x00013953, 0x00013951, 0x00013954 };
				for (const auto formID : steelArmor) {
					auto* form = RE::TESForm::LookupByID(formID);
					if (!form) form = dataHandler->LookupForm(formID, "Skyrim.esm");
					if (auto* item = form ? form->As<RE::TESBoundObject>() : nullptr) {
						guard->AddObjectToContainer(item, nullptr, 1, nullptr);
						equipManager->EquipObject(guard, item, nullptr, 1, nullptr, true, true, false, false);
					}
				}

				if (std::strcmp(label, "male") == 0) {
					// Male guard: Steel Sword & Steel Shield
					const std::uint32_t meleeGear[] = { 0x00013989, 0x00013955 };
					for (const auto formID : meleeGear) {
						auto* form = RE::TESForm::LookupByID(formID);
						if (!form) form = dataHandler->LookupForm(formID, "Skyrim.esm");
						if (auto* item = form ? form->As<RE::TESBoundObject>() : nullptr) {
							guard->AddObjectToContainer(item, nullptr, 1, nullptr);
							equipManager->EquipObject(guard, item, nullptr, 1, nullptr, true, true, false, false);
						}
					}
				} else {
					// Female guard: Hunting Bow, Steel Arrows, Steel Sword
					auto* bowForm = RE::TESForm::LookupByID(0x00013985);
					if (!bowForm) bowForm = dataHandler->LookupForm(0x00013985, "Skyrim.esm");
					if (auto* bow = bowForm ? bowForm->As<RE::TESBoundObject>() : nullptr) {
						guard->AddObjectToContainer(bow, nullptr, 1, nullptr);
						equipManager->EquipObject(guard, bow, nullptr, 1, nullptr, true, true, false, false);
					}

					auto* arrowForm = RE::TESForm::LookupByID(0x0001397F);
					if (!arrowForm) arrowForm = dataHandler->LookupForm(0x0001397F, "Skyrim.esm");
					if (auto* arrows = arrowForm ? arrowForm->As<RE::TESBoundObject>() : nullptr) {
						guard->AddObjectToContainer(arrows, nullptr, 50, nullptr);
						equipManager->EquipObject(guard, arrows, nullptr, 50, nullptr, true, true, false, false);
					}

					auto* swordForm = RE::TESForm::LookupByID(0x00013989);
					if (!swordForm) swordForm = dataHandler->LookupForm(0x00013989, "Skyrim.esm");
					if (auto* sword = swordForm ? swordForm->As<RE::TESBoundObject>() : nullptr) {
						guard->AddObjectToContainer(sword, nullptr, 1, nullptr);
					}
				}
			}

			SKSE::log::info("StartingGearManager: Spawned level-one {} Merchant bodyguard {}.", label, guard->GetName());
		};

		spawnGuard(maleBase, maleAlreadySpawned, -65.0f, "male");
		spawnGuard(femaleBase, femaleAlreadySpawned, 65.0f, "female");
		FollowerManager::GetSingleton()->ScanActiveFollowers(true);
		PrismaUIManager::Notify("Two Merchant Bodyguards have joined your caravan.", "success");
	}

	void StartingGearManager::RestoreClassCompanionsAfterLoad()
	{
		const auto currentClass = ClassManager::GetSingleton()->GetPlayerData().currentClass;
		if (currentClass == ClassType::Merchant) {
			SpawnMerchantGuards();
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!dataHandler || !processLists) return;

		if (currentClass == ClassType::Knight || currentClass == ClassType::Merchant) {
			auto* playerHorseFaction = dataHandler->LookupForm<RE::TESFaction>(kPlayerHorseFactionFormID, "Skyrim.esm");
			if (playerHorseFaction) {
				processLists->ForAllActors([&](RE::Actor* actor) {
					if (actor && actor->IsInFaction(playerHorseFaction)) {
						IntegrateHorseWithAHL(dataHandler, actor);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		}

		if (currentClass == ClassType::Explorer || currentClass == ClassType::Druid) {
			auto* dogBase = dataHandler->LookupForm<RE::TESNPC>(kADogLifeFollowerDogFormID, kADogLifePlugin);
			if (!dogBase) {
				if (auto* form = RE::TESForm::LookupByID(kADogLifeFollowerDogFormID)) dogBase = form->As<RE::TESNPC>();
			}
			if (dogBase) {
				processLists->ForAllActors([&](RE::Actor* actor) {
					if (actor && actor->GetActorBase() == dogBase) RegisterDogAsADLFollower(dataHandler, actor);
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	void StartingGearManager::SpawnClassDogFollower()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!player || !dataHandler || !player->GetParentCell()) {
			SKSE::log::warn("StartingGearManager: Cannot spawn class dog; player or cell is unavailable.");
			return;
		}

		auto* dogBase = dataHandler->LookupForm<RE::TESNPC>(kADogLifeFollowerDogFormID, kADogLifePlugin);
		// WEFollowerDog is an override record in A Dog's Life whose saved FormID
		// is inherited from its master, so the plugin-local lookup may not resolve it.
		if (!dogBase) {
			if (auto* form = RE::TESForm::LookupByID(kADogLifeFollowerDogFormID)) dogBase = form->As<RE::TESNPC>();
		}
		if (!dogBase) {
			const auto lightIndex = dataHandler->GetLoadedLightModIndex(kADogLifePlugin);
			const auto fullIndex = dataHandler->GetLoadedModIndex(kADogLifePlugin);
			SKSE::log::warn("StartingGearManager: A Dog's Life dog unresolved (loaded={}, lightIndex={}, fullIndex={}); skipping class dog follower.",
				dataHandler->LookupLoadedModByName(kADogLifePlugin) != nullptr,
				lightIndex ? std::to_string(*lightIndex) : "none",
				fullIndex ? std::to_string(*fullIndex) : "none");
			return;
		}

		RE::Actor* existingDog = nullptr;
		if (auto* processLists = RE::ProcessLists::GetSingleton()) {
			processLists->ForAllActors([&](RE::Actor* actor) {
				if (actor && actor->GetActorBase() == dogBase) existingDog = actor;
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
		if (existingDog) {
			RegisterDogAsADLFollower(dataHandler, existingDog);
			SKSE::log::info("StartingGearManager: Existing A Dog's Life class dog was reconciled; no duplicate spawned.");
			return;
		}

		auto placed = player->PlaceObjectAtMe(dogBase, true);
		auto* dog = placed ? placed->As<RE::Actor>() : nullptr;
		if (!dog) {
			SKSE::log::warn("StartingGearManager: Failed to spawn A Dog's Life follower dog.");
			return;
		}

		const auto playerPosition = player->GetPosition();
		const float angle = player->GetAngleZ();
		RE::NiPoint3 dogPosition = playerPosition;
		dogPosition.x += 105.0f * std::sin(angle);
		dogPosition.y -= 105.0f * std::cos(angle);
		dog->SetPosition(dogPosition, true);

		RegisterDogAsADLFollower(dataHandler, dog);

		SKSE::log::info("StartingGearManager: Spawned A Dog's Life dog follower for Explorer/Druid.");
		FollowerManager::GetSingleton()->ScanActiveFollowers(true);
		PrismaUIManager::Notify("A loyal dog has joined your journey.", "success");
	}

	void StartingGearManager::GrantStartingGear(ClassType classType)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		GrantStartingGearToActor(player, classType);

		if (classType == ClassType::Knight || classType == ClassType::Merchant) {
			SpawnHorseForPlayer();
		}
		if (classType == ClassType::Merchant) {
			SpawnMerchantGuards();
		}
		if (classType == ClassType::Explorer || classType == ClassType::Druid) {
			SpawnClassDogFollower();
		}

		char msg[128];
		snprintf(msg, sizeof(msg), "Starting equipment & spells received for class!");
		PrismaUIManager::Notify(msg, "success");
	}
}
