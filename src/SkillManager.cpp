#include "SkillManager.h"
#include "ClassManager.h"
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <chrono>
#include <algorithm>

namespace Classbound
{
	double SkillManager::NowSeconds() const
	{
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		return std::chrono::duration<double>(duration).count();
	}

	void SkillManager::TickTempBuffs()
	{
		if (_tempBuffs.empty()) return;
		
		double now = NowSeconds();
		
		// Iterate backwards to safely erase
		for (auto it = _tempBuffs.end(); it != _tempBuffs.begin(); ) {
			--it;
			if (now >= it->expireTime) {
				ExpireBuff(*it);
				it = _tempBuffs.erase(it);
			}
		}
	}
	
	void SkillManager::ExpireBuff(const TempBuff& buff)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;
		
		auto* avo = player->AsActorValueOwner();
		if (!avo) return;
		
		auto av = static_cast<RE::ActorValue>(buff.avIndex);
		float current = avo->GetBaseActorValue(av);
		avo->SetActorValue(av, current - buff.amount);
		
		SKSE::log::info("Expired Temp Boost {} (reverted {} amount)", buff.avIndex, buff.amount);
	}
}
