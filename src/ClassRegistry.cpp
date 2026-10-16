#include "ClassRegistry.h"
#include <SKSE/SKSE.h>

namespace Classbound
{

	void ClassRegistry::Initialize()
	{
		// AV Reference:
		// kOneHanded=6, kTwoHanded=7, kArchery=8, kBlock=9, kSmithing=10
		// kHeavyArmor=11, kLightArmor=12, kPickpocket=13, kLockpicking=14
		// kSneak=15, kAlchemy=16, kSpeech=17, kAlteration=18, kConjuration=19
		// kDestruction=20, kIllusion=21, kRestoration=22, kEnchanting=23
		// kHealth=24, kMagicka=25, kStamina=26

		// ── MELEE CLASSES ───────────────────────────────────────────────────

		{
			ClassDefinition d;
			d.type = ClassType::Warrior; d.category = MainCategory::Melee;
			d.name = "Warrior"; d.playstyle = "Melee Frontliner";
			d.icon = "[WAR]";
			d.description = "Powerful, resilient, and ruthless. A front-line fighter specializing in heavy armor and two-handed weaponry.";
			d.stats = { 9.0f, 8.0f, 1.0f, 2.0f, 2.0f };
			d.startingBonuses = { "Two-Handed Damage +10%", "Heavy Armor Efficiency +5%", "Physical Resistance +5%" };
			d.xpSources = {
				{ "Two-Handed Kill",       20  },
				{ "Heavy Armor Combat",    10  },
				{ "Boss Kill",             100 },
				{ "Block Successful Hit",   5  },
				{ "Power Attack Kill",      15  },
				{ "Dragon Kill",           150 },
			};
			d.avBonuses = {
				{ 24, 15.0f,  5.0f, "Health"      },
				{ 26, 10.0f,  3.0f, "Stamina"     },
				{  7,  5.0f,  1.0f, "Two-Handed"  },
				{ 11,  5.0f,  1.0f, "Heavy Armor" },
				{  9,  5.0f,  1.0f, "Block"       },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Knight; d.category = MainCategory::Melee;
			d.name = "Knight"; d.playstyle = "Sword & Shield Guardian";
			d.icon = "[KNG]";
			d.description = "Disciplined and honorable knight. Expert in shield defense, battlefield control, and one-handed swords.";
			d.stats = { 7.0f, 9.0f, 1.0f, 1.0f, 3.0f };
			d.startingBonuses = { "Shield Block +10%", "Companion Damage +5%", "Armor Rating +25" };
			d.xpSources = {
				{ "Shield Bash Kill",       20  },
				{ "Block & Counter",        10  },
				{ "Protect Ally",           15  },
				{ "Boss Kill (Shielded)",  100  },
				{ "One-Handed Kill",        10  },
				{ "Dungeon Cleared",        50  },
			};
			d.avBonuses = {
				{ 24, 15.0f,  5.0f, "Health"      },
				{ 26, 10.0f,  3.0f, "Stamina"     },
				{  6,  5.0f,  1.0f, "One-Handed"  },
				{  9,  5.0f,  1.0f, "Block"       },
				{ 11,  5.0f,  1.0f, "Heavy Armor" },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Paladin; d.category = MainCategory::Melee;
			d.name = "Paladin"; d.playstyle = "Holy Warrior";
			d.icon = "[PAL]";
			d.description = "A holy warrior combining martial swordplay with divine Restoration magic to purge the undead.";
			d.stats = { 7.0f, 8.0f, 5.0f, 1.0f, 2.0f };
			d.startingBonuses = { "Damage vs Undead +15%", "Restoration Cost -10%", "Disease Resistance +25%" };
			d.xpSources = {
				{ "Undead Destroyed",       25  },
				{ "Healing in Combat",      10  },
				{ "Vampire Kill",           40  },
				{ "Melee Kill",             10  },
				{ "Boss Kill",             100  },
			};
			d.avBonuses = {
				{ 24, 15.0f,  4.0f, "Health"      },
				{ 25, 15.0f,  4.0f, "Magicka"     },
				{  6,  5.0f,  1.0f, "One-Handed"  },
				{ 22,  5.0f,  1.0f, "Restoration" },
				{ 11,  5.0f,  1.0f, "Heavy Armor" },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Thief; d.category = MainCategory::Melee;
			d.name = "Thief"; d.playstyle = "Stealth & Agility";
			d.icon = "[THF]";
			d.description = "Operates from the shadows, picking locks and emptying pockets with supreme agility.";
			d.stats = { 3.0f, 3.0f, 1.0f, 9.0f, 7.0f };
			d.startingBonuses = { "Lockpicking Sweetspot +10%", "Pickpocket Chance +10%", "Footstep Noise -15%" };
			d.xpSources = {
				{ "Lock Picked",            15  },
				{ "Pickpocket Successful",  20  },
				{ "Sneak Takedown",         30  },
				{ "Chest Looted",           10  },
				{ "Stealth Escape",         15  },
			};
			d.avBonuses = {
				{ 26, 15.0f,  4.0f, "Stamina"     },
				{ 15,  6.0f,  1.0f, "Sneak"       },
				{ 14,  5.0f,  1.0f, "Lockpicking" },
				{ 13,  5.0f,  1.0f, "Pickpocket"  },
				{ 12,  4.0f,  1.0f, "Light Armor" },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Assassin; d.category = MainCategory::Melee;
			d.name = "Assassin"; d.playstyle = "One-Hit Executioner";
			d.icon = "[ASN]";
			d.description = "Deals silent death with lethal precision. Master of daggers, deadly poisons, and critical strikes.";
			d.stats = { 8.0f, 2.0f, 1.0f, 10.0f, 3.0f };
			d.startingBonuses = { "Sneak Dagger Damage +25%", "Poison Potency +15%", "Sneak Sprint Penalty -25%" };
			d.xpSources = {
				{ "Sneak Kill",             35  },
				{ "Poison Kill",            25  },
				{ "Critical Strike",        15  },
				{ "Contract Target Kill",   75  },
				{ "Silent Kill",            20  },
			};
			d.avBonuses = {
				{ 26, 12.0f,  3.0f, "Stamina"     },
				{ 15,  6.0f,  1.0f, "Sneak"       },
				{  6,  5.0f,  1.0f, "One-Handed"  },
				{ 16,  5.0f,  1.0f, "Alchemy"     },
				{ 12,  4.0f,  1.0f, "Light Armor" },
			};
			RegisterClass(d);
		}

		// ── RANGED CLASSES ──────────────────────────────────────────────────

		{
			ClassDefinition d;
			d.type = ClassType::Archer; d.category = MainCategory::Ranged;
			d.name = "Archer"; d.playstyle = "Ranged Marksman";
			d.icon = "[ARC]";
			d.description = "Master of the bow and arrow. Neutralizes foes from afar with speed and agility.";
			d.stats = { 8.0f, 3.0f, 1.0f, 6.0f, 3.0f };
			d.startingBonuses = { "Bow Damage +10%", "Arrow Velocity +10%", "Eagle Eye Zoom +15%" };
			d.xpSources = {
				{ "Archery Kill",           20  },
				{ "Long Distance Hit",      15  },
				{ "Headshot / Crit Hit",    20  },
				{ "Boss Kill (Bow)",       100  },
			};
			d.avBonuses = {
				{ 26, 15.0f,  4.0f, "Stamina"     },
				{  8,  6.0f,  1.0f, "Archery"     },
				{ 15,  5.0f,  1.0f, "Sneak"       },
				{ 12,  5.0f,  1.0f, "Light Armor" },
				{ 24, 10.0f,  2.0f, "Health"      },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Hunter; d.category = MainCategory::Ranged;
			d.name = "Hunter"; d.playstyle = "Tracker & Trapper";
			d.icon = "[HNT]";
			d.description = "Master of the wild. Tracks beasts, skins game, and lays cunning traps.";
			d.stats = { 6.0f, 4.0f, 1.0f, 7.0f, 4.0f };
			d.startingBonuses = { "Damage vs Animals +15%", "Pelt Harvesting +25%", "Poison & Disease Resistance +15%" };
			d.xpSources = {
				{ "Animal Hunted",          20  },
				{ "Archery Kill",           15  },
				{ "Large Beast Kill",       40  },
				{ "Pelt Gathered",          10  },
			};
			d.avBonuses = {
				{ 26, 15.0f,  4.0f, "Stamina"     },
				{  8,  5.0f,  1.0f, "Archery"     },
				{ 15,  5.0f,  1.0f, "Sneak"       },
				{ 16,  5.0f,  1.0f, "Alchemy"     },
				{ 24, 12.0f,  3.0f, "Health"      },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Marksman; d.category = MainCategory::Ranged;
			d.name = "Marksman"; d.playstyle = "Precision Sniper";
			d.icon = "[MRK]";
			d.description = "Precision shooting and armor-piercing specialist. Fells foes in a single calculated shot.";
			d.stats = { 9.0f, 2.0f, 1.0f, 8.0f, 2.0f };
			d.startingBonuses = { "Critical Strike Chance +10%", "Long Range Damage +15%", "Bow Steady Focus +25%" };
			d.xpSources = {
				{ "Archery Kill",           20  },
				{ "Crit Kill",              25  },
				{ "Long Distance Hit",      20  },
				{ "One-Shot Kill",          30  },
			};
			d.avBonuses = {
				{  8,  6.0f,  1.0f, "Archery"     },
				{ 26, 12.0f,  3.0f, "Stamina"     },
				{ 15,  6.0f,  1.0f, "Sneak"       },
				{ 12,  4.0f,  1.0f, "Light Armor" },
				{ 24, 10.0f,  2.0f, "Health"      },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Gunslinger; d.category = MainCategory::Ranged;
			d.name = "Gunslinger"; d.playstyle = "Pistol & Firearm Outlaw";
			d.icon = "[GUN]";
			d.description = "A swift and deadly firearms marksman wielding custom flintlock pistols and handguns. Deals devastating burst damage with lead and powder.";
			d.stats = { 7.0f, 4.0f, 1.0f, 6.0f, 4.0f };
			d.startingBonuses = { "Firearm Damage +15%", "Quickdraw Speed +15%", "Stamina Recovery +10%" };
			d.xpSources = {
				{ "Archery Kill",           20  },
				{ "Crit Kill",              25  },
				{ "Contract Target Kill",   35  },
				{ "Dungeon Cleared",        40  },
			};
			d.avBonuses = {
				{ 26, 20.0f,  5.0f, "Stamina"     },
				{  8,  8.0f,  2.0f, "Archery"     },
				{ 12,  5.0f,  1.0f, "Light Armor" },
				{ 24, 15.0f,  3.0f, "Health"      },
				{  6,  4.0f,  1.0f, "One-Handed"  },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Crossbowman; d.category = MainCategory::Ranged;
			d.name = "Crossbowman"; d.playstyle = "Heavy Crossbow Specialist";
			d.icon = "[XBW]";
			d.description = "A disciplined marksman who wields the crossbow with deadly precision. Slow to reload, but every bolt hits with bone-shattering power.";
			d.stats = { 6.0f, 5.0f, 0.0f, 6.0f, 2.0f };
			d.startingBonuses = { "Crossbow Damage +15%", "Armor Penetration +10%", "Reload Speed +10%" };
			d.xpSources = {
				{ "Archery Kill",         15  },
				{ "Headshot / Crit Hit",  30  },
				{ "Long Distance Hit",    20  },
				{ "Boss Kill (Bow)",      100 },
				{ "Crit Kill",            25  },
			};
			d.avBonuses = {
				{  8, 12.0f,  3.0f, "Archery"     },  // Archery
				{ 12,  8.0f,  2.0f, "Light Armor" },  // Light Armor
				{ 26, 10.0f,  2.0f, "Stamina"     },  // Stamina
				{  9,  5.0f,  1.0f, "Block"       },  // Block
			};
			RegisterClass(d);
		}

		// ── MAGIC CLASSES ───────────────────────────────────────────────────

		{
			ClassDefinition d;
			d.type = ClassType::Mage; d.category = MainCategory::Magic;
			d.name = "Mage"; d.playstyle = "Pure Arcane Mage";
			d.icon = "[MAG]";
			d.description = "A dedicated scholar of arcane mysteries. Channels raw magic through staff and spell, wielding arcane power that reshapes reality.";
			d.stats = { 2.0f, 2.0f, 10.0f, 2.0f, 4.0f };
			d.startingBonuses = { "Magicka Regen +25%", "All Spell Costs -5%", "Staff Damage +15%", "Dual-Cast Power +10%" };
			d.xpSources = {
				{ "Spell Cast",             5   },
				{ "Magic Kill",             20  },
				{ "New Spell Learned",      50  },
				{ "Boss Kill (Magic)",     100  },
				{ "Dual Cast Kill",         25  },
			};
			d.avBonuses = {
				{ 25, 20.0f,  6.0f, "Magicka"     },
				{ 20,  8.0f,  2.0f, "Destruction" },  // +2 more for staff synergy
				{ 18,  5.0f,  1.0f, "Alteration"  },
				{ 22,  5.0f,  1.0f, "Restoration" },
				{ 23,  4.0f,  1.0f, "Enchanting"  },
				{ 19,  4.0f,  1.0f, "Conjuration" },  // Staff of Conjuration synergy
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Necromancer; d.category = MainCategory::Magic;
			d.name = "Necromancer"; d.playstyle = "Dark Death Mage";
			d.icon = "[NEC]";
			d.description = "A practitioner of the dark arts who commands undead minions and traps mortal souls.";
			d.stats = { 3.0f, 3.0f, 9.0f, 3.0f, 2.0f };
			d.startingBonuses = { "Reanimated Minion Duration +50%", "Soul Trap Success +50%", "Undead Resistance +15%" };
			d.xpSources = {
				{ "Undead Raised",          20  },
				{ "Soul Trapped",           15  },
				{ "Conjuration Kill",       25  },
				{ "Necromancy Boss Defeat", 100 },
			};
			d.avBonuses = {
				{ 25, 18.0f,  5.0f, "Magicka"     },
				{ 19,  6.0f,  1.0f, "Conjuration" },
				{ 20,  5.0f,  1.0f, "Destruction" },
				{ 24, 10.0f,  2.0f, "Health"      },
				{ 23,  4.0f,  1.0f, "Enchanting"  },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Battlemage; d.category = MainCategory::Magic;
			d.name = "Battlemage"; d.playstyle = "Armored Spellsword";
			d.icon = "[BTM]";
			d.description = "Clad in heavy armor, wielding a blade in one hand and devastating elemental fire in the other.";
			d.stats = { 7.0f, 7.0f, 7.0f, 1.0f, 2.0f };
			d.startingBonuses = { "Normal Spell Cost in Heavy Armor", "Sword & Spell Damage +10%", "Combat Magicka Regen +10%" };
			d.xpSources = {
				{ "Spell & Sword Kill",     25  },
				{ "Destruction Kill",       15  },
				{ "Heavy Armor Hit Taken",   5  },
				{ "Boss Defeated",         100  },
			};
			d.avBonuses = {
				{ 24, 12.0f,  3.0f, "Health"      },
				{ 25, 12.0f,  3.0f, "Magicka"     },
				{  6,  5.0f,  1.0f, "One-Handed"  },
				{ 20,  5.0f,  1.0f, "Destruction" },
				{ 11,  5.0f,  1.0f, "Heavy Armor" },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Druid; d.category = MainCategory::Magic;
			d.name = "Druid"; d.playstyle = "Nature Shaman";
			d.icon = "[DRU]";
			d.description = "Guardian of nature. Summons wildlife, harvests healing flora, and communes with earth spirits.";
			d.stats = { 4.0f, 5.0f, 8.0f, 4.0f, 3.0f };
			d.startingBonuses = { "Poison & Disease Immunity", "Alchemy Potency +15%", "Nature Spell Cost -10%" };
			d.xpSources = {
				{ "Familiar / Beast Kill",  20  },
				{ "Healing Spell Cast",     10  },
				{ "Ingredient Harvested",    5  },
				{ "Spriggan Defeated",      30  },
			};
			d.avBonuses = {
				{ 24, 12.0f,  3.0f, "Health"      },
				{ 25, 15.0f,  4.0f, "Magicka"     },
				{ 22,  5.0f,  1.0f, "Restoration" },
				{ 18,  5.0f,  1.0f, "Alteration"  },
				{ 16,  5.0f,  1.0f, "Alchemy"     },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Elementalist; d.category = MainCategory::Magic;
			d.name = "Elementalist"; d.playstyle = "Elemental Destruction";
			d.icon = "[ELE]";
			d.description = "A master of raw primordial forces - unleashing devastating storms of Fire, Frost, and Lightning.";
			d.stats = { 2.0f, 2.0f, 10.0f, 2.0f, 3.0f };
			d.startingBonuses = { "Destruction Damage +15%", "Elemental Resistances +10%", "Magicka Pool +25" };
			d.xpSources = {
				{ "Fire Kill",              20  },
				{ "Frost Kill",             20  },
				{ "Shock Kill",             20  },
				{ "Master Destruction Kill",50  },
				{ "Dragon Defeated",       150  },
			};
			d.avBonuses = {
				{ 25, 20.0f,  6.0f, "Magicka"     },
				{ 20,  8.0f,  1.0f, "Destruction" },
				{ 18,  5.0f,  1.0f, "Alteration"  },
				{ 23,  4.0f,  1.0f, "Enchanting"  },
				{ 24, 10.0f,  2.0f, "Health"      },
			};
			RegisterClass(d);
		}

		// ── SOCIAL & LIFESTYLE CLASSES ──────────────────────────────────────

		{
			ClassDefinition d;
			d.type = ClassType::Merchant; d.category = MainCategory::Social;
			d.name = "Merchant"; d.playstyle = "Economy & Trade";
			d.icon = "[MRC]";
			d.description = "A master of Skyrim's trade routes. Commands markets through persuasion, bartering, and wealth.";
			d.stats = { 1.0f, 3.0f, 1.0f, 4.0f, 10.0f };
			d.startingBonuses = { "Barter Prices +10%", "Carry Weight +50", "Extra Gold Finding +15%" };
			d.xpSources = {
				{ "Item Sold (100+ gold)",  15  },
				{ "Persuasion Successful",  25  },
				{ "Gold Milestone 1000g",   50  },
				{ "Property Purchased",    100  },
			};
			d.avBonuses = {
				{ 17, 10.0f,  2.0f, "Speech"      },
				{ 26, 15.0f,  4.0f, "Stamina"     },
				{ 14,  5.0f,  1.0f, "Lockpicking" },
				{ 16,  5.0f,  1.0f, "Alchemy"     },
				{ 24, 12.0f,  3.0f, "Health"      },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Bard; d.category = MainCategory::Social;
			d.name = "Bard"; d.playstyle = "Support & Morale";
			d.icon = "[BRD]";
			d.description = "Inspires allies with legendary songs and confuses enemies with captivating Illusion melodies.";
			d.stats = { 3.0f, 3.0f, 6.0f, 5.0f, 8.0f };
			d.startingBonuses = { "Illusion Spell Cost -10%", "Companion Damage +10%", "Free Tavern Stays" };
			d.xpSources = {
				{ "Song Performed",         20  },
				{ "Calm Spell Cast",        15  },
				{ "Tavern Performance",     25  },
				{ "Persuasion Successful",  20  },
			};
			d.avBonuses = {
				{ 17,  8.0f,  2.0f, "Speech"      },
				{ 21,  8.0f,  2.0f, "Illusion"    },
				{ 25, 12.0f,  3.0f, "Magicka"     },
				{ 26, 12.0f,  3.0f, "Stamina"     },
				{  6,  4.0f,  1.0f, "One-Handed"  },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Alchemist; d.category = MainCategory::Social;
			d.name = "Alchemist"; d.playstyle = "Potion & Poison Crafter";
			d.icon = "[ALC]";
			d.description = "A scientist distilling nature's essences into miraculous elixirs and deadly poisons.";
			d.stats = { 2.0f, 3.0f, 6.0f, 6.0f, 7.0f };
			d.startingBonuses = { "Crafted Potion Potency +15%", "Reveal 4 Effects from Ingredients", "Potion Sale Price +15%" };
			d.xpSources = {
				{ "Potion Crafted",         15  },
				{ "Ingredient Discovered",   5  },
				{ "Poison Applied Hit",     10  },
				{ "Master Potion Created",  50  },
			};
			d.avBonuses = {
				{ 16, 10.0f,  2.0f, "Alchemy"     },
				{ 24, 12.0f,  3.0f, "Health"      },
				{ 25, 12.0f,  3.0f, "Magicka"     },
				{ 17,  5.0f,  1.0f, "Speech"      },
				{ 23,  4.0f,  1.0f, "Enchanting"  },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Blacksmith; d.category = MainCategory::Social;
			d.name = "Blacksmith"; d.playstyle = "Armorsmith & Weaponsmith";
			d.icon = "[BSM]";
			d.description = "Master of the forge. Forges the finest tempered steel, runic armaments, and reinforced armor.";
			d.stats = { 7.0f, 8.0f, 2.0f, 1.0f, 6.0f };
			d.startingBonuses = { "Improved Weapon/Armor Rating +15%", "Double Ingot Smelting Chance", "Heavy Armor Weight -25%" };
			d.xpSources = {
				{ "Weapon/Armor Crafted",   20  },
				{ "Item Tempered/Improved", 15  },
				{ "Ore Mined",               5  },
				{ "Legendary Item Forged",  75  },
			};
			d.avBonuses = {
				{ 10, 10.0f,  2.0f, "Smithing"    },
				{ 11,  8.0f,  2.0f, "Heavy Armor" },
				{ 26, 15.0f,  4.0f, "Stamina"     },
				{ 24, 12.0f,  3.0f, "Health"      },
				{  7,  4.0f,  1.0f, "Two-Handed"  },
			};
			RegisterClass(d);
		}

		{
			ClassDefinition d;
			d.type = ClassType::Explorer; d.category = MainCategory::Social;
			d.name = "Explorer"; d.playstyle = "Dungeon Delver & Scavenger";
			d.icon = "[EXP]";
			d.description = "A bold adventurer who unearths Skyrim's deepest ruins, lost tombs, and ancient relics.";
			d.stats = { 5.0f, 5.0f, 3.0f, 6.0f, 5.0f };
			d.startingBonuses = { "New Discovery XP +50%", "Lockpicks Do Not Break (%15)", "Trap Damage -25%" };
			d.xpSources = {
				{ "New Location Discovered", 30  },
				{ "Dungeon Cleared",         50  },
				{ "Word Wall Learned",       75  },
				{ "Treasure Chest Found",    20  },
			};
			d.avBonuses = {
				{ 26, 20.0f,  5.0f, "Stamina"     },
				{ 14,  8.0f,  2.0f, "Lockpicking" },
				{ 15,  5.0f,  1.0f, "Sneak"       },
				{ 12,  5.0f,  1.0f, "Light Armor" },
				{ 24, 12.0f,  3.0f, "Health"      },
			};
			RegisterClass(d);
		}

		SKSE::log::info("ClassRegistry: 20 classes successfully registered in English.");
	}

	void ClassRegistry::RegisterClass(const ClassDefinition& def)
	{
		_classes[def.type] = def;
	}

	std::optional<ClassDefinition> ClassRegistry::GetClass(ClassType type) const
	{
		auto it = _classes.find(type);
		if (it != _classes.end()) return it->second;
		return std::nullopt;
	}

	const std::unordered_map<ClassType, ClassDefinition>& ClassRegistry::GetAllClasses() const
	{
		return _classes;
	}
}
