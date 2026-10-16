#include "ClassChangeManager.h"
#include "ClassManager.h"
#include "SkillManager.h"
#include "StartingGearManager.h"

namespace Classbound
{
	// ── Trainer NPC base-form IDs ────────────────────────────────────────────
	// Comprehensive list of iconic and official Skyrim skill trainers / masters:
	static constexpr uint32_t kTrainerIDs[] = {
		// Riverwood
		0x0001348C, // Faendal             – Archery Trainer (Common)
		0x00013475, // Alvor               – Riverwood Blacksmith
		0x0001347C, // Lucan Valerius      – Riverwood Merchant

		// Whiterun & Companions
		0x0001A694, // Vilkas              – Two-Handed Master
		0x0001A695, // Farkas              – Heavy Armor Master
		0x0001A698, // Aela the Huntress   – Archery Expert
		0x0001A697, // Athis               – One-Handed Expert
		0x0001A696, // Torvar              – Heavy Armor Journeyman
		0x0001A692, // Njada Stonearm      – Block Expert
		0x0001A672, // Farengar Secret-Fire– Whiterun Court Mage
		0x00013BA8, // Danica Pure-Spring  – Restoration Master
		0x00013BA5, // Amren               – One-Handed (Common)
		0x000B1498, // Arcadia             – Alchemy Expert
		0x0001A681, // Adrianne Avenicci   – Smithing (Common)
		0x0001A693, // Belethor            – Merchant
		0x00013BA1, // Eorlund Gray-Mane   – Smithing Master

		// College of Winterhold (Magic Masters)
		0x0001C1A2, // Faralda             – Destruction Master
		0x0001C1A3, // Phinis Gestor       – Conjuration Master
		0x0001C1A4, // Drevis Neloren      – Illusion Master
		0x0001C1A1, // Colette Marence     – Restoration Master
		0x0001C1A0, // Tolfdir             – Alteration Master
		0x000292B4, // Mirabelle Ervine    – College Master
		0x0002C39C, // Sergius Turrianus   – Enchanting Master

		// Thieves Guild & Riften (Stealth & Speech)
		0x00013267, // Brynjolf            – Thieves Guild
		0x00013264, // Delvin Mallory      – Sneak Master
		0x00013261, // Vex                 – Lockpicking Master
		0x00013262, // Vipir the Fleet     – Pickpocket Master
		0x00013263, // Niruin              – Archery Master
		0x0001334E, // Grelka              – Light Armor Expert
		0x0001334C, // Balimund            – Smithing Expert
		0x00019DD4, // Ungrien             – Speech (Common)

		// Windhelm
		0x00014127, // Torbjorn Shatter-Shield – Two-Handed Expert
		0x00014140, // Angrenor Once-Honored – Two-Handed (Common)
		0x0001B135, // Silda the Unseen    – Pickpocket Master
		0x00014124, // Wuunferth the Unliving – Destruction

		// Solitude & Bards College
		0x0001328D, // Giraud Gemane       – Speech Master
		0x000132AB, // Sybille Stentor     – Illusion Expert
		0x0001328E, // Pantea Ateia        – Speech
		0x0001328F, // Inge Six-Fingers    – Speech
		0x00013260, // Ahtar               – Two-Handed

		// Markarth
		0x00013398, // Ghorza gra-Bagol    – Smithing Expert
		0x0001338B, // Garvey              – Sneak Expert
		0x0001339E, // Hamal               – Restoration Master
		0x000133A1, // Ogmund              – Speech Expert
		0x00013391, // Calcelmo            – Court Wizard

		// Dawnstar, Falkreath, Morthal, Winterhold
		0x000135DD, // Babette             – Alchemy Master (Dark Brotherhood)
		0x000135EB, // Nazir               – Light Armor Master
		0x00013627, // Runil               – Conjuration (Common, Falkreath)
		0x000135E6, // Falion              – Conjuration Master (Morthal)
		0x000135FE, // Lami                – Alchemy (Common, Morthal)
		0x000135D7, // Rustleif            – Smithing (Common, Dawnstar)
		0x00013606, // Dravynea the Stoneweaver – Alteration Expert (Kynesgrove)
		0x0001B114, // Angi                – Archery (Angi's Camp)
	};

