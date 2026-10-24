#include "UIManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include "ClassChangeManager.h"
#include "SettingsManager.h"
#include "SkillManager.h"
#include "GlobalCharacterDB.h"
#include "XPManager.h"
#include "EffectManager.h"
#include "FollowerManager.h"
#include "AffinityManager.h"
#include <vector>
#include <chrono>

namespace RE { class InputEvent; }
#include "SKSEMenuFramework.h"

namespace Classbound
{
	// ─── colour helpers ──────────────────────────────────────────────────────
	static ImGuiMCP::ImVec4 cGold()   { return { 0.82f, 0.66f, 0.24f, 1.0f }; }
	static ImGuiMCP::ImVec4 cLGold()  { return { 0.95f, 0.85f, 0.55f, 1.0f }; }
	static ImGuiMCP::ImVec4 cGreen()  { return { 0.30f, 0.90f, 0.40f, 1.0f }; }
	static ImGuiMCP::ImVec4 cRed()    { return { 0.90f, 0.20f, 0.20f, 1.0f }; }
	static ImGuiMCP::ImVec4 cGray()   { return { 0.55f, 0.55f, 0.55f, 1.0f }; }
	static ImGuiMCP::ImVec4 cMelee()  { return { 0.90f, 0.35f, 0.20f, 1.0f }; }
	static ImGuiMCP::ImVec4 cRanged() { return { 0.25f, 0.80f, 0.35f, 1.0f }; }
	static ImGuiMCP::ImVec4 cMagic()  { return { 0.40f, 0.60f, 1.00f, 1.0f }; }
	static ImGuiMCP::ImVec4 cSocial() { return { 0.95f, 0.78f, 0.20f, 1.0f }; }
	static ImGuiMCP::ImVec4 cXP()     { return { 0.20f, 0.85f, 0.80f, 1.0f }; }

	static ImGuiMCP::ImVec4 CatColor(MainCategory c)
	{
		switch (c) {
		case MainCategory::Melee:  return cMelee();
		case MainCategory::Ranged: return cRanged();
		case MainCategory::Magic:  return cMagic();
		case MainCategory::Social: return cSocial();
		default: return cGray();
		}
	}

	static const char* CatLabel(MainCategory c)
	{
		switch (c) {
		case MainCategory::Melee:  return "MELEE COMBAT";
		case MainCategory::Ranged: return "RANGED & MARKSMAN";
		case MainCategory::Magic:  return "MAGIC & ARCANE";
		case MainCategory::Social: return "SOCIAL & LIFESTYLE";
		default: return "?";
		}
	}

	static const char* CatDesc(MainCategory c)
	{
		switch (c) {
		case MainCategory::Melee:  return "Masters of swords, axes, and heavy steel. Frontline combatants.";
		case MainCategory::Ranged: return "Deliver death from afar. Masters of stealth, bow, and precision.";
		case MainCategory::Magic:  return "Wielders of arcane energy, commanding elemental forces and spirits.";
		case MainCategory::Social: return "Masters of economy, forging, alchemy, and world exploration.";
		default: return "";
		}
	}

	// ─── Main Render ──────────────────────────────────────────────────────────
	static int _selectedPreviewClass = 0;
	static bool _showConfirmDialog = false;

	void UIManager::Render()
	{
		// Temporary buff expiry is checked four times per second while this menu renders.
		static auto lastBuffTick = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now - lastBuffTick >= std::chrono::milliseconds(250)) { SkillManager::GetSingleton()->TickTempBuffs(); lastBuffTick = now; }

		// Live-check: If DialogueMenu is currently active or near trainer, detect speaker immediately
		auto checkActor = [](RE::Actor* actor) {
			if (actor) {
				uint32_t refId = actor->GetFormID();
				uint32_t baseId = actor->GetActorBase() ? actor->GetActorBase()->GetFormID() : 0;
				if (ClassChangeManager::IsTrainerNPC(refId) || ClassChangeManager::IsTrainerNPC(baseId)) {
					ClassChangeManager::GetSingleton()->GrantTrainerAccess();
				}
			}
		};

		if (auto topicMgr = RE::MenuTopicManager::GetSingleton()) {
			if (auto speaker = topicMgr->speaker.get()) {
				checkActor(speaker->As<RE::Actor>());
			}
			if (auto lastSpeaker = topicMgr->lastSpeaker.get()) {
				checkActor(lastSpeaker->As<RE::Actor>());
			}
		}

		// Also check Crosshair target if looking at an NPC
		if (auto crosshair = RE::CrosshairPickData::GetSingleton()) {
			if (crosshair->target && crosshair->target.get()->Is(RE::FormType::ActorCharacter)) {
				checkActor(crosshair->target.get()->As<RE::Actor>());
			}
		}

