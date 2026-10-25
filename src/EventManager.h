#pragma once
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace Classbound
{
	class EventManager : 
		public RE::BSTEventSink<RE::TESHitEvent>,
		public RE::BSTEventSink<RE::TESSpellCastEvent>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<RE::TESLockChangedEvent>,
		public RE::BSTEventSink<RE::TESActivateEvent>,
		public RE::BSTEventSink<RE::TESQuestStageEvent>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::LocationDiscovery::Event>,
		public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		static EventManager* GetSingleton()
		{
			static EventManager singleton;
			return &singleton;
		}

		void Register();

		// Data-driven action dispatcher: awards shared activity XP
		void HandleAction(const char* actionKey, float customMultiplier = 1.0f, bool isPracticeDummy = false);
		void UpdateCombatAndTrainingBatch();

		// Event handlers
		RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESLockChangedEvent* a_event, RE::BSTEventSource<RE::TESLockChangedEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::LocationDiscovery::Event* a_event, RE::BSTEventSource<RE::LocationDiscovery::Event>* a_eventSource) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>* a_eventSource) override;

	private:
		EventManager() = default;
		~EventManager() = default;

		static bool IsPracticeDummy(RE::TESObjectREFR* target);

		struct DamageRecord {
			uint64_t timestamp{ 0 };
			bool wasPowerAttack{ false };
			bool wasBash{ false };
			bool wasSneak{ false };
			bool wasTwoHanded{ false };
			bool wasOneHanded{ false };
			bool wasBow{ false };
			bool wasStaff{ false };
			bool wasSpell{ false };
			uint32_t spellSchool{ 0 };
		};

		// Track which actors were recently damaged by player and details of the attack
		std::unordered_map<RE::FormID, DamageRecord> _partyDamagedActors;

		// Druid follower healing cooldown (ms timestamp of last heal tick)
		uint64_t _combatTacticsCooldownMs = 0;


		// Action cooldowns to prevent repeat spam on rapid triggers
		std::unordered_map<std::string, uint64_t> _actionCooldowns;
		std::unordered_map<RE::FormID, uint64_t> _activityCooldowns;
		std::unordered_map<RE::FormID, uint64_t> _deathRewards;
		std::unordered_set<RE::FormID> _completedQuests;
		std::chrono::steady_clock::time_point _lastMaintenance{};
		std::chrono::steady_clock::time_point _lastActivityPrune{};

		// Combat batching state
		bool _inCombatState{ false };
		uint32_t _batchedCombatXP{ 0 };
		uint32_t _combatHitsCount{ 0 };

		// Practice dummy batching state
		uint32_t _batchedDummyXP{ 0 };
		uint64_t _lastDummyHitTimeMs{ 0 };

		// Periodic batch update timer
		uint64_t _lastBatchCheckMs{ 0 };

		// Track player gold when entering barter menu
		int32_t _goldBeforeBarter{ 0 };
		bool _craftingMenuOpen{ false };
		uint32_t _batchedFoodCount{ 0 };
		uint32_t _batchedFoodXP{ 0 };
		uint32_t _batchedPotionsCount{ 0 };
		uint32_t _batchedPotionsXP{ 0 };
		uint32_t _batchedGearCount{ 0 };
		uint32_t _batchedGearXP{ 0 };
		RE::FormID _lockpickingTarget{ 0 };
		uint64_t _lockpickingAttemptUntilMs{ 0 };
	};
}
