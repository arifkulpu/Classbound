#include "StartingGearManager.h"
#include "PrismaUIManager.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace Classbound
{
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
				{ 0x0003BE12, 75, "Iron Arrow" },          // 75x Iron Arrow
				{ 0x00013911, 1, "Hide Armor" },           // Hide Armor
				{ 0x00013910, 1, "Hide Boots" },           // Hide Boots
				{ 0x00013912, 1, "Hide Bracers" },         // Hide Bracers (handwear)
				{ 0x00013913, 1, "Hide Helmet" },          // Hide Helmet
				{ 0x0003EAE5, 3, "Potion of Minor Stamina" }
			};

		case ClassType::Hunter:
			return {
				{ 0x00013985, 1, "Hunting Bow" },          // Hunting Bow
				{ 0x0003BE12, 60, "Iron Arrow" },          // 60x Iron Arrow
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
				{ 0x0003BE12, 100, "Iron Arrow" },         // 100x Iron Arrow
				{ 0x0003619E, 1, "Leather Armor" },        // Leather Armor
				{ 0x00013920, 1, "Leather Boots" },        // Leather Boots
				{ 0x00013921, 1, "Leather Bracers" },      // Leather Bracers (handwear)
				{ 0x00013922, 1, "Leather Helmet" },       // Leather Helmet
				{ 0x0003EAE5, 5, "Potion of Minor Stamina" }
			};

		case ClassType::Gunslinger:
			return {
				{ 0x0000082E, 1, "Iron Pistol", "PistolMod.esp", "Skyrim.esm", 0x00012EB7 }, // Iron Pistol (fallback: Iron Sword)
				{ 0x00000857, 24, "Pistol Bullets", "PistolMod.esp" },    // 24x Pistol Bullets
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
				{ 0x0000000F, 250, "Gold" }                // 250 Extra Starting Gold
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
				{ 0x00073F38, 3, "Frostbite Venom" }       // Poisons
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
				{ 0x000800E4, 5, "Leather Strips" }        // Leather Strips
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
				{ 0x0003EADD, 3, "Potion of Minor Healing" }
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
			break;

		case ClassType::Necromancer:
			AddSpellToActor(actor, 0x000211EB); // Raise Zombie
			AddSpellToActor(actor, 0x0004E3CF); // Conjure Familiar
			break;

		case ClassType::Paladin:
			AddSpellToActor(actor, 0x00012FCC); // Healing
			AddSpellToActor(actor, 0x0004D3F2); // Lesser Ward
			break;

		case ClassType::Druid:
			AddSpellToActor(actor, 0x00012FCC); // Healing
			AddSpellToActor(actor, 0x00012FC6); // Healing Hands (heal others)
			AddSpellToActor(actor, 0x0004E3CF); // Conjure Familiar
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

	void StartingGearManager::GrantStartingGear(ClassType classType)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		GrantStartingGearToActor(player, classType);

		char msg[128];
		snprintf(msg, sizeof(msg), "Starting equipment & spells received for class!");
		PrismaUIManager::Notify(msg, "success");
	}
}
