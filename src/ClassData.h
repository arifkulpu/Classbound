#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace Classbound
{
	enum class MainCategory : uint32_t
	{
		None   = 0,
		Melee  = 1,
		Ranged = 2,
		Magic  = 3,
		Social = 4
	};

	enum class TacticalFocus : uint32_t
	{
		Free = 0,             // Free choice / default AI
		ProtectPlayer = 1,    // Focus enemies attacking player
		TargetPlayerFocus = 2,// Focus what player is attacking
		TargetCasters = 3,    // Prioritize enemy mages & archers
		TargetWeakest = 4     // Finish low HP enemies
	};

	enum class ClassType : uint32_t
	{
		None = 0,

		// Melee
		Warrior = 1,
		Knight,
		Paladin,
		Thief,
		Assassin,

		// Ranged
		Archer = 10,
		Hunter,
		Marksman,
		Gunslinger,
		Crossbowman,

		// Magic
		Mage = 20,
		Necromancer,
		Battlemage,
		Druid,
		Elementalist,

		// Social & Lifestyle
		Merchant = 30,
		Bard,
		Alchemist,
		Blacksmith,
		Explorer
	};

	struct ClassStatistics
	{
		float combat{ 0.0f };
		float defense{ 0.0f };
		float magic{ 0.0f };
		float stealth{ 0.0f };
		float trading{ 0.0f };
	};

	// Represents a single in-game action that grants Class XP
	struct XpSource
	{
		const char* action;
		int         xp;
	};

	// ─── ActorValue bonus ────────────────────────────────────────────────────
	// avIndex maps to RE::ActorValue enum values (uint32_t to avoid RE header dep).
	// kHealth=24, kMagicka=25, kStamina=26
	// kOneHanded=6, kTwoHanded=7, kArchery=8, kBlock=9, kSmithing=10
	// kHeavyArmor=11, kLightArmor=12, kPickpocket=13, kLockpicking=14
	// kSneak=15, kAlchemy=16, kSpeech=17, kAlteration=18, kConjuration=19
	// kDestruction=20, kIllusion=21, kRestoration=22, kEnchanting=23
	struct AVBonus
	{
		uint32_t avIndex;    // RE::ActorValue as uint32_t
		float    base;       // Bonus at class level 1
		float    perLevel;   // Extra per 5 levels
		const char* label;   // Display name (ASCII only)
	};

	struct ClassDefinition
	{
		ClassType    type{ ClassType::None };
		MainCategory category{ MainCategory::None };
		const char*  name{ "" };
		const char*  description{ "" };
		const char*  playstyle{ "" };
		const char*  icon{ "" };
		ClassStatistics stats;
		std::vector<std::string> startingBonuses;
		std::vector<XpSource>   xpSources;
		std::vector<AVBonus>    avBonuses;
	};

	struct PlayerStatistics
	{
		uint32_t enemiesKilled{ 0 };
		uint32_t meleeKills{ 0 };
		uint32_t archeryKills{ 0 };
		uint32_t magicKills{ 0 };
		uint32_t sneakKills{ 0 };
		uint32_t bossesDefeated{ 0 };
		uint32_t spellsCast{ 0 };
		uint32_t itemsSold{ 0 };
		uint32_t itemsCrafted{ 0 };
		uint32_t locksPicked{ 0 };
		uint32_t dungeonsCleared{ 0 };
	};

	static constexpr size_t kUniversalAttributeCount = 17;
	static constexpr uint32_t kMaxUniversalRank = 30;
	static constexpr uint32_t kMaxUniversalSkillPoints = 300;

	struct UniversalSkillProgress
	{
		uint32_t earnedPoints{ 3 };
		uint32_t bonusPoints{ 0 }; // Test/admin-granted points, independent of level-earned points.
		uint32_t lastAwardedLevel{ 1 };
		std::array<uint32_t, kUniversalAttributeCount> ranks{};
		std::vector<std::pair<uint32_t, float>> appliedBonuses;
		bool bonusesApplied{ false };
		bool pendingRebuild{ false };
	};

	struct PlayerClassData
	{
		ClassType   currentClass{ ClassType::None };
		uint32_t    classLevel{ 1 };
		uint32_t    classXP{ 0 };
		PlayerStatistics stats;
		UniversalSkillProgress universalSkills;

		// Track exactly how much of each AV we have applied (base class bonuses)
		std::vector<std::pair<uint32_t, float>> appliedAVBonuses;

	};

	struct FollowerClassData
	{
		uint32_t    formID{ 0 };             // Actor Base FormID
		std::string name{ "" };              // Actor Name (e.g. Lydia, Serana)
		ClassType   currentClass{ ClassType::None };
		uint32_t    classLevel{ 1 };
		uint32_t    classXP{ 0 };
		bool        isActiveFollower{ true };
		UniversalSkillProgress universalSkills;

		// Track applied AV bonuses to this follower
		std::vector<std::pair<uint32_t, float>> appliedAVBonuses;

		// Optional target priority for companion AI
		TacticalFocus    tacticalFocus{ TacticalFocus::Free };
	};
}
