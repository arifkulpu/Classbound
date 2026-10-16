#include "AffinityManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace Classbound
{
	const AffinityProfile& AffinityManager::GetProfileForClass(ClassType type) const
	{
		static const AffinityProfile kDefaultProfile{ "None", "None", "No equipment affinity active.", {} };

		static const std::unordered_map<ClassType, AffinityProfile> kProfiles = {
			// Melee
			{ ClassType::Warrior, {
				"Titan's Might",
				"3+ Pieces Heavy Armor & Two-Handed Weapon",
				"+15% Two-Handed Power Attack Damage & +25% Stamina Regeneration.",
				{ { 60, 15.0f }, { 29, 25.0f } }
			}},
			{ ClassType::Knight, {
				"Impenetrable Fortress",
				"3+ Pieces Heavy Armor & Shield",
				"+50 Armor Rating & +20% Shield Block Efficiency.",
				{ { 44, 50.0f }, { 62, 20.0f } }
			}},
			{ ClassType::Paladin, {
				"Holy Bastion",
				"Heavy Armor & Restoration Magic / Shield",
				"+20% Restoration Spell Efficiency & +25% Health Regeneration.",
				{ { 75, 20.0f }, { 27, 25.0f } }
			}},
			{ ClassType::Thief, {
				"Silent Infiltrator",
				"Light Armor / Clothes with No Heavy Armor",
				"+20% Sneak Efficiency & +20% Pickpocket Success.",
				{ { 68, 20.0f }, { 66, 20.0f } }
			}},
			{ ClassType::Assassin, {
				"Shadow Strike",
				"Light Armor / Clothes & Dagger in Hand",
				"+25% Sneak Attack Damage & +10% Movement Speed.",
				{ { 68, 25.0f }, { 30, 10.0f } }
			}},

			// Ranged
			{ ClassType::Archer, {
				"Hawkeye Synergy",
				"3+ Pieces Light Armor & Bow/Crossbow",
				"+15% Archery Damage & +10% Movement Speed.",
				{ { 61, 15.0f }, { 30, 10.0f } }
			}},
			{ ClassType::Hunter, {
				"Wilderness Tracker",
				"Light Armor / Hide & Bow or 1H Weapon",
				"+20% Archery Damage & +35% Poison Resistance.",
				{ { 61, 20.0f }, { 45, 35.0f } }
			}},
			{ ClassType::Marksman, {
				"Deadly Precision",
				"Light Armor & Bow/Crossbow",
				"+25% Ranged Critical Damage & +15% Stamina Regen.",
				{ { 61, 25.0f }, { 29, 15.0f } }
			}},
			{ ClassType::Gunslinger, {
				"Quickdraw Outlaw",
				"Light Armor & Firearm / Crossbow / Bow Equipped",
				"+20% Ranged Damage, +15% Stamina Regen, & +10% Movement Speed.",
				{ { 61, 20.0f }, { 29, 15.0f }, { 30, 10.0f } }
			}},
			{ ClassType::Crossbowman, {
				"Marksman's Stance",
				"Light Armor + Crossbow/Bow Equipped",
				"+20% Archery Damage & +15% Stamina Regeneration.",
				{ { 8, 20.0f }, { 29, 15.0f } }
			}},

			// Magic
			{ ClassType::Mage, {
				"Arcane Scholar & Staff Focus",
				"Pure Robes/Cloth (No Armor) + Magic Staff or Spell",
				"+30% Magicka Regen, +30 Max Magicka, & +15% Staff/Destruction Magnitude.",
				{ { 28, 30.0f }, { 25, 30.0f }, { 73, 15.0f } }
			}},
			{ ClassType::Necromancer, {
				"Death's Embrace",
				"Robes / Cloth & Conjuration Spell",
				"+25% Conjuration Spell Effectiveness & +25 Magicka.",
				{ { 72, 25.0f }, { 25, 25.0f } }
			}},
			{ ClassType::Battlemage, {
				"Spellsword Mastery",
				"Heavy Armor & Destruction Spell / One-Handed Sword",
				"+15% Destruction Spell Power & +15% One-Handed Damage.",
				{ { 73, 15.0f }, { 59, 15.0f } }
			}},
			{ ClassType::Druid, {
				"Nature's Harmony",
				"Clothes / Light Armor & Nature Spell",
				"+30% Poison Resistance & +20% Health/Magicka Regen.",
				{ { 45, 30.0f }, { 27, 20.0f }, { 28, 20.0f } }
			}},
			{ ClassType::Elementalist, {
				"Prismatic Surge",
				"Robes / Cloth & Destruction Magic",
				"+20% Destruction Spell Magnitude & +25 Magicka.",
				{ { 73, 20.0f }, { 25, 25.0f } }
			}},

			// Social & Lifestyle
			{ ClassType::Merchant, {
				"Master Trader",
				"Fine Clothes / Merchant Apparel & Jewelry",
				"+15% Better Barter Prices & +50 Carry Weight.",
				{ { 70, 15.0f }, { 32, 50.0f } }
			}},
			{ ClassType::Bard, {
				"Lyrical Grace",
				"Clothes / Light Armor & Illusion Spell",
				"+20% Speechcraft & +20% Illusion Spell Efficiency.",
				{ { 70, 20.0f }, { 74, 20.0f } }
			}},
			{ ClassType::Alchemist, {
				"Apothecary's Touch",
				"Cloth / Light Armor & Dagger / Poison",
				"+25% Alchemy Potency & +50% Poison Resistance.",
				{ { 69, 25.0f }, { 45, 50.0f } }
			}},
			{ ClassType::Blacksmith, {
				"Forge Master",
				"Heavy Armor / Apron & Mace / War Axe",
				"+20% Smithing Quality & +35 Armor Rating.",
				{ { 63, 20.0f }, { 44, 35.0f } }
			}},
			{ ClassType::Explorer, {
				"Wayfarer",
				"Light Armor / Boots & One-Handed Weapon / Torch",
				"+50 Carry Weight, +10% Movement Speed & +25% Stamina Regen.",
				{ { 32, 50.0f }, { 30, 10.0f }, { 29, 25.0f } }
			}}
		};

		auto it = kProfiles.find(type);
		if (it != kProfiles.end()) {
			return it->second;
		}
		return kDefaultProfile;
	}

	void AffinityManager::ModifyPlayerAV(uint32_t avIndex, float delta)
	{
		if (delta == 0.0f) return;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;
		auto* avo = player->AsActorValueOwner();
		if (!avo) return;

		auto av = static_cast<RE::ActorValue>(avIndex);
		float current = avo->GetBaseActorValue(av);
		avo->SetActorValue(av, current + delta);
	}

	void AffinityManager::ApplyBonuses(const AffinityProfile& profile)
	{
		ClearBonuses();
		for (const auto& [av, val] : profile.bonusAVs) {
			ModifyPlayerAV(av, val);
			_appliedBonuses.push_back({ av, val });
		}
		_isAffinityActive = true;
		_currentProfile = profile;

		char msg[128];
		snprintf(msg, sizeof(msg), "Class Affinity Active: %s!", profile.name.c_str());
		RE::DebugNotification(msg);
		SKSE::log::info("AffinityManager: Synergy activated.");
	}

	void AffinityManager::ClearBonuses()
	{
		for (const auto& [av, val] : _appliedBonuses) {
			ModifyPlayerAV(av, -val);
		}
		_appliedBonuses.clear();
		_isAffinityActive = false;
	}

	void AffinityManager::ResetOnLoad(bool wasActive, bool stateKnown)
	{
		// Reconstruct _appliedBonuses and mark active using the current class profile,
		// but do NOT call ModifyPlayerAV — AV values are already in the native save.
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		if (pd.currentClass == ClassType::None) {
			_isAffinityActive = false;
			_appliedBonuses.clear();
			return;
		}

		bool criteriaMet = CheckCriteria(pd.currentClass);
		// Legacy records did not save the flag. Assume the native save may contain the
		// previous affinity bonus so it can be removed if equipment no longer qualifies.
		if (!stateKnown) wasActive = true;
		if (!criteriaMet && wasActive) {
			const auto& profile = GetProfileForClass(pd.currentClass);
			for (const auto& [av, val] : profile.bonusAVs) ModifyPlayerAV(av, -val);
			_isAffinityActive = false;
			_appliedBonuses.clear();
			return;
		}
		if (!criteriaMet) {
			_isAffinityActive = false;
			_appliedBonuses.clear();
			return;
		}
		if (!wasActive) {
			ApplyBonuses(GetProfileForClass(pd.currentClass));
			return;
		}

		const auto& profile = GetProfileForClass(pd.currentClass);
		_appliedBonuses.clear();
		for (const auto& [av, val] : profile.bonusAVs) {
			_appliedBonuses.push_back({ av, val });
		}
		_currentProfile = profile;
		_isAffinityActive = wasActive;

		SKSE::log::info("AffinityManager::ResetOnLoad: Affinity '{}' restored (no AV re-apply).", profile.name);
	}

	bool AffinityManager::CheckCriteria(ClassType type)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return false;

		int heavyCount = 0;
		int lightCount = 0;
		int clothingCount = 0;
		bool hasShield = false;
		bool has2H = false;
		bool hasBow = false;
		bool hasCrossbow = false;
		bool hasDagger = false;
		bool has1H = false;
		bool hasStaff = false;
		bool hasDestruction = false;
		bool hasRestoration = false;
		bool hasConjuration = false;
		bool hasIllusion = false;
		bool hasFineClothes = false;

		auto inv = player->GetInventory();
		for (const auto& [item, data] : inv) {
			if (!item || data.second->IsWorn() == false) continue;

			if (auto* armo = item->As<RE::TESObjectARMO>()) {
				auto armorType = armo->GetArmorType();
				if (armorType == RE::BGSBipedObjectForm::ArmorType::kHeavyArmor) {
					heavyCount++;
				} else if (armorType == RE::BGSBipedObjectForm::ArmorType::kLightArmor) {
					lightCount++;
				} else if (armorType == RE::BGSBipedObjectForm::ArmorType::kClothing) {
					clothingCount++;
					if (armo->HasKeywordString("ClothingRich") || armo->HasKeywordString("ClothingBody")) {
						hasFineClothes = true;
					}
				}
				if (armo->HasKeywordString("ArmorShield") || armo->HasKeywordString("BlockShield")) {
					hasShield = true;
				}
			} else if (auto* weap = item->As<RE::TESObjectWEAP>()) {
				if (weap->IsTwoHandedSword() || weap->IsTwoHandedAxe()) {
					has2H = true;
				} else if (weap->IsBow()) {
					hasBow = true;
				} else if (weap->IsCrossbow()) {
					hasCrossbow = true;
				} else if (weap->IsStaff()) {
					hasStaff = true;
				} else if (weap->IsOneHandedDagger()) {
					hasDagger = true;
				} else if (weap->IsOneHandedSword() || weap->IsOneHandedAxe() || weap->IsOneHandedMace()) {
					has1H = true;
				}
			}
		}

		auto checkSpell = [&](RE::TESForm* form) {
			if (!form) return;
			if (auto* spell = form->As<RE::SpellItem>()) {
				auto school = spell->GetAssociatedSkill();
				if (school == RE::ActorValue::kDestruction) hasDestruction = true;
				else if (school == RE::ActorValue::kRestoration) hasRestoration = true;
				else if (school == RE::ActorValue::kConjuration) hasConjuration = true;
				else if (school == RE::ActorValue::kIllusion) hasIllusion = true;
			}
		};

		checkSpell(player->GetEquippedObject(false));
		checkSpell(player->GetEquippedObject(true));

		switch (type) {
		case ClassType::Warrior:
			return (heavyCount >= 3 && has2H);
		case ClassType::Knight:
			return (heavyCount >= 3 && hasShield);
		case ClassType::Paladin:
			return (heavyCount >= 2 && (hasRestoration || hasShield));
		case ClassType::Thief:
			return (heavyCount == 0 && (lightCount >= 2 || clothingCount >= 2));
		case ClassType::Assassin:
			return (heavyCount == 0 && hasDagger);
		case ClassType::Archer:
		case ClassType::Marksman:
			return (lightCount >= 2 && (hasBow || hasCrossbow));
		case ClassType::Hunter:
			return (lightCount >= 2 && (hasBow || hasCrossbow || has1H));
		case ClassType::Gunslinger:
			return (lightCount >= 2 && (hasBow || hasCrossbow || has1H));
		case ClassType::Crossbowman:
			return (lightCount >= 2 && (hasCrossbow || hasBow));
		case ClassType::Mage:
			return (heavyCount == 0 && lightCount == 0 && clothingCount >= 1 && (hasStaff || hasDestruction || hasConjuration || hasRestoration || hasIllusion));
		case ClassType::Necromancer:
			return (heavyCount == 0 && hasConjuration);
		case ClassType::Battlemage:
			return (heavyCount >= 2 && (hasDestruction || has1H || hasStaff));
		case ClassType::Druid:
			return (heavyCount == 0 && (hasRestoration || hasConjuration || hasStaff));
		case ClassType::Elementalist:
			return (heavyCount == 0 && (hasDestruction || hasStaff));
		case ClassType::Merchant:
			return (hasFineClothes || (heavyCount == 0 && clothingCount >= 2));
		case ClassType::Bard:
			return (heavyCount == 0 && (hasIllusion || clothingCount >= 2));
		case ClassType::Alchemist:
			return (heavyCount == 0 && (hasDagger || clothingCount >= 1));
		case ClassType::Blacksmith:
			return (heavyCount >= 2 || clothingCount >= 1);
		case ClassType::Explorer:
			return (lightCount >= 2 || clothingCount >= 2);
		default:
			return false;
		}
	}

	void AffinityManager::EvaluateAffinity()
	{
		auto& pd = ClassManager::GetSingleton()->GetPlayerData();
		if (pd.currentClass == ClassType::None) {
			ClearBonuses();
			return;
		}

		const auto& profile = GetProfileForClass(pd.currentClass);
		bool criteriaMet = CheckCriteria(pd.currentClass);

		if (criteriaMet && !_isAffinityActive) {
			ApplyBonuses(profile);
		} else if (!criteriaMet && _isAffinityActive) {
			ClearBonuses();
			RE::DebugNotification("Class Affinity deactivated (Equipment changed).");
		}
	}
}