	// ── IsTrainerNPC ────────────────────────────────────────────────────────
	bool ClassChangeManager::IsTrainerNPC(uint32_t formID)
	{
		for (auto id : kTrainerIDs) {
			if (id == formID) return true;
		}

		// Also check if this form or actor is a trainer via game classes / factions
		auto* form = RE::TESForm::LookupByID(formID);
		if (form) {
			auto* actor = form->As<RE::Actor>();
			auto* npc = form->As<RE::TESNPC>();
			if (actor && actor->GetActorBase()) {
				npc = actor->GetActorBase();
			}

			if (npc) {
				// 1. Check base form ID in list
				for (auto id : kTrainerIDs) {
					if (id == npc->GetFormID()) return true;
				}

				// 2. Check NPC class name if available
				// BUG FIX #6: GetFullName() can return nullptr — constructing std::string from
				// nullptr is undefined behaviour / crash. Guard with a null check first.
				if (npc->npcClass) {
					const char* rawName = npc->npcClass->GetFullName();
					if (rawName) {
						std::string className = rawName;
						if (className.find("Trainer") != std::string::npos ||
							className.find("Master") != std::string::npos ||
							className.find("Teacher") != std::string::npos ||
							className.find("Mage") != std::string::npos ||
							className.find("Blacksmith") != std::string::npos) {
							return true;
						}
					}
				}

				// 3. Check factions
				for (const auto& rank : npc->factions) {
					if (rank.faction) {
						const char* fn = rank.faction->GetName();
						if (fn && (strstr(fn, "Trainer") || strstr(fn, "JobTrainer") || strstr(fn, "Merchant") || strstr(fn, "JobMerchant") || strstr(fn, "JobBlacksmith"))) {
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	// ── GrantTrainerAccess ──────────────────────────────────────────────────
	void ClassChangeManager::GrantTrainerAccess()
	{
		if (!_trainerAccessGranted) {
			_trainerAccessGranted = true;
			SKSE::log::info("ClassChangeManager: Trainer access granted.");
		}
	}

	// ── GetPlayerGold ───────────────────────────────────────────────────────
	int32_t ClassChangeManager::GetPlayerGold() const
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return 0;

		// Gold001 has base FormID 0xF in Skyrim.esm
		auto* goldForm = RE::TESForm::LookupByID<RE::TESObjectMISC>(0xF);
		if (!goldForm) return 0;

		return player->GetItemCount(goldForm);
	}

	// ── CanAfford ───────────────────────────────────────────────────────────
	bool ClassChangeManager::CanAfford(uint32_t cost) const
	{
		return GetPlayerGold() >= static_cast<int32_t>(cost);
	}

	// ── DeductGold ──────────────────────────────────────────────────────────
	bool ClassChangeManager::DeductGold(uint32_t amount)
	{
		if (!CanAfford(amount)) return false;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return false;

		auto* goldForm = RE::TESForm::LookupByID<RE::TESObjectMISC>(0xF);
		if (!goldForm) return false;

		player->RemoveItem(goldForm, static_cast<int32_t>(amount),
			RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

		SKSE::log::info("ClassChangeManager: Deducted {} gold.", amount);
		return true;
	}

	// ── ExecuteClassChange ───────────────────────────────────────────────────
	bool ClassChangeManager::ExecuteClassChange(ClassType newClass)
	{
		auto* cm = ClassManager::GetSingleton();
		auto& pd = cm->GetPlayerData();
		uint32_t cost = GetChangeCost(pd.classLevel);

		if (!DeductGold(cost)) {
			SKSE::log::warn("ClassChangeManager: Not enough gold for class change.");
			return false;
		}

		ClassType oldClass = pd.currentClass;


		// Shared character XP and level persist when changing class.

		// Set new class (and grant new starting gear for the new class)
		cm->SetPlayerClass(newClass, true);

		// Revoke trainer access — must visit trainer again for next change
		RevokeTrainerAccess();

		SKSE::log::info("ClassChangeManager: Class changed from {} to {} for {} gold.",
			(uint32_t)oldClass, (uint32_t)newClass, cost);
		return true;
	}
}
