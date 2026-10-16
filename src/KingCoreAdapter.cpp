#include "KingCoreAdapter.h"
#include "FollowerManager.h"
#include "ClassRegistry.h"
#include "AffinityManager.h"
#include <RE/Skyrim.h>
#include <algorithm>

namespace Classbound
{
    uint32_t KingCoreAdapter::GetVersion() const noexcept
    {
        return KingCore::API_VERSION;
    }

    KingCore::ClassID KingCoreAdapter::ToKingCoreClass(ClassType ct)
    {
        switch (ct) {
        case ClassType::Warrior:      return KingCore::ClassID::Warrior;
        case ClassType::Knight:       return KingCore::ClassID::Knight;
        case ClassType::Paladin:      return KingCore::ClassID::Paladin;
        case ClassType::Thief:        return KingCore::ClassID::Thief;
        case ClassType::Assassin:     return KingCore::ClassID::Assassin;

        case ClassType::Archer:       return KingCore::ClassID::Archer;
        case ClassType::Hunter:       return KingCore::ClassID::Hunter;
        case ClassType::Marksman:     return KingCore::ClassID::Marksman;
        case ClassType::Gunslinger:   return KingCore::ClassID::Gunslinger;
        case ClassType::Crossbowman:  return KingCore::ClassID::Crossbowman;

        case ClassType::Mage:         return KingCore::ClassID::Mage;
        case ClassType::Necromancer:  return KingCore::ClassID::Necromancer;
        case ClassType::Battlemage:   return KingCore::ClassID::Battlemage;
        case ClassType::Druid:        return KingCore::ClassID::Druid;
        case ClassType::Elementalist: return KingCore::ClassID::Elementalist;

        case ClassType::Merchant:     return KingCore::ClassID::Merchant;
        case ClassType::Bard:         return KingCore::ClassID::Bard;
        case ClassType::Alchemist:    return KingCore::ClassID::Alchemist;
        case ClassType::Blacksmith:   return KingCore::ClassID::Blacksmith;
        case ClassType::Explorer:     return KingCore::ClassID::Explorer;
        default:                      return KingCore::ClassID::Unknown;
        }
    }

    ClassType KingCoreAdapter::ToClassBoundClass(KingCore::ClassID id)
    {
        switch (id) {
        case KingCore::ClassID::Warrior:      return ClassType::Warrior;
        case KingCore::ClassID::Knight:       return ClassType::Knight;
        case KingCore::ClassID::Paladin:      return ClassType::Paladin;
        case KingCore::ClassID::Thief:        return ClassType::Thief;
        case KingCore::ClassID::Assassin:     return ClassType::Assassin;

        case KingCore::ClassID::Archer:       return ClassType::Archer;
        case KingCore::ClassID::Hunter:       return ClassType::Hunter;
        case KingCore::ClassID::Marksman:     return ClassType::Marksman;
        case KingCore::ClassID::Gunslinger:   return ClassType::Gunslinger;
        case KingCore::ClassID::Crossbowman:  return ClassType::Crossbowman;

        case KingCore::ClassID::Mage:         return ClassType::Mage;
        case KingCore::ClassID::Necromancer:  return ClassType::Necromancer;
        case KingCore::ClassID::Battlemage:   return ClassType::Battlemage;
        case KingCore::ClassID::Druid:        return ClassType::Druid;
        case KingCore::ClassID::Elementalist: return ClassType::Elementalist;

        case KingCore::ClassID::Merchant:     return ClassType::Merchant;
        case KingCore::ClassID::Bard:         return ClassType::Bard;
        case KingCore::ClassID::Alchemist:    return ClassType::Alchemist;
        case KingCore::ClassID::Blacksmith:   return ClassType::Blacksmith;
        case KingCore::ClassID::Explorer:     return ClassType::Explorer;
        default:                              return ClassType::None;
        }
    }

    bool KingCoreAdapter::IsActorEligible(RE::Actor* actor) const noexcept
    {
        if (!actor || actor->IsDead()) return false;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (actor == player) return false;

        if (FollowerManager::IsAnimalActor(actor)) return false;

        return true;
    }

    KingCore::ClassID KingCoreAdapter::GetFollowerClass(RE::Actor* actor) const noexcept
    {
        if (!actor) return KingCore::ClassID::Unknown;
        auto* followerMgr = FollowerManager::GetSingleton();
        auto* data = followerMgr->GetFollowerData(actor->GetFormID());
        if (!data && actor->GetActorBase()) {
            data = followerMgr->GetFollowerData(actor->GetActorBase()->GetFormID());
        }
        if (!data || data->currentClass == ClassType::None) {
            return KingCore::ClassID::Unknown;
        }
        return ToKingCoreClass(data->currentClass);
    }

    uint32_t KingCoreAdapter::GetFollowerClassLevel(RE::Actor* actor) const noexcept
    {
        if (!actor) return 1;
        auto* followerMgr = FollowerManager::GetSingleton();
        auto* data = followerMgr->GetFollowerData(actor->GetFormID());
        if (!data && actor->GetActorBase()) {
            data = followerMgr->GetFollowerData(actor->GetActorBase()->GetFormID());
        }
        return data ? data->classLevel : 1;
    }

