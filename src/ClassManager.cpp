#include "ClassManager.h"
#include "EffectManager.h"
#include "ClassRegistry.h"
#include "SkillManager.h"
#include "StartingGearManager.h"
#include "AffinityManager.h"
#include <SKSE/SKSE.h>

namespace Classbound
{
	PlayerClassData& ClassManager::GetPlayerData()
	{
		return _playerData;
	}

	const PlayerClassData& ClassManager::GetPlayerData() const
	{
		return _playerData;
	}

	void ClassManager::SetPlayerClass(ClassType newClass, bool isInitialPick)
	{
		// Remove old effects (using exact tracked amounts)
		if (_playerData.currentClass != ClassType::None) {
			EffectManager::GetSingleton()->RemoveClassEffects(_playerData.currentClass);
		}

		_playerData.currentClass = newClass;

		AffinityManager::GetSingleton()->ClearBonuses();

		// Apply new class effects at the current shared class level
		EffectManager::GetSingleton()->ApplyClassEffects(newClass, _playerData.classLevel);
		AffinityManager::GetSingleton()->EvaluateAffinity();

		// Grant thematic starting equipment and spells ONLY on initial class selection
		if (isInitialPick) {
			StartingGearManager::GetSingleton()->GrantStartingGear(newClass);
		}

		SKSE::log::info("Player class changed to: {} (InitialPick: {})", static_cast<uint32_t>(newClass), isInitialPick);
	}

	bool ClassManager::HasClass() const
	{
		return _playerData.currentClass != ClassType::None;
	}
}
