# ClassBound

ClassBound is an SKSE plugin that adds class selection, shared progression, a universal skill tree, and companion class management to Skyrim Special Edition / Anniversary Edition.

## Current features

- **20 playable classes:** Warrior, Knight, Paladin, Thief, Assassin, Archer, Hunter, Marksman, Gunslinger, Crossbowman, Mage, Necromancer, Battlemage, Druid, Elementalist, Merchant, Bard, Alchemist, Blacksmith, and Explorer.
- **Class progression:** Shared XP advances a ClassBound level from 1 to 100. XP required per level increases with level. The setting to limit ClassBound level to Skyrim character level is enabled by default in settings.
- **Universal Skill Tree:** Earn 3 skill points per ClassBound level and spend them across 16 shared attributes. Skill ranks are capped at 30. A class must be chosen (for player) or assigned (for companions) before investing skill points in the tree. The tree includes Health, Magicka, Stamina, regeneration, physical and spell power, critical chance and damage, defense, resistances, agility, luck, XP gain, and Carry Capacity. Carry Capacity adds 10 carry weight per rank.
- **Companions:** Assign classes to active humanoid followers, view their progression, and manage each companion's universal skill points. Companions cannot spend skill points until a class has been chosen. Follower cards show a portrait selected from the companion's race and sex. Active party members share XP with the player. Class changes for an already-classed character require trainer access and gold. Followers use Skyrim's normal combat and formation behavior; ClassBound does not grant them custom combat abilities or formation commands. Target priority can still be selected in the Followers page.
- **Class Codex interface:** PrismaUI pages include Class Codex, Change Class, Character, Skill Tree, Companion Skills, Followers, Bonuses, Statistics, Settings, and Cheat Menu. The legacy SKSE Menu Framework interface remains available.
- **Level-up feedback:** Level-ups grant the configured skill points, show an in-game notification and character-centered overlay, play the supplied `level-up.mp3`, and apply a visual effect. If custom audio cannot load, Skyrim's level-up sound is used as a fallback.
- **Progression test controls:** The Cheat Menu can grant XP and skill points to the selected player or active companion, grant levels directly to the player, and has a dedicated Follower Level Test selector with +1 and +5 level actions. Follower tests use the normal companion level-up notification, sound, and visual effect. These actions modify the current save.
- **Progression & Notification settings:** Adjust the shared XP multiplier and toggle automatic XP, level notifications, XP notifications, the character-level limit, and a master switch to toggle all notifications on or off to protect performance during rapid skill allocation.
- **Save persistence:** Class progression and skill data are stored with the save through SKSE serialization.

- **NPC & Training Dummy hits:** Combat XP is granted only upon striking an actual target (enemy actor or training/practice dummy/target). Swinging weapons or shooting into thin air does not grant XP. One-Handed, Two-Handed, Archery, Staff strikes, and Blocking grant their respective XP.
- **Spell use:** Destructive and hostile spells award XP only during actual combat or upon hitting an enemy/target dummy. Out-of-combat air casting of offensive spells is ignored. Utility and buff spells (Healing, Conjuration, Alteration) reward XP when cast intentionally, governed by an internal cooldown.
- **NPC kills:** A base reward is granted only when the player or an active companion recently damaged the NPC. Repeated kill events are guarded against duplicate rewards.
- **Completed quests:** A larger reward is granted when a quest completes.
- **New locations:** Discovering a location grants a small reward.
- **Harvesting, mining, and fishing:** Valid resource collection grants small rewards.
- **Crafting and improvement:** Potion crafting and gear crafting or improvement grant XP based on the produced item count.
- **Successful lockpicking:** A small reward is granted after the lock changes to the unlocked state; merely opening the lockpicking interface does not award XP.

XP is shared between the player and currently active followers in equal party shares (for example, player plus three active followers means roughly one quarter each). Any integer remainder goes to the player. Each character's Experience Gain skill rank modifies that character's share. The global XP multiplier applies to the reward before the party split.

## Opening the interface

- Open **SKSE Menu Framework → ClassBound → Class Codex (Prisma UI)**, or press **Left Shift + V**.
- Close the Codex with **Esc**.
- The Shift+V shortcut and PrismaUI Codex require PrismaUI and SKSE Menu Framework to be installed and loaded. Without PrismaUI, the legacy SKSE Menu Framework settings interface remains available.
- The Cheat Menu is a page in the PrismaUI Codex. Treat its XP, skill point, and level buttons as save-changing debug tools.