    std::string_view KingCoreAdapter::GetFollowerSpecialization(RE::Actor* /*actor*/) const noexcept
    {
        return "";
    }

    bool KingCoreAdapter::GetFollowerClassInfo(RE::Actor* actor, KingCore::ClassInfo& outInfo) const noexcept
    {
        if (!actor) return false;
        auto* followerMgr = FollowerManager::GetSingleton();
        auto* data = followerMgr->GetFollowerData(actor->GetFormID());
        if (!data && actor->GetActorBase()) {
            data = followerMgr->GetFollowerData(actor->GetActorBase()->GetFormID());
        }
        if (!data || data->currentClass == ClassType::None) return false;

        outInfo.id = ToKingCoreClass(data->currentClass);
        outInfo.level = data->classLevel;
        outInfo.xp = data->classXP;
        outInfo.displayName = "";

        auto defOpt = ClassRegistry::GetSingleton()->GetClass(data->currentClass);
        if (defOpt) {
            outInfo.identifierName = defOpt->name;
            outInfo.displayName = defOpt->name;
            switch (defOpt->category) {
            case MainCategory::Melee:  outInfo.category = KingCore::ClassCategory::Melee; break;
            case MainCategory::Ranged: outInfo.category = KingCore::ClassCategory::Ranged; break;
            case MainCategory::Magic:  outInfo.category = KingCore::ClassCategory::Magic; break;
            case MainCategory::Social: outInfo.category = KingCore::ClassCategory::Social; break;
            default:                   outInfo.category = KingCore::ClassCategory::None; break;
            }
        }
        return true;
    }

    bool KingCoreAdapter::GetBehaviorProfile(RE::Actor* actor, KingCore::BehaviorProfile& outProfile) const noexcept
    {
        if (!actor) return false;
        auto* followerMgr = FollowerManager::GetSingleton();
        auto* data = followerMgr->GetFollowerData(actor->GetFormID());
        if (!data && actor->GetActorBase()) {
            data = followerMgr->GetFollowerData(actor->GetActorBase()->GetFormID());
        }
        if (!data || data->currentClass == ClassType::None) return false;

        outProfile = KingCore::BehaviorProfile{};
        outProfile.classID = ToKingCoreClass(data->currentClass);

        auto defOpt = ClassRegistry::GetSingleton()->GetClass(data->currentClass);
        if (defOpt) {
            switch (defOpt->category) {
            case MainCategory::Melee:  outProfile.category = KingCore::ClassCategory::Melee; break;
            case MainCategory::Ranged: outProfile.category = KingCore::ClassCategory::Ranged; break;
            case MainCategory::Magic:  outProfile.category = KingCore::ClassCategory::Magic; break;
            case MainCategory::Social: outProfile.category = KingCore::ClassCategory::Social; break;
            default:                   outProfile.category = KingCore::ClassCategory::None; break;
            }
        }

        // Configure class-specific behavioral priorities
        switch (data->currentClass) {
        case ClassType::Warrior:
        case ClassType::Knight:
            outProfile.preferredArmor = KingCore::ArmorPreference::Heavy;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedSword;
            outProfile.weaponWeight = 90;
            outProfile.armorWeight = 95;
            outProfile.healthPotionWeight = 85;
            outProfile.targetHealthPotions = 4;
            break;

        case ClassType::Paladin:
            outProfile.preferredArmor = KingCore::ArmorPreference::Heavy;
            outProfile.preferredWeapon = KingCore::WeaponPreference::ShieldBearer;
            outProfile.weaponWeight = 85;
            outProfile.armorWeight = 90;
            outProfile.healthPotionWeight = 80;
            outProfile.magickaPotionWeight = 60;
            outProfile.targetHealthPotions = 3;
            outProfile.targetMagickaPotions = 2;
            break;

        case ClassType::Thief:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedDagger;
            outProfile.prefersStealth = true;
            outProfile.goldWeight = 95;
            outProfile.staminaPotionWeight = 80;
            break;

        case ClassType::Assassin:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedDagger;
            outProfile.prefersStealth = true;
            outProfile.poisonWeight = 95;
            outProfile.goldWeight = 85;
            outProfile.healthPotionWeight = 70;
            break;

        case ClassType::Archer:
        case ClassType::Hunter:
        case ClassType::Marksman:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::Bow;
            outProfile.ammoWeight = 100;
            outProfile.targetAmmoCount = 80;
            outProfile.canCraftAmmo = true;
            outProfile.staminaPotionWeight = 90;
            outProfile.targetStaminaPotions = 3;
            break;

        case ClassType::Crossbowman:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::Crossbow;
            outProfile.ammoWeight = 100;
            outProfile.targetAmmoCount = 60;
            outProfile.canCraftAmmo = true;
            break;

        case ClassType::Mage:
        case ClassType::Elementalist:
            outProfile.preferredArmor = KingCore::ArmorPreference::Cloth;
            outProfile.preferredWeapon = KingCore::WeaponPreference::PureMagic;
            outProfile.magickaPotionWeight = 100;
            outProfile.targetMagickaPotions = 6;
            outProfile.soulGemWeight = 85;
            outProfile.spellTomeWeight = 90;
            break;

        case ClassType::Necromancer:
            outProfile.preferredArmor = KingCore::ArmorPreference::Cloth;
            outProfile.preferredWeapon = KingCore::WeaponPreference::DestructionStaff;
            outProfile.magickaPotionWeight = 90;
            outProfile.soulGemWeight = 100;
            outProfile.spellTomeWeight = 85;
            break;

        case ClassType::Battlemage:
            outProfile.preferredArmor = KingCore::ArmorPreference::Heavy;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedSword;
            outProfile.magickaPotionWeight = 80;
            outProfile.healthPotionWeight = 80;
            outProfile.targetHealthPotions = 3;
            outProfile.targetMagickaPotions = 3;
            break;

        case ClassType::Druid:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::PureMagic;
            outProfile.alchemyIngredientWeight = 80;
            outProfile.healthPotionWeight = 85;
            outProfile.canCraftPotions = true;
            break;

        case ClassType::Merchant:
            outProfile.isMerchantMinded = true;
            outProfile.goldWeight = 100;
            outProfile.craftingMaterialWeight = 70;
            outProfile.alchemyIngredientWeight = 70;
            break;

        case ClassType::Bard:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedSword;
            outProfile.goldWeight = 80;
            outProfile.staminaPotionWeight = 60;
            outProfile.healthPotionWeight = 60;
            break;

        case ClassType::Explorer:
            outProfile.preferredArmor = KingCore::ArmorPreference::Light;
            outProfile.preferredWeapon = KingCore::WeaponPreference::OneHandedSword;
            outProfile.staminaPotionWeight = 80;
            outProfile.healthPotionWeight = 70;
            outProfile.targetStaminaPotions = 3;
            break;

        case ClassType::Alchemist:
            outProfile.alchemyIngredientWeight = 100;
            outProfile.healthPotionWeight = 85;
            outProfile.magickaPotionWeight = 85;
            outProfile.staminaPotionWeight = 85;
            outProfile.poisonWeight = 85;
            outProfile.canCraftPotions = true;
            break;

        case ClassType::Blacksmith:
            outProfile.craftingMaterialWeight = 100;
            outProfile.weaponWeight = 85;
            outProfile.armorWeight = 90;
            break;

        default:
            break;
        }

        // Class level scaling: scale ammo stock and potion quotas
        if (data->classLevel > 20) {
            outProfile.targetAmmoCount = static_cast<uint16_t>(outProfile.targetAmmoCount * 1.5f);
            outProfile.targetHealthPotions += 2;
        }

        return true;
    }

