# ClassBound

ClassBound is an SKSE plugin that adds class selection, shared progression, a universal skill tree, and companion class management to Skyrim Special Edition / Anniversary Edition.

## Current features

- **20 playable classes:** Warrior, Knight, Paladin, Thief, Assassin, Archer, Hunter, Marksman, Gunslinger, Crossbowman, Mage, Necromancer, Battlemage, Druid, Elementalist, Merchant, Bard, Alchemist, Blacksmith, and Explorer.
- **Class progression:** Shared XP advances a ClassBound level from 1 to 100. XP required per level increases with level. The setting to limit ClassBound level to Skyrim character level is enabled by default in settings.
- **Universal Skill Tree:** Earn 3 skill points per ClassBound level and spend them across 16 shared attributes. Skill ranks are capped at 30. The tree includes Health, Magicka, Stamina, regeneration, physical and spell power, critical chance and damage, defense, resistances, agility, luck, XP gain, and Carry Capacity. Carry Capacity adds 10 carry weight per rank.
- **Companions:** Assign classes to active humanoid followers, view their progression, and manage each companion's universal skill points. Follower cards show a portrait selected from the companion's race and sex. Active party members share XP with the player. Class changes for an already-classed character require trainer access and gold.
- **Class Codex interface:** PrismaUI pages include Class Codex, Change Class, Character, Skill Tree, Companion Skills, Followers, Bonuses, Statistics, Settings, and Cheat Menu. The legacy SKSE Menu Framework interface remains available.
- **Level-up feedback:** Level-ups grant the configured skill points, show an in-game notification and character-centered overlay, play the supplied `level-up.mp3`, and apply a visual effect. If custom audio cannot load, Skyrim's level-up sound is used as a fallback.
- **Progression test controls:** The Cheat Menu can grant XP and skill points to the selected player or active companion, grant levels directly to the player, and has a dedicated Follower Level Test selector with +1 and +5 level actions. Follower tests use the normal companion level-up notification, sound, and visual effect. These actions modify the current save.
- **Progression settings:** Adjust the shared XP multiplier and toggle automatic XP, level notifications, XP notifications, and the character-level limit.
- **Save persistence:** Class progression and skill data are stored with the save through SKSE serialization.

## XP rewards currently implemented

- **NPC kills:** A small base reward is granted only when the player or an active companion recently damaged the NPC. Repeated kill events are guarded against duplicate rewards.
- **Completed quests:** A larger reward is granted when a quest completes.
- **New locations:** Discovering a location grants a small reward.
- **Harvesting, mining, and fishing:** Valid resource collection grants small rewards.
- **Crafting and improvement:** Potion crafting and gear crafting or improvement grant XP based on the produced item count.
- **Successful lockpicking:** A small reward is granted after the lock changes to the unlocked state; merely opening the lockpicking interface does not award XP.
- **Spell use:** Selected spell actions can grant small rewards with cooldowns to limit repeated gains.

XP is shared between the player and currently active followers in equal party shares (for example, player plus three active followers means roughly one quarter each). Any integer remainder goes to the player. Each character's Experience Gain skill rank modifies that character's share. The global XP multiplier applies to the reward before the party split.

## Opening the interface

- Open **SKSE Menu Framework → ClassBound → Class Codex (Prisma UI)**, or press **Left Shift + V**.
- Close the Codex with **Esc**.
- The Shift+V shortcut and PrismaUI Codex require PrismaUI and SKSE Menu Framework to be installed and loaded. Without PrismaUI, the legacy SKSE Menu Framework settings interface remains available.
- The Cheat Menu is a page in the PrismaUI Codex. Treat its XP, skill point, and level buttons as save-changing debug tools.

## Requirements

- Skyrim Special Edition / Anniversary Edition with a runtime supported by the installed plugin build.
- SKSE64 matching the game's runtime.
- Address Library for SKSE Plugins.
- SKSE Menu Framework.
- PrismaUI for the PrismaUI Codex and Shift+V hotkey.

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

## Known scope

- Companion management is based on followers recognized as active by Skyrim/SKSE; nearby NPCs who are not active followers are not eligible party members.
- Universal skill effects use Skyrim actor values where available. A few concepts without a direct general-purpose Skyrim actor value use the compatible fallback described in the skill tree.
- XP from optional crafting and harvesting events depends on the game exposing the corresponding inventory or quest event. Some mod-added activities may not report these events in the same way as vanilla Skyrim.
- The old class-specific specialization unlock tree and old skill-point system are removed. Class progression uses the Universal Skill Tree.

## Development

This repository contains the C++ SKSE plugin source and PrismaUI assets. Build configuration is in `CMakeLists.txt`; it expects a local CommonLibSSE-NG checkout (`COMMONLIBSSE_NG_DIR`) and King Core headers (`KINGCORE_INCLUDE_DIR`).

© 2026 ClassBound.