## Required mods

Install these for the in-game ClassBound menus and SKSE plugin:

- **Skyrim Special Edition / Anniversary Edition** with a runtime supported by the installed ClassBound DLL.
- **[SKSE64](https://skse.silverlock.org/)** matching the exact game runtime. Start Skyrim through the SKSE loader.
- **[Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)** with files matching the game runtime.
- **[SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)** to register and open the ClassBound menus. Current framework releases (3.15 and later) also require **[ImGui Icons](https://www.nexusmods.com/skyrimspecialedition/mods/114790)**.

### Optional interface framework

- **[Prisma UI](https://www.nexusmods.com/skyrimspecialedition/mods/148718)** enables the full Class Codex, the Shift+V shortcut, and PrismaUI overlays. Without it, the legacy ImGui settings menu remains available through SKSE Menu Framework.
- Prisma UI requires **[Media Keys Fix SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/92948)** (or [Keyboard Shortcuts Fix](https://www.nexusmods.com/skyrimspecialedition/mods/3620)) and the **[Microsoft Visual C++ 2022 Redistributable x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)**.
- King Core is optional; it is only needed for integrations that use ClassBound's King Core API.

### Optional weapon mods for class starter gear

Install these optional mods to receive the matching non-vanilla starter weapons. ClassBound detects each plugin by its ESP filename; if it is missing, the affected class receives the listed vanilla fallback:

- **Knight and Assassin — [Animated Armoury - DAR Version - New Weapons with animations](https://www.nexusmods.com/skyrimspecialedition/mods/35978)** (`NewArmoury.esp`): Knight receives an Iron Pike and Assassin receives Iron Claws. Without this plugin, they receive an Iron Sword and Iron Daggers respectively.
- **Gunslinger — [Lore Friendly Guns of Skyrim (Formerly Rifles)](https://www.nexusmods.com/skyrimspecialedition/mods/60183)** (`HandgunsOfSkyrim.esp`): Gunslinger receives an Iron Handgun and Common Lead Shot. Without this plugin, Gunslinger receives an Iron Sword.

## Mod Organizer 2 installation

Install ClassBound as a mod in MO2 and ensure its required SKSE plugins/frameworks are enabled. Launch the game through the SKSE loader configured for that profile. The plugin and interface assets should be present in the virtual `Data` tree at:

- `Data/SKSE/Plugins/Classbound.dll`
- `Data/SKSE/Plugins/ClassBound/Audio/level-up.mp3`
- `Data/PrismaUI/views/ClassBound/`

The custom level-up audio file is decoded at plugin startup. If replacing it, keep the filename and path unchanged.

## Class list

| Category | Classes |
|---|---|
| Melee / stealth | Warrior, Knight, Paladin, Thief, Assassin |
| Ranged | Archer, Hunter, Marksman, Gunslinger, Crossbowman |
| Magic | Mage, Necromancer, Battlemage, Druid, Elementalist |
| Social / utility | Merchant, Bard, Alchemist, Blacksmith, Explorer |

## Starting equipment by class

The following items are granted when a class is first selected or changed through a class trainer. Optional weapon mods affect only the items noted below. Knight and Merchant also receive a horse at the player's current location.

### Melee and stealth

- **Warrior:** Iron Greatsword; Iron Armor, Iron Boots, Iron Gauntlets, Iron Helmet; 3 Minor Healing Potions.
- **Knight:** Iron Pike (requires `NewArmoury.esp`; otherwise this item falls back to an Iron Sword), an additional Iron Sword, Iron Shield, Iron Armor, Iron Boots, Iron Gauntlets, Iron Helmet; 3 Minor Healing Potions; one horse.
- **Paladin:** Iron War Axe, Iron Shield, Iron Armor, Iron Boots, Iron Gauntlets, Iron Helmet; 5 Minor Healing Potions, 3 Minor Magicka Potions; knows Healing and Lesser Ward; receives Firebolt and Lesser Ward spell tomes.
- **Thief:** Iron Dagger; Leather Armor, Leather Boots, Leather Bracers, Leather Helmet; 30 Lockpicks, 5 Minor Stamina Potions, 3 Sneak Potions.
- **Assassin:** Iron Claws (requires `NewArmoury.esp`; each claw falls back to an Iron Dagger); Hide Armor, Hide Boots, Hide Bracers, Hide Helmet; 10 Frostbite Venoms, 3 Invisibility Potions.

### Ranged

- **Archer:** Long Bow, 75 Steel Arrows; Hide Armor, Hide Boots, Hide Bracers, Hide Helmet; 3 Minor Stamina Potions.
- **Hunter:** Hunting Bow, 60 Steel Arrows; Hide Armor, Hide Boots, Hide Bracers, Hide Helmet; 3 Torches, 3 Minor Healing Potions.
- **Marksman:** Hunting Bow, 100 Steel Arrows; Leather Armor, Leather Boots, Leather Bracers, Leather Helmet; 5 Minor Stamina Potions.
- **Gunslinger:** Iron Handgun and 24 Common Lead Shot (requires `HandgunsOfSkyrim.esp`; the handgun falls back to an Iron Sword if the plugin is absent); Hide Armor, Hide Boots, Hide Bracers, Hide Helmet; 3 Minor Healing Potions, 3 Minor Stamina Potions.
- **Crossbowman:** Crossbow, 60 Steel Bolts; Hide Armor, Hide Boots, Hide Bracers, Hide Helmet; 4 Minor Stamina Potions. The Crossbow and Steel Bolts require `Dawnguard.esm`.

### Magic

- **Mage:** Staff of Sparks; Novice Robes, Boots, Gloves, Mage Hood; 5 Minor Magicka Potions, 3 Minor Healing Potions; knows Flames, Frostbite, and Sparks; receives Firebolt and Oakflesh spell tomes.
- **Necromancer:** Iron Dagger; Necromancer Robes, Boots, Gloves, Mage Hood; 3 Petty Soul Gems, 5 Minor Magicka Potions; knows Raise Zombie and Conjure Familiar; receives Raise Zombie and Oakflesh spell tomes.
- **Battlemage:** Iron Sword; Iron Armor, Iron Boots, Iron Gauntlets, Iron Helmet; 4 Minor Magicka Potions, 3 Minor Healing Potions; receives Bound Sword and Oakflesh spell tomes.
- **Druid:** Iron War Axe; Novice Robes, Boots, Gloves, Mage Hood; 4 Minor Magicka Potions, 4 Minor Healing Potions; knows Healing, Healing Hands, and Conjure Familiar; receives Conjure Familiar and Healing Hands spell tomes.
- **Elementalist:** Novice Robes, Boots, Gloves, Mage Hood; 8 Minor Magicka Potions; knows Flames, Frostbite, and Sparks; receives Firebolt and Oakflesh spell tomes.

### Social and utility

- **Merchant:** Iron Dagger; Fine Clothes, Fine Boots, Gloves, Fine Hat; 1,000 Gold, 2 Garnets, 2 Amethysts, 1 Ruby, 1 Sapphire; one horse.
- **Bard:** Iron Dagger; Tavern Clothes, Boots, Gloves, Hat; 1 Flute, 1 Lute, 100 Gold; knows Courage and Clairvoyance.
- **Alchemist:** Iron Dagger; Fine Clothes, Fine Boots, Gloves, Mage Hood; 4 each of Minor Healing, Minor Magicka, and Minor Stamina Potions; 3 Frostbite Venoms; 1 each of Poison Recipe Note 1 and 2.
- **Blacksmith:** Iron Mace; Blacksmith's Apron, Boots, Gloves, Leather Helmet; 10 Iron Ingots, 5 Steel Ingots, 5 Leather Strips, 5 Leather, 6 Firewood.
- **Explorer:** Iron Sword; Fur Armor, Fur Boots, Fur Gauntlets, Fur Helmet; 5 Torches, 10 Lockpicks, 3 Minor Healing Potions; Adventurer Backpack with Bedroll if `ccfsvsse001-backpacks.esl` is installed.

## Known scope

- Companion management is based on followers recognized as active by Skyrim/SKSE; nearby NPCs who are not active followers are not eligible party members.
- Universal skill effects use Skyrim actor values where available. A few concepts without a direct general-purpose Skyrim actor value use the compatible fallback described in the skill tree.
- XP from optional crafting and harvesting events depends on the game exposing the corresponding inventory or quest event. Some mod-added activities may not report these events in the same way as vanilla Skyrim.
- The old class-specific specialization unlock tree and old skill-point system are removed. Class progression uses the Universal Skill Tree.

## Development

This repository contains the C++ SKSE plugin source and PrismaUI assets. Build configuration is in `CMakeLists.txt`; it expects a local CommonLibSSE-NG checkout (`COMMONLIBSSE_NG_DIR`) and King Core headers (`KINGCORE_INCLUDE_DIR`).

© 2026 ClassBound.