    void KingCoreAdapter::RegisterEventListener(KingCore::IClassEventListener* listener) noexcept
    {
        if (!listener) return;
        std::lock_guard lock(_listenerMutex);
        auto it = std::find(_listeners.begin(), _listeners.end(), listener);
        if (it == _listeners.end()) {
            _listeners.push_back(listener);
        }
    }

    void KingCoreAdapter::UnregisterEventListener(KingCore::IClassEventListener* listener) noexcept
    {
        std::lock_guard lock(_listenerMutex);
        _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
    }

    bool KingCoreAdapter::AssignFollowerClass(RE::Actor* actor, KingCore::ClassID newClass) noexcept
    {
        if (!actor) return false;
        ClassType cbClass = ToClassBoundClass(newClass);
        if (cbClass == ClassType::None) return false;

        ClassType oldClass = ClassType::None;
        auto* data = FollowerManager::GetSingleton()->GetFollowerData(actor->GetFormID());
        if (data) {
            oldClass = data->currentClass;
        }

        bool result = FollowerManager::GetSingleton()->AssignClass(actor->GetFormID(), cbClass);
        if (result) {
            NotifyClassEvent(KingCore::ClassEventType::ClassChanged, actor, ToKingCoreClass(oldClass), newClass, 1);
        }
        return result;
    }

    void KingCoreAdapter::NotifyClassEvent(KingCore::ClassEventType type, RE::Actor* actor, KingCore::ClassID oldClass, KingCore::ClassID newClass, uint32_t newLevel)
    {
        // Snapshot under lock — then call out lock-free to prevent reentrancy deadlock
        // (a listener that calls back into RegisterEventListener would otherwise deadlock).
        std::vector<KingCore::IClassEventListener*> listenersCopy;
        {
            std::lock_guard lock(_listenerMutex);
            if (_listeners.empty()) return;
            listenersCopy = _listeners;
        }

        KingCore::ClassEvent ev{
            .type = type,
            .actorFormID = actor ? actor->GetFormID() : 0,
            .actor = actor,
            .oldClass = oldClass,
            .newClass = newClass,
            .newLevel = newLevel
        };

        for (auto* listener : listenersCopy) {
            if (listener) {
                listener->OnClassEvent(ev);
            }
        }
    }
}