		// Trainer proximity requires a cell-wide scan, so run it once per second.
		static auto lastProximityCheck = std::chrono::steady_clock::time_point{};
		auto proximityNow = std::chrono::steady_clock::now();
		if (proximityNow - lastProximityCheck >= std::chrono::seconds(1)) {
		lastProximityCheck = proximityNow;
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			if (auto* cell = player->GetParentCell()) {
				for (auto& ref : cell->GetRuntimeData().references) {
					if (ref && ref->Is(RE::FormType::ActorCharacter)) {
						auto* actor = ref->As<RE::Actor>();
						if (actor && actor != player && !actor->IsDead()) {
							float dist = player->GetPosition().GetDistance(actor->GetPosition());
							if (dist < 400.0f) {
								checkActor(actor);
							}
						}
					}
				}
			}
		}

		}

		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		const bool hasClass = (pd.currentClass != ClassType::None);
		auto classMgr = ClassChangeManager::GetSingleton();
		const bool hasTrainer = classMgr->HasTrainerAccess();

		// Header
		ImGuiMCP::TextColored(cGold(), "  CLASSBOUND RPG");
		ImGuiMCP::SameLine();
		if (hasClass) {
			auto defOpt = ClassRegistry::GetSingleton()->GetClass(pd.currentClass);
			if (defOpt) {
				ImGuiMCP::TextColored(CatColor(defOpt->category), " | %s", defOpt->name);
				ImGuiMCP::SameLine();
				ImGuiMCP::TextColored(cGreen(), "(Level %u)", pd.classLevel);
			}
		} else {
			ImGuiMCP::TextColored(cRed(), " | [No Class Selected]");
		}
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		if (!hasClass) {
			// If player has NO class, show the Class Selection directly as the primary interactive screen!
			ImGuiMCP::TextColored(cGold(), "  CHOOSE YOUR CHARACTER CLASS");
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cGray(), "  Welcome adventurer! Choose a class from the categories below to start your journey.");
			ImGuiMCP::Separator();
			ImGuiMCP::Spacing();

			RenderChangeClass();
		} else {
			// Player has a class: show organized tabs
			if (ImGuiMCP::BeginTabBar("##CBTabs")) {
				if (ImGuiMCP::BeginTabItem(" Character "))        { ImGuiMCP::Spacing(); RenderCharacterClass(); ImGuiMCP::EndTabItem(); }
								
				if (ImGuiMCP::BeginTabItem(" Followers "))        { ImGuiMCP::Spacing(); RenderFollowers(); ImGuiMCP::EndTabItem(); }
				if (ImGuiMCP::BeginTabItem(" Bonuses "))          { ImGuiMCP::Spacing(); RenderBonusOverview(); ImGuiMCP::EndTabItem(); }
				if (ImGuiMCP::BeginTabItem(" Statistics "))       { ImGuiMCP::Spacing(); RenderClassStatistics(); ImGuiMCP::EndTabItem(); }
				
				// Change class tab
				if (hasTrainer) {
					if (ImGuiMCP::BeginTabItem(" [Trainer] Change Class ")) {
						ImGuiMCP::Spacing();
						RenderChangeClass();
						ImGuiMCP::EndTabItem();
					}
				} else {
					if (ImGuiMCP::BeginTabItem(" Change Class ")) {
						ImGuiMCP::Spacing();
						RenderChangeClass();
						ImGuiMCP::EndTabItem();
					}
				}

				if (ImGuiMCP::BeginTabItem(" Settings "))         { ImGuiMCP::Spacing(); RenderSettings(); ImGuiMCP::EndTabItem(); }
				ImGuiMCP::EndTabBar();
			}
		}

		// Class Change Confirmation Popup Modal
		if (_showConfirmDialog) {
			ImGuiMCP::OpenPopup("Class Change Confirmation");
		}

		ImGuiMCP::SetNextWindowPos(ImGuiMCP::ImVec2(ImGuiMCP::GetIO()->DisplaySize.x * 0.5f, ImGuiMCP::GetIO()->DisplaySize.y * 0.5f), 1 << 2, ImGuiMCP::ImVec2(0.5f,0.5f));
		
		if (ImGuiMCP::BeginPopupModal("Class Change Confirmation", NULL, 1 << 1)) {
			uint32_t cost = ClassChangeManager::GetSingleton()->GetChangeCost(pd.classLevel);

			ImGuiMCP::TextColored(cRed(), "WARNING!");
			ImGuiMCP::Spacing();
			ImGuiMCP::Text("Your shared level and XP are preserved when changing class.");
			ImGuiMCP::Text("Class passives are reset for the new class.");
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cGold(), "Cost: %u Gold", cost);
			ImGuiMCP::Spacing();
			ImGuiMCP::Separator();
			ImGuiMCP::Spacing();

			if (ImGuiMCP::Button("CONFIRM", ImGuiMCP::ImVec2(120, 0))) {
				if (ClassChangeManager::GetSingleton()->ExecuteClassChange(static_cast<ClassType>(_selectedPreviewClass))) {
					_selectedPreviewClass = 0;
					ImGuiMCP::CloseCurrentPopup();
					_showConfirmDialog = false;
				}
			}
			ImGuiMCP::SetItemDefaultFocus();
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("CANCEL", ImGuiMCP::ImVec2(120, 0))) {
				ImGuiMCP::CloseCurrentPopup();
				_showConfirmDialog = false;
			}
			ImGuiMCP::EndPopup();
		}
	}

	// --- TAB 1: Character Class ------------------------------------------------
	void UIManager::RenderCharacterClass()
	{
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		if (pd.currentClass == ClassType::None) return;

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(pd.currentClass);
		if (!defOpt) return;
		const auto& def = defOpt.value();
		auto col = CatColor(def.category);

		// Header Card
		ImGuiMCP::TextColored(col,   "  [%s]", CatLabel(def.category));
		ImGuiMCP::SameLine();
		ImGuiMCP::TextColored(cGold(),"   %s", def.name);
		ImGuiMCP::Spacing();
		ImGuiMCP::TextColored(cLGold(), "  Playstyle: %s", def.playstyle);
		ImGuiMCP::TextWrapped("  %s", def.description);
		ImGuiMCP::Spacing();
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// Level and XP dashboard card
		ImGuiMCP::Columns(2, "##ci", false);
		ImGuiMCP::SetColumnWidth(0, 160.0f);
		ImGuiMCP::SetColumnWidth(1, 240.0f);

		ImGuiMCP::TextColored(cGray(), "Class Level:");
		ImGuiMCP::TextColored(cGold(), "  Level %u", pd.classLevel);
		ImGuiMCP::NextColumn();

		uint32_t reqXP = XPManager::GetSingleton()->GetXPForNextLevel(pd.classLevel);
		ImGuiMCP::TextColored(cGray(), "Experience (XP):");
		if (pd.classLevel >= 100) {
			ImGuiMCP::TextColored(cGreen(), "  %u (MAX LEVEL)", pd.classXP);
		} else {
			ImGuiMCP::TextColored(cXP(), "  %u / %u XP", pd.classXP, reqXP);
		}
		ImGuiMCP::NextColumn();


		ImGuiMCP::Columns(1);
		ImGuiMCP::Spacing();

		// Huge, Clear XP Progress Bar
		float xpPercent = (pd.classLevel >= 100) ? 1.0f : (float)pd.classXP / (float)reqXP;
		char overlay[64];
		if (pd.classLevel >= 100) {
			snprintf(overlay, sizeof(overlay), "MAX LEVEL REACHED (Level 100)");
		} else {
			snprintf(overlay, sizeof(overlay), "Level Progress: %u / %u XP (%.1f%%)", pd.classXP, reqXP, xpPercent * 100.0f);
		}
		ImGuiMCP::ProgressBar(xpPercent, ImGuiMCP::ImVec2(-1.0f, 34.0f), overlay);

		auto* settings = SettingsManager::GetSingleton();
		if (settings->limitClassLevelToPlayerLevel && pd.classLevel < 100) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				if (pd.classLevel >= player->GetLevel()) {
					ImGuiMCP::Spacing();
					ImGuiMCP::TextColored(cGold(), "  * Class Level capped at Character Level (%u). Level up in Skyrim to unlock higher class levels, or disable this in Settings.", player->GetLevel());
				}
			}
		}

		ImGuiMCP::Spacing();
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// Two-column layout for Stats & Bonuses
		ImGuiMCP::Columns(2, "##class_overview_cols", false);
		ImGuiMCP::SetColumnWidth(0, 260.0f);

		// Left Column: Statistics distribution
		ImGuiMCP::TextColored(cGold(), "  Combat Attributes:");
		ImGuiMCP::Spacing();
		struct SB { const char* n; float v; ImGuiMCP::ImVec4 c; };
		SB bars[] = {
			{ "Combat  ", def.stats.combat,  cMelee() },
			{ "Defense ", def.stats.defense, cMagic() },
			{ "Magic   ", def.stats.magic,   cMagic() },
			{ "Stealth ", def.stats.stealth, cGray()  },
			{ "Trading ", def.stats.trading, cSocial()},
		};
		for (auto& b : bars) {
			ImGuiMCP::TextColored(b.c, "  %s", b.n);
			ImGuiMCP::SameLine();
			ImGuiMCP::ImVec2 sz{ 120.0f, 10.0f };
			char ov[8]; snprintf(ov, sizeof(ov), "%.0f", b.v);
			ImGuiMCP::ProgressBar(b.v / 10.0f, sz, ov);
		}

		ImGuiMCP::NextColumn();

		// Right Column: Passive bonuses
		ImGuiMCP::TextColored(cGold(), "  Active Class Bonuses:");
		ImGuiMCP::Spacing();
		for (const auto& b : def.startingBonuses) {
			ImGuiMCP::TextColored(cGreen(), "    + %s", b.c_str());
		}
		ImGuiMCP::Spacing();

		ImGuiMCP::Columns(1);
		ImGuiMCP::Spacing();

		// Equipment Affinity & Set Synergy Card
		auto affMgr = AffinityManager::GetSingleton();
		const auto& profile = affMgr->GetProfileForClass(pd.currentClass);
		bool isAffActive = affMgr->IsAffinityActive();

		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();
		ImGuiMCP::TextColored(cGold(), "  CLASS EQUIPMENT AFFINITY & SET SYNERGY");
		ImGuiMCP::SameLine();
		if (isAffActive) {
			ImGuiMCP::TextColored(cGreen(), "[ACTIVE: %s]", profile.name.c_str());
		} else {
			ImGuiMCP::TextColored(cGray(), "[INACTIVE: %s]", profile.name.c_str());
		}

		ImGuiMCP::Spacing();
		ImGuiMCP::TextColored(cLGold(), "    Required Gear:");
		ImGuiMCP::SameLine();
		ImGuiMCP::TextColored(cXP(), " %s", profile.requirementText.c_str());

		ImGuiMCP::TextColored(cLGold(), "    Synergy Bonus:");
		ImGuiMCP::SameLine();
		if (isAffActive) {
			ImGuiMCP::TextColored(cGreen(), " %s", profile.effectDescription.c_str());
		} else {
			ImGuiMCP::TextColored(cGray(), " %s", profile.effectDescription.c_str());
		}
		ImGuiMCP::Spacing();
	}

	// --- TAB 4: Class Statistics ----------------------------------------------
	void UIManager::RenderClassStatistics()
	{
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		if (pd.currentClass == ClassType::None) {
			ImGuiMCP::TextColored(cGray(), "  No class selected.");
			return;
		}

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(pd.currentClass);
		if (!defOpt) return;
		const auto& def = defOpt.value();
		auto col = CatColor(def.category);

		char title[128];
		snprintf(title, sizeof(title), "  %s %s - CAREER STATISTICS", def.icon, def.name);
		ImGuiMCP::TextColored(col, "%s", title);
		ImGuiMCP::TextColored(cLGold(), "  Playstyle: %s", def.playstyle);
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// Top Dashboard Cards
		ImGuiMCP::Columns(3, "##stat_top_cards", false);
		ImGuiMCP::SetColumnWidth(0, 160.0f);
		ImGuiMCP::SetColumnWidth(1, 180.0f);

		ImGuiMCP::TextColored(cGray(), "Class Level:");
		ImGuiMCP::TextColored(cGold(), "  Level %u", pd.classLevel);
		ImGuiMCP::NextColumn();

		uint32_t reqXP = XPManager::GetSingleton()->GetXPForNextLevel(pd.classLevel);
		ImGuiMCP::TextColored(cGray(), "Current Progress:");
		if (pd.classLevel >= 100) {
			ImGuiMCP::TextColored(cGreen(), "  %u XP (MAX)", pd.classXP);
		} else {
			ImGuiMCP::TextColored(cXP(), "  %u / %u XP", pd.classXP, reqXP);
		}
		ImGuiMCP::NextColumn();
		ImGuiMCP::TextColored(cGray(), "Class Role:");
		ImGuiMCP::TextColored(cGreen(), "  %s", def.name);
		ImGuiMCP::NextColumn();

		ImGuiMCP::Columns(1);
		ImGuiMCP::Spacing();

		// Progress Bar in Statistics
		float statXpPercent = (pd.classLevel >= 100) ? 1.0f : (float)pd.classXP / (float)reqXP;
		char statXpOverlay[64];
		if (pd.classLevel >= 100) {
			snprintf(statXpOverlay, sizeof(statXpOverlay), "MAX LEVEL (100)");
		} else {
			snprintf(statXpOverlay, sizeof(statXpOverlay), "Experience: %u / %u XP (%.1f%%)", pd.classXP, reqXP, statXpPercent * 100.0f);
		}
		ImGuiMCP::ProgressBar(statXpPercent, ImGuiMCP::ImVec2(-1.0f, 28.0f), statXpOverlay);
		ImGuiMCP::Spacing();
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// Two Column Layout for Combat/Role Stats & Class XP Rewards
		ImGuiMCP::Columns(2, "##stat_details_cols", true);
		ImGuiMCP::SetColumnWidth(0, 260.0f);

		// Left: Archetype-Specific Metrics
		ImGuiMCP::TextColored(cGold(), "  Class Performance Metrics:");
		ImGuiMCP::Spacing();

		ImGuiMCP::TextColored(cGray(), "  Total Enemies Slain:");
		ImGuiMCP::SameLine(180.0f);
		ImGuiMCP::Text("%u", pd.stats.enemiesKilled);

		ImGuiMCP::TextColored(cGray(), "  Bosses Defeated:");
		ImGuiMCP::SameLine(180.0f);
		ImGuiMCP::TextColored(cGold(), "%u", pd.stats.bossesDefeated);

		// Specific metric rows based on category
		switch (def.category) {
		case MainCategory::Melee:
			ImGuiMCP::TextColored(cGray(), "  Melee Takedowns:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.meleeKills);

			if (pd.currentClass == ClassType::Thief || pd.currentClass == ClassType::Assassin) {
				ImGuiMCP::TextColored(cGray(), "  Sneak / Critical Kills:");
				ImGuiMCP::SameLine(180.0f);
				ImGuiMCP::Text("%u", pd.stats.sneakKills);

				ImGuiMCP::TextColored(cGray(), "  Locks Picked:");
				ImGuiMCP::SameLine(180.0f);
				ImGuiMCP::Text("%u", pd.stats.locksPicked);
			}
			break;

		case MainCategory::Ranged:
			ImGuiMCP::TextColored(cGray(), "  Archery Kills:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.archeryKills);

			ImGuiMCP::TextColored(cGray(), "  Sneak / Sniping Kills:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.sneakKills);
			break;

		case MainCategory::Magic:
			ImGuiMCP::TextColored(cGray(), "  Spells Cast:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.spellsCast);

			ImGuiMCP::TextColored(cGray(), "  Magic / Arcane Kills:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.magicKills);
			break;

		case MainCategory::Social:
			ImGuiMCP::TextColored(cGray(), "  Items & Goods Sold:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.itemsSold);

			ImGuiMCP::TextColored(cGray(), "  Equipment & Items Crafted:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.itemsCrafted);

			ImGuiMCP::TextColored(cGray(), "  Locks Picked:");
			ImGuiMCP::SameLine(180.0f);
			ImGuiMCP::Text("%u", pd.stats.locksPicked);

			if (pd.currentClass == ClassType::Bard) {
				ImGuiMCP::TextColored(cGray(), "  Songs & Spells Cast:");
				ImGuiMCP::SameLine(180.0f);
				ImGuiMCP::Text("%u", pd.stats.spellsCast);
			}
			break;

		default:
			break;
		}

		ImGuiMCP::TextColored(cGray(), "  Dungeons & Ruins Cleared:");
		ImGuiMCP::SameLine(180.0f);
		ImGuiMCP::Text("%u", pd.stats.dungeonsCleared);

		ImGuiMCP::NextColumn();

		// Right: Shared XP reward guide
		ImGuiMCP::TextColored(cGold(), "  Shared XP Rewards:");
		ImGuiMCP::Spacing();
		ImGuiMCP::TextColored(cXP(), "  NPC defeated"); ImGuiMCP::SameLine(220.0f); ImGuiMCP::TextColored(cGold(), "+8 XP");
		ImGuiMCP::TextColored(cXP(), "  New location discovered"); ImGuiMCP::SameLine(220.0f); ImGuiMCP::TextColored(cGold(), "+20 XP");
		ImGuiMCP::TextColored(cXP(), "  Harvesting, mining, fishing"); ImGuiMCP::SameLine(220.0f); ImGuiMCP::TextColored(cGold(), "+12 XP");
		ImGuiMCP::TextColored(cXP(), "  Quest completed"); ImGuiMCP::SameLine(220.0f); ImGuiMCP::TextColored(cGreen(), "+400 XP");

		ImGuiMCP::Columns(1);
	}

	// ── TAB 5: Followers / Companion Class Management ────────────────────────
	static uint32_t _selectedFollowerFormID = 0;
	static int _selectedFollowerClassPreview = 0;
	static int _followerTabSubView = 0; // 0 = Overview, 1 = Assign Class

	void UIManager::RenderFollowers()
	{
		auto followerMgr = FollowerManager::GetSingleton();
		followerMgr->ScanActiveFollowers(true);
		auto& allFollowers = followerMgr->GetAllFollowers();
		std::vector<FollowerClassData> followers;
		for (auto& f : allFollowers) {
			RE::Actor* actor = followerMgr->FindFollowerActor(f.formID);
			if (actor && FollowerManager::IsAnimalActor(actor)) {
				continue;
			}
			followers.push_back(f);
		}

		ImGuiMCP::TextColored(cGold(), "  COMPANION & FOLLOWER CLASS SYSTEM");
		ImGuiMCP::TextColored(cGray(), "  Empower your party! Assign classes to followers, track their levels, and grow together in combat.");
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		if (followers.empty()) {
			ImGuiMCP::TextColored(cGray(), "  No active followers detected nearby.");
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cLGold(), "  * How to recruit followers to ClassBound:");
			ImGuiMCP::TextColored(cGray(), "    1. Hire or ask a follower (e.g. Lydia, Faendal, Serana, Inigo) to follow you.");
			ImGuiMCP::TextColored(cGray(), "    2. Stand near your companion and reopen this menu.");
			ImGuiMCP::TextColored(cGray(), "    3. Assign them any of the 20 classes to boost their stats and let them gain XP!");
			return;
		}

		// Sub-view navigation buttons (Large & clear)
		if (_followerTabSubView == 0) {
			ImGuiMCP::PushStyleColor(0, cGold());
		}
		if (ImGuiMCP::Button("  [1] Followers Overview  ", ImGuiMCP::ImVec2(240, 36))) {
			_followerTabSubView = 0;
		}
		if (_followerTabSubView == 0) {
			ImGuiMCP::PopStyleColor();
		}

		ImGuiMCP::SameLine();

		if (_followerTabSubView == 1) {
			ImGuiMCP::PushStyleColor(0, cGold());
		}
		if (ImGuiMCP::Button("  [2] Assign / Change Class  ", ImGuiMCP::ImVec2(260, 36))) {
			_followerTabSubView = 1;
		}
		if (_followerTabSubView == 1) {
			ImGuiMCP::PopStyleColor();
		}

		ImGuiMCP::Spacing();
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// VIEW 0: Followers Overview Dashboard
		if (_followerTabSubView == 0) {
			for (size_t idx = 0; idx < followers.size(); ++idx) {
				auto& f = followers[idx];
				bool hasClass = (f.currentClass != ClassType::None);
				auto defOpt = hasClass ? ClassRegistry::GetSingleton()->GetClass(f.currentClass) : std::nullopt;

				// Read live actor stats
				RE::Actor* actor = FollowerManager::GetSingleton()->FindFollowerActor(f.formID);
				float liveHealth = 0.0f, liveMaxHealth = 0.0f;
				float liveMagicka = 0.0f, liveMaxMagicka = 0.0f;
				float liveStamina = 0.0f, liveMaxStamina = 0.0f;
				float skill1H = 0.0f, skill2H = 0.0f, skillBow = 0.0f;
				float skillSneak = 0.0f, skillBlock = 0.0f;
				float skillDestr = 0.0f, skillConj = 0.0f;
				float skillHeavy = 0.0f, skillLight = 0.0f;
				if (actor) {
					auto* avo = actor->AsActorValueOwner();
					if (avo) {
						liveHealth     = avo->GetActorValue(RE::ActorValue::kHealth);
						liveMaxHealth  = avo->GetBaseActorValue(RE::ActorValue::kHealth);
						liveMagicka    = avo->GetActorValue(RE::ActorValue::kMagicka);
						liveMaxMagicka = avo->GetBaseActorValue(RE::ActorValue::kMagicka);
						liveStamina    = avo->GetActorValue(RE::ActorValue::kStamina);
						liveMaxStamina = avo->GetBaseActorValue(RE::ActorValue::kStamina);
						skill1H    = avo->GetBaseActorValue(RE::ActorValue::kOneHanded);
						skill2H    = avo->GetBaseActorValue(RE::ActorValue::kTwoHanded);
						skillBow   = avo->GetBaseActorValue(RE::ActorValue::kArchery);
						skillSneak = avo->GetBaseActorValue(RE::ActorValue::kSneak);
						skillBlock = avo->GetBaseActorValue(RE::ActorValue::kBlock);
						skillDestr = avo->GetBaseActorValue(RE::ActorValue::kDestruction);
						skillConj  = avo->GetBaseActorValue(RE::ActorValue::kConjuration);
						skillHeavy = avo->GetBaseActorValue(RE::ActorValue::kHeavyArmor);
						skillLight = avo->GetBaseActorValue(RE::ActorValue::kLightArmor);
					}
				}

				ImGuiMCP::PushID((int)idx);

				// ── Card Header ─────────────────────────────────────────────
				ImGuiMCP::TextColored(cGold(), "  [Companion] %s", f.name.c_str());
				ImGuiMCP::SameLine();
				if (f.isActiveFollower)
					ImGuiMCP::TextColored(cGreen(), " [Active Follower]");
				else
					ImGuiMCP::TextColored(cGray(), " [Away / Inactive]");
				ImGuiMCP::Spacing();

				// ── Left Block: Class & Progression ──────────────────────────
				ImGuiMCP::BeginGroup();

				ImGuiMCP::TextColored(cGray(), "Class:");
				ImGuiMCP::SameLine(0, 6.0f);
				if (defOpt)
					ImGuiMCP::TextColored(CatColor(defOpt->category), "%s %s  (Lv.%u)",
						defOpt->icon, defOpt->name, f.classLevel);
				else
					ImGuiMCP::TextColored(cRed(), "[No Class Assigned]");

				// XP bar
				if (hasClass) {
					uint32_t reqXP = XPManager::GetSingleton()->GetXPForNextLevel(f.classLevel);
					float pct = (reqXP > 0) ? (float)f.classXP / (float)reqXP : 1.0f;
					char xpLbl[48];
					snprintf(xpLbl, sizeof(xpLbl), "%u / %u XP (%.0f%%)", f.classXP, reqXP, pct * 100.0f);
					ImGuiMCP::ProgressBar(pct, ImGuiMCP::ImVec2(320.0f, 24.0f), xpLbl);
				} else {
					ImGuiMCP::TextColored(cGray(), "Assign a class to start earning XP.");
				}

				// Bonuses — one per line to avoid overflow
				if (hasClass && defOpt) {
					uint32_t tiers = f.classLevel / 5;
					for (const auto& avb : defOpt->avBonuses) {
						float total = avb.base + (avb.perLevel * (float)tiers);
						if (total > 0.0f) {
							ImGuiMCP::TextColored(cGreen(), "  +%d %s", (int)total, avb.label ? avb.label : "");
						}
					}
					ImGuiMCP::Spacing();
				}

				ImGuiMCP::PushStyleColor(0, hasClass ? cLGold() : cGreen());
				char btnTxt[80];
				snprintf(btnTxt, sizeof(btnTxt), "  %s  ", hasClass ? "Change Class" : "Assign a Class");
				if (ImGuiMCP::Button(btnTxt, ImGuiMCP::ImVec2(320.0f, 32.0f))) {
					_selectedFollowerFormID = f.formID;
					_selectedFollowerClassPreview = hasClass ? (int)f.currentClass : (int)ClassType::Warrior;
					_followerTabSubView = 1;
				}
				ImGuiMCP::PopStyleColor();

				ImGuiMCP::EndGroup();

				ImGuiMCP::SameLine(0, 20.0f);  // gap between left and right blocks

				// ── Right Block: Live Skyrim Stats ────────────────────────────
				// Use SameLine(0, gap) everywhere — absolute positions don't work inside
				// a group that's offset from the window edge; they'd cause overlap.
				ImGuiMCP::BeginGroup();

				ImGuiMCP::TextColored(cGold(), "Live Character Stats:");
				ImGuiMCP::Spacing();

				if (actor) {
					// ── Vitals row ────────────────────────────────────────────
					ImGuiMCP::TextColored({ 0.9f,0.25f,0.25f,1.0f }, "[HP]");
					ImGuiMCP::SameLine(0, 4.0f);
					ImGuiMCP::TextColored(cGold(), "%.0f/%.0f", liveHealth, liveMaxHealth);

					ImGuiMCP::SameLine(0, 18.0f);
					ImGuiMCP::TextColored({ 0.4f,0.55f,1.0f,1.0f }, "[MP]");
					ImGuiMCP::SameLine(0, 4.0f);
					ImGuiMCP::TextColored(cGold(), "%.0f/%.0f", liveMagicka, liveMaxMagicka);

					ImGuiMCP::SameLine(0, 18.0f);
					ImGuiMCP::TextColored({ 0.25f,0.85f,0.45f,1.0f }, "[SP]");
					ImGuiMCP::SameLine(0, 4.0f);
					ImGuiMCP::TextColored(cGold(), "%.0f/%.0f", liveStamina, liveMaxStamina);

					ImGuiMCP::Spacing();
					ImGuiMCP::Separator();
					ImGuiMCP::Spacing();

					// ── Combat & Magic Skills ─────────────────────────────────
					ImGuiMCP::TextColored(cMelee(), "1H:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skill1H);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cMelee(), "2H:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skill2H);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cRanged(), "Bow:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillBow);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cMagic(), "Destr:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillDestr);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cMagic(), "Conj:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillConj);

					// ── Armor & Stealth row ───────────────────────────────────
					ImGuiMCP::TextColored(cGray(), "Sneak:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillSneak);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cGray(), "Block:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillBlock);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cGray(), "HvyArm:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillHeavy);
					ImGuiMCP::SameLine(0, 12.0f);
					ImGuiMCP::TextColored(cGray(), "LtArm:");
					ImGuiMCP::SameLine(0, 3.0f); ImGuiMCP::Text("%.0f", skillLight);
				} else {
					ImGuiMCP::TextColored(cGray(), "Stats unavailable (companion not loaded).");
				}

				ImGuiMCP::EndGroup();

				ImGuiMCP::Spacing();
				ImGuiMCP::Separator();
				ImGuiMCP::Spacing();
				ImGuiMCP::PopID();
			}
		}
		// VIEW 1: Assign Class Page
		else {
			// Follower Selector Buttons
			ImGuiMCP::TextColored(cLGold(), "1. Select Companion to Train:");
			ImGuiMCP::Spacing();

			if (_selectedFollowerFormID == 0 && !followers.empty()) {
				_selectedFollowerFormID = followers[0].formID;
			}

			for (auto& f : followers) {
				bool isSel = (_selectedFollowerFormID == f.formID);
				if (isSel) ImGuiMCP::PushStyleColor(0, cGold());
				char btnLbl[128];
				snprintf(btnLbl, sizeof(btnLbl), " [Companion] %s ##%u", f.name.c_str(), f.formID);
				if (ImGuiMCP::Button(btnLbl, ImGuiMCP::ImVec2(180, 32))) {
					_selectedFollowerFormID = f.formID;
				}
				if (isSel) ImGuiMCP::PopStyleColor();
				ImGuiMCP::SameLine();
			}
			ImGuiMCP::NewLine();
			ImGuiMCP::Separator();
			ImGuiMCP::Spacing();

			auto* currentFollower = followerMgr->GetFollowerData(_selectedFollowerFormID);
			if (currentFollower) {
				// Read companion stats for recommendation
				RE::Actor* actor = followerMgr->FindFollowerActor(currentFollower->formID);

				ImGuiMCP::TextColored(cGreen(), "  Selected Companion: %s", currentFollower->name.c_str());
				if (currentFollower->currentClass != ClassType::None) {
					auto currDef = ClassRegistry::GetSingleton()->GetClass(currentFollower->currentClass);
					if (currDef) {
						ImGuiMCP::SameLine();
						ImGuiMCP::TextColored(CatColor(currDef->category), " (Current: %s %s - Level %u)",
							currDef->icon, currDef->name, currentFollower->classLevel);
					}
				} else {
					ImGuiMCP::SameLine();
					ImGuiMCP::TextColored(cRed(), " (No Class Assigned)");
				}
				ImGuiMCP::Spacing();

				// Two Pane Layout: Left = Class Selection List (280px), Right = Details & Assign
				const auto& all = ClassRegistry::GetSingleton()->GetAllClasses();
				const MainCategory cats[] = {
					MainCategory::Melee, MainCategory::Ranged,
					MainCategory::Magic, MainCategory::Social
				};

				ImGuiMCP::BeginChild("##FollowerClassListPane", ImGuiMCP::ImVec2(280.0f, 0.0f), true);
				ImGuiMCP::TextColored(cGold(), "  Available Classes:");
				ImGuiMCP::Spacing();

				for (auto cat : cats) {
					auto col = CatColor(cat);
					bool open = ImGuiMCP::TreeNodeEx(CatLabel(cat), 1 << 5);
					if (open) {
						std::vector<const ClassDefinition*> sorted;
						for (auto& [t, d] : all)
							if (d.category == cat) sorted.push_back(&d);
						std::sort(sorted.begin(), sorted.end(),
							[](auto* a, auto* b){ return (uint32_t)a->type < (uint32_t)b->type; });

						for (auto* def : sorted) {
							bool isSel = (_selectedFollowerClassPreview == (int)def->type);
							bool isAct = (currentFollower->currentClass == def->type);
							
							if (isAct) {
								ImGuiMCP::PushStyleColor(0, cGreen());
							} else if (isSel) {
								ImGuiMCP::PushStyleColor(0, cGold());
							} else {
								ImGuiMCP::PushStyleColor(0, col);
							}

							char lbl[96];
							if (isAct) {
				snprintf(lbl, sizeof(lbl), " [ACTIVE] %s", def->name);
							} else {
				snprintf(lbl, sizeof(lbl), "  %s", def->name);
							}
							
							if (ImGuiMCP::Selectable(lbl, isSel, 0, ImGuiMCP::ImVec2(0, 24)))
								_selectedFollowerClassPreview = (int)def->type;
							ImGuiMCP::PopStyleColor();
						}
						ImGuiMCP::Spacing();
						ImGuiMCP::TreePop();
					}
				}
				ImGuiMCP::EndChild();

				ImGuiMCP::SameLine();

				// Right Pane: Class Details & Clear Assign Action
				ImGuiMCP::BeginChild("##FollowerClassDetailsPane", ImGuiMCP::ImVec2(0.0f, 0.0f), true);
				if (_selectedFollowerClassPreview != 0) {
					auto defOpt = ClassRegistry::GetSingleton()->GetClass(
						static_cast<ClassType>(_selectedFollowerClassPreview));

					if (defOpt) {
						const auto& def = defOpt.value();
						auto col = CatColor(def.category);

						ImGuiMCP::TextColored(col, "  [%s]", CatLabel(def.category));
						ImGuiMCP::SameLine();
						ImGuiMCP::TextColored(cGold(), " %s %s", def.icon, def.name);
						ImGuiMCP::TextColored(cLGold(), "  Role / Playstyle: %s", def.playstyle);
						ImGuiMCP::Separator();
						ImGuiMCP::Spacing();

						// Stat Growth Table
						if (!def.avBonuses.empty()) {
							ImGuiMCP::TextColored(cGold(), "  Companion Class Stat Growth:");
							ImGuiMCP::Spacing();

							ImGuiMCP::Columns(3, "##fstat_growth", false);
							ImGuiMCP::SetColumnWidth(0, 180.0f);
							ImGuiMCP::SetColumnWidth(1, 100.0f);

							ImGuiMCP::TextColored(cGray(), "  Stat");
							ImGuiMCP::NextColumn();
							ImGuiMCP::TextColored(cGray(), "Base (Lv.1)");
							ImGuiMCP::NextColumn();
							ImGuiMCP::TextColored(cGray(), "Every 5 Levels");
							ImGuiMCP::NextColumn();
							ImGuiMCP::Columns(1);
							ImGuiMCP::Separator();

							for (const auto& avb : def.avBonuses) {
								ImGuiMCP::Columns(3, "##fstat_growth_row", false);
								ImGuiMCP::SetColumnWidth(0, 180.0f);
								ImGuiMCP::SetColumnWidth(1, 100.0f);

								ImGuiMCP::TextColored(cXP(), "  %s", avb.label ? avb.label : "Stat");
								ImGuiMCP::NextColumn();
								ImGuiMCP::TextColored(cGreen(), "+%.0f", avb.base);
								ImGuiMCP::NextColumn();
								if (avb.perLevel > 0.0f)
									ImGuiMCP::TextColored(cGold(), "+%.0f", avb.perLevel);
								else
									ImGuiMCP::TextColored(cGray(), " -");
								ImGuiMCP::NextColumn();
								ImGuiMCP::Columns(1);
							}
							ImGuiMCP::Spacing();
							ImGuiMCP::Separator();
							ImGuiMCP::Spacing();
						}

						ImGuiMCP::TextColored(cLGold(), "  Description:");
						ImGuiMCP::TextWrapped("  %s", def.description);
						ImGuiMCP::Spacing();
						ImGuiMCP::Separator();
						ImGuiMCP::Spacing();

						// Assign / Change Class Action
						bool isAlreadyClass = (currentFollower->currentClass == static_cast<ClassType>(_selectedFollowerClassPreview));
						bool hasClass = (currentFollower->currentClass != ClassType::None);
						auto classMgr = ClassChangeManager::GetSingleton();

						if (isAlreadyClass) {
							ImGuiMCP::TextColored(cGreen(), "  [ACTIVE] %s is currently training in %s (Level %u).",
								currentFollower->name.c_str(), def.name, currentFollower->classLevel);
						} else {
							if (!hasClass) {
								// First initial assignment is free
								ImGuiMCP::PushStyleColor(0, cGreen());
								char btnTxt[128];
								snprintf(btnTxt, sizeof(btnTxt), "  Assign %s to %s", def.name, currentFollower->name.c_str());
								if (ImGuiMCP::Button(btnTxt, ImGuiMCP::ImVec2(-1.0f, 38.0f))) {
									followerMgr->AssignClass(currentFollower->formID, static_cast<ClassType>(_selectedFollowerClassPreview));
								}
								ImGuiMCP::PopStyleColor();
							} else {
								// Class change requires Trainer NPC and Gold fee (identical to Player)
								uint32_t cost = ClassChangeManager::GetChangeCost(currentFollower->classLevel);
								if (!classMgr->HasTrainerAccess()) {
									ImGuiMCP::TextColored(cRed(), "To retrain a companion, you must speak with a Trainer NPC first");
									ImGuiMCP::TextColored(cRed(), "(e.g., Faendal, Vilkas, Farengar, Alvor, etc.).");
								} else {
									bool hasGold = classMgr->CanAfford(cost);
									ImGuiMCP::TextColored(hasGold ? cGold() : cRed(), "Companion Retraining Fee: %u Gold", cost);
									if (!hasGold) {
										ImGuiMCP::TextColored(cRed(), "Insufficient gold in inventory.");
									} else {
										ImGuiMCP::PushStyleColor(0, cGold());
										char btnTxt[128];
										snprintf(btnTxt, sizeof(btnTxt), "  Retrain %s as %s (Pay %u Gold)",
											currentFollower->name.c_str(), def.name, cost);
										if (ImGuiMCP::Button(btnTxt, ImGuiMCP::ImVec2(-1.0f, 38.0f))) {
											followerMgr->AssignClass(currentFollower->formID, static_cast<ClassType>(_selectedFollowerClassPreview));
										}
										ImGuiMCP::PopStyleColor();
									}
								}
							}
						}
					}
				} else {
					ImGuiMCP::Spacing();
					ImGuiMCP::TextColored(cGray(), "  Select a class from the list on the left to preview bonuses.");
				}
				ImGuiMCP::EndChild();
			}
		}
	}
	// ── TAB 5b: Bonus Overview ─────────────────────────────────────────────
	void UIManager::RenderBonusOverview()
	{
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		if (pd.currentClass == ClassType::None) {
			ImGuiMCP::TextColored(cGray(), "  No class selected — no bonuses active.");
			return;
		}

		auto classDefOpt = ClassRegistry::GetSingleton()->GetClass(pd.currentClass);
		if (!classDefOpt) return;
		const auto& classDef = classDefOpt.value();

		// ── Helper: AV index → display name ──────────────────────────────────
		auto avName = [](uint32_t idx) -> const char* {
			switch (idx) {
			case 6:  return "One-Handed";
			case 7:  return "Two-Handed";
			case 8:  return "Archery";
			case 9:  return "Block";
			case 10: return "Smithing";
			case 11: return "Heavy Armor";
			case 12: return "Light Armor";
			case 13: return "Pickpocket";
			case 14: return "Lockpicking";
			case 15: return "Sneak";
			case 16: return "Alchemy";
			case 17: return "Speech";
			case 18: return "Alteration";
			case 19: return "Conjuration";
			case 20: return "Destruction";
			case 21: return "Illusion";
			case 22: return "Restoration";
			case 23: return "Enchanting";
			case 24: return "Health";
			case 25: return "Magicka";
			case 26: return "Stamina";
			case 27: return "HealRate";
			case 28: return "MagickaRate";
			case 29: return "StaminaRate";
			case 30: return "SpeedMult";
			case 32: return "CarryWeight";
			case 44: return "ArmorRating";
			case 45: return "PoisonResist";
			case 59: return "OneHandedDmg";
			case 60: return "TwoHandedDmg";
			case 61: return "BowDmg";
			case 62: return "BlockMult";
			case 63: return "SmithingMult";
			case 66: return "PickpocketMult";
			case 68: return "SneakMult";
			case 69: return "AlchemyMult";
			case 70: return "SpeechMult";
			case 72: return "ConjurationMult";
			case 73: return "DestructionMult";
			case 74: return "IllusionMult";
			case 75: return "RestorationMult";
			default: return "Unknown";
			}
		};

		// Helper: draw a bonus row
		auto DrawBonusRow = [&](const char* source, const char* avLabel, float value, ImGuiMCP::ImVec4 srcCol) {
			ImGuiMCP::TextColored(srcCol, "    %-22s", source);
			ImGuiMCP::SameLine();
			ImGuiMCP::TextColored(cLGold(), "%-18s", avLabel);
			ImGuiMCP::SameLine();
			if (value >= 0.0f)
				ImGuiMCP::TextColored(cGreen(), "+%.1f", value);
			else
				ImGuiMCP::TextColored(cRed(), "%.1f", value);
		};

		ImGuiMCP::TextColored(cGold(), "  ALL ACTIVE BONUSES - SOURCE BREAKDOWN");
		ImGuiMCP::Spacing();
		ImGuiMCP::TextColored(cGray(), "  Every bonus applied to your character, grouped by origin.");
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		// Column headers
		ImGuiMCP::TextColored(cGold(), "    %-22s%-18s%s", "Source", "Stat", "Value");
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		bool anyBonus = false;

		// --- 1. Class Base Bonuses --------------------------------------------
		if (!pd.appliedAVBonuses.empty()) {
			anyBonus = true;
			ImGuiMCP::TextColored(cGold(), "  [Class Base] %s (Lv.%u)", classDef.name, pd.classLevel);
			ImGuiMCP::Spacing();
			for (const auto& [avIdx, amount] : pd.appliedAVBonuses) {
				// Find label from classDef
				const char* label = avName(avIdx);
				for (const auto& avb : classDef.avBonuses) {
					if (avb.avIndex == avIdx && avb.label) { label = avb.label; break; }
				}
				DrawBonusRow("Class Passive", label, amount, cMelee());
			}
			ImGuiMCP::Spacing();
		}

		// --- 3. Equipment Affinity Bonuses ------------------------------------
		auto* affMgr = AffinityManager::GetSingleton();
		if (affMgr->IsAffinityActive()) {
			anyBonus = true;
			const auto& affProfile = affMgr->GetCurrentProfile();
			ImGuiMCP::TextColored(cGold(), "  [Equipment Affinity] %s", affProfile.name.c_str());
			ImGuiMCP::Spacing();
			for (const auto& [avIdx, val] : affProfile.bonusAVs) {
				DrawBonusRow("Equip. Set Synergy", avName(avIdx), val, { 0.4f, 0.8f, 1.0f, 1.0f });
			}
			ImGuiMCP::Spacing();
		}

		if (!anyBonus) {
			ImGuiMCP::TextColored(cGray(), "  No bonuses applied yet.");
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cGray(), "  Level up or equip matching gear for your class!");
		}

		// --- Grand Total Summary ----------------------------------------------
		if (anyBonus) {
			ImGuiMCP::Separator();
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cGold(), "  GRAND TOTAL (all sources combined)");
			ImGuiMCP::Spacing();

			// Aggregate all AV bonuses across all sources into a map
			std::unordered_map<uint32_t, float> totals;
			for (const auto& [idx, val] : pd.appliedAVBonuses)    totals[idx] += val;
			if (affMgr->IsAffinityActive()) {
				for (const auto& [idx, val] : affMgr->GetCurrentProfile().bonusAVs) totals[idx] += val;
			}
			ImGuiMCP::TextColored(cGold(), "    %-18s%s", "Stat", "Total");
			ImGuiMCP::Separator();
			for (const auto& [idx, total] : totals) {
				if (total == 0.0f) continue;
				ImGuiMCP::TextColored(cLGold(), "    %-18s", avName(idx));
				ImGuiMCP::SameLine();
				ImGuiMCP::TextColored(total > 0 ? cGreen() : cRed(), "%+.1f", total);
			}
			ImGuiMCP::Spacing();
		}
	}

	// ── TAB 6: Change Class / Class Selection ───────────────────────────────
	void UIManager::RenderChangeClass()
	{
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		const bool hasClass = (pd.currentClass != ClassType::None);
		auto classMgr = ClassChangeManager::GetSingleton();

		if (hasClass) {
			auto d = ClassRegistry::GetSingleton()->GetClass(pd.currentClass);
			if (d) {
				ImGuiMCP::TextColored(cGreen(), "  Current Class: %s (Level %u)", d->name, pd.classLevel);
				ImGuiMCP::SameLine();
				if (!classMgr->HasTrainerAccess()) {
					ImGuiMCP::TextColored(cRed(), "  [Trainer Required]");
				} else {
					ImGuiMCP::TextColored(cGold(), "  [Trainer Verified]");
				}
				ImGuiMCP::Separator();
				ImGuiMCP::Spacing();
			}
		}

		const auto& all = ClassRegistry::GetSingleton()->GetAllClasses();
		const MainCategory cats[] = {
			MainCategory::Melee, MainCategory::Ranged,
			MainCategory::Magic, MainCategory::Social
		};

		// Left Column: Category & Class List
		ImGuiMCP::BeginChild("##ClassListPane", ImGuiMCP::ImVec2(240.0f, 0.0f), true);
		for (auto cat : cats) {
			auto col = CatColor(cat);
			bool open = ImGuiMCP::TreeNodeEx(CatLabel(cat), 1 << 5);
			if (open) {
				ImGuiMCP::TextColored(cGray(), "  %s", CatDesc(cat));
				ImGuiMCP::Spacing();

				std::vector<const ClassDefinition*> sorted;
				for (auto& [t, d] : all)
					if (d.category == cat) sorted.push_back(&d);
				std::sort(sorted.begin(), sorted.end(),
					[](auto* a, auto* b){ return (uint32_t)a->type < (uint32_t)b->type; });

				for (auto* def : sorted) {
					bool isSel = (_selectedPreviewClass == (int)def->type);
					bool isAct = (pd.currentClass == def->type);
					ImGuiMCP::PushStyleColor(0,
						isAct ? cGreen() : isSel ? cGold() : col);
					char lbl[80];
					snprintf(lbl, sizeof(lbl), " %s  %s", def->icon, def->name);
					if (ImGuiMCP::Selectable(lbl, isSel))
						_selectedPreviewClass = (int)def->type;
					ImGuiMCP::PopStyleColor();
				}
				ImGuiMCP::Spacing();
				ImGuiMCP::TreePop();
			}
		}
		ImGuiMCP::EndChild();

		ImGuiMCP::SameLine();

		// Right Column: Class Details Card
		ImGuiMCP::BeginChild("##ClassDetailsPane", ImGuiMCP::ImVec2(0.0f, 0.0f), false);
		if (_selectedPreviewClass != 0) {
			auto defOpt = ClassRegistry::GetSingleton()->GetClass(
				static_cast<ClassType>(_selectedPreviewClass));

			if (defOpt) {
				const auto& def = defOpt.value();
				auto col = CatColor(def.category);

				// Header
				ImGuiMCP::TextColored(col, "  [%s]", CatLabel(def.category));
				ImGuiMCP::SameLine();
				ImGuiMCP::TextColored(cGold(), " %s", def.name);
				ImGuiMCP::TextColored(cLGold(), "  Playstyle: %s", def.playstyle);
				ImGuiMCP::Separator();
				ImGuiMCP::Spacing();

				// ★ STARTING BASE STATS (Moved to Top-Right / Upper Section)
				if (!def.avBonuses.empty()) {
					ImGuiMCP::TextColored(cGold(), "  Starting Base Stats (Level 1):");
					ImGuiMCP::Spacing();

					ImGuiMCP::TextColored(cGray(), "    %-18s", "Stat");
					ImGuiMCP::SameLine(160.0f);
					ImGuiMCP::TextColored(cGray(), "%-8s", "Base");
					ImGuiMCP::SameLine(240.0f);
					ImGuiMCP::TextColored(cGray(), "%s", "+5 Levels");
					ImGuiMCP::Separator();

					for (const auto& avb : def.avBonuses) {
						ImGuiMCP::TextColored(cXP(), "    %s", avb.label ? avb.label : "Stat");
						ImGuiMCP::SameLine(160.0f);
						ImGuiMCP::TextColored(cGreen(), "+%.0f", avb.base);
						ImGuiMCP::SameLine(240.0f);
						if (avb.perLevel > 0.0f)
							ImGuiMCP::TextColored(cGold(), "+%.0f", avb.perLevel);
						else
							ImGuiMCP::TextColored(cGray(), " -");
					}
					ImGuiMCP::Spacing();
					ImGuiMCP::Separator();
					ImGuiMCP::Spacing();
				}

				// Description
				ImGuiMCP::TextColored(cLGold(), "  Description:");
				ImGuiMCP::TextWrapped("  %s", def.description);
				ImGuiMCP::Spacing();
				ImGuiMCP::Separator();
				ImGuiMCP::Spacing();

				// Combat Attributes
				ImGuiMCP::TextColored(cLGold(), "  Combat Attributes:");
				struct S { const char* n; float v; };
				S stats[] = {
					{"Combat  ", def.stats.combat},
					{"Defense ", def.stats.defense},
					{"Magic   ", def.stats.magic},
					{"Stealth ", def.stats.stealth},
					{"Trading ", def.stats.trading},
				};
				for (auto& s : stats) {
					ImGuiMCP::Text("    %s", s.n);
					ImGuiMCP::SameLine();
					char ov[8]; snprintf(ov, sizeof(ov), "%.0f", s.v);
					ImGuiMCP::ImVec2 sz{ 120.0f, 10.0f };
					ImGuiMCP::ProgressBar(s.v / 10.0f, sz, ov);
				}
				ImGuiMCP::Spacing();
				ImGuiMCP::Separator();
				ImGuiMCP::Spacing();

				// Starting Bonuses / Perks
				if (!def.startingBonuses.empty()) {
					ImGuiMCP::TextColored(cLGold(), "  Starting Bonuses:");
					for (const auto& sb : def.startingBonuses) {
						ImGuiMCP::TextColored(cGreen(), "    + %s", sb.c_str());
					}
					ImGuiMCP::Spacing();
					ImGuiMCP::Separator();
					ImGuiMCP::Spacing();
				}

				// Button / Change Logic
				bool isAct = (pd.currentClass == static_cast<ClassType>(_selectedPreviewClass));
				if (isAct) {
					ImGuiMCP::TextColored(cGreen(), "  [ACTIVE] CURRENT ACTIVE CLASS");
				} else {
					if (!hasClass) {
						// Free initial selection (Grants starter items & spells)
						ImGuiMCP::PushStyleColor(0, cGreen());
						ImGuiMCP::ImVec2 btnSz{ -1.0f, 34.0f };
						if (ImGuiMCP::Button("  Select Class & Begin", btnSz)) {
							ClassManager::GetSingleton()->SetPlayerClass(static_cast<ClassType>(_selectedPreviewClass), true);
							_selectedPreviewClass = 0;
						}
						ImGuiMCP::PopStyleColor();
					} else {
						// Class change requires trainer & gold
						uint32_t cost = classMgr->GetChangeCost(pd.classLevel);
						if (!classMgr->HasTrainerAccess()) {
							ImGuiMCP::TextColored(cRed(), "To change class, you must speak with a Trainer NPC");
							ImGuiMCP::TextColored(cRed(), "(e.g., Faendal, Vilkas, Farengar, Alvor, etc.).");
						} else {
							bool hasGold = classMgr->CanAfford(cost);
							ImGuiMCP::TextColored(hasGold ? cGold() : cRed(), "Class Change Fee: %u Gold", cost);
							if (!hasGold) {
								ImGuiMCP::TextColored(cRed(), "Insufficient gold in inventory.");
							} else {
								ImGuiMCP::PushStyleColor(0, cGold());
								ImGuiMCP::ImVec2 btnSz{ -1.0f, 34.0f };
								if (SettingsManager::GetSingleton()->confirmClassChange) {
									if (ImGuiMCP::Button("  Change Class (Pay Gold)", btnSz)) {
										_showConfirmDialog = true;
									}
								} else {
									if (ImGuiMCP::Button("  Change Class (Pay Gold)", btnSz)) {
										ClassChangeManager::GetSingleton()->ExecuteClassChange(static_cast<ClassType>(_selectedPreviewClass));
										_selectedPreviewClass = 0;
									}
								}
								ImGuiMCP::PopStyleColor();
							}
						}
					}
				}
			}
		} else {
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cGray(), "  Select a class from the list on the left to view details.");
		}
		ImGuiMCP::EndChild();
	}

	// ── TAB 6: Settings ──────────────────────────────────────────────────────
	void UIManager::RenderSettings()
	{
		auto settings = SettingsManager::GetSingleton();

		ImGuiMCP::TextColored(cGold(), "  Classbound Settings");
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();

		ImGuiMCP::TextColored(cLGold(), "General System Settings");
		ImGuiMCP::Spacing();

		if (ImGuiMCP::SliderFloat("Shared XP Multiplier", &settings->xpMultiplier, 0.1f, 5.0f, "x%.1f"))
			settings->SaveSettings();
		ImGuiMCP::Spacing();

		if (ImGuiMCP::Checkbox("Enable All Notifications (Master Toggle)", &settings->enableNotifications))
			settings->SaveSettings();
		ImGuiMCP::Spacing();

		if (ImGuiMCP::Checkbox("Shared XP Notifications", &settings->notifyClassXP))
			settings->SaveSettings();
		ImGuiMCP::Spacing();

		if (ImGuiMCP::Checkbox("Level Up Notifications", &settings->notifyClassLevel))
			settings->SaveSettings();
		ImGuiMCP::Spacing();
		
		if (ImGuiMCP::Checkbox("Class Damage Numbers", &settings->showDamageNumbers))
			settings->SaveSettings();
		ImGuiMCP::Spacing();
		
		if (ImGuiMCP::Checkbox("Class Change Confirmation Dialog", &settings->confirmClassChange))
			settings->SaveSettings();
		ImGuiMCP::Spacing();
		
		if (ImGuiMCP::Checkbox("Automatic Shared XP Gains", &settings->automaticClassXP))
			settings->SaveSettings();
		ImGuiMCP::Spacing();
		
		if (ImGuiMCP::Checkbox("Limit Adventurer Level to Character Level", &settings->limitClassLevelToPlayerLevel))
			settings->SaveSettings();
		ImGuiMCP::Spacing();
		
		ImGuiMCP::Separator();
		ImGuiMCP::Spacing();
		if (ImGuiMCP::Checkbox("Enable Cheat / Debug Menu", &settings->enableCheatMenu))
			settings->SaveSettings();
		
		if (settings->enableCheatMenu) {
			ImGuiMCP::Spacing();
			ImGuiMCP::TextColored(cRed(), "  WARNING: Cheating may reduce gameplay enjoyment!");
			ImGuiMCP::Spacing();
			
			auto& pd = ClassManager::GetSingleton()->GetPlayerData();
			
			if (ImGuiMCP::Button("+1 Class Level")) {
				pd.classLevel++;
				if (pd.classLevel > 100) pd.classLevel = 100;
				EffectManager::GetSingleton()->ApplyLevelUpBonus(pd.currentClass, pd.classLevel);
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("+10 Class Level")) {
				for (int i=0; i<10; i++) {
					pd.classLevel++;
					if (pd.classLevel > 100) pd.classLevel = 100;
					EffectManager::GetSingleton()->ApplyLevelUpBonus(pd.currentClass, pd.classLevel);
				}
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("+1000 Class XP")) {
				XPManager::GetSingleton()->AddXP(1000);
			}
			ImGuiMCP::SameLine();
		}
		
		ImGuiMCP::Spacing();
	}
}
