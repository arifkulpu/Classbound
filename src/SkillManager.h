#pragma once
#include "ClassData.h"
#include <optional>

namespace Classbound
{
	class SkillManager
	{
	public:
		static SkillManager* GetSingleton()
		{
			static SkillManager singleton;
			return &singleton;
		}

		// Active temp buffs: {avIndex, amount, expireTime (seconds epoch)}
		// Public so SaveManager can serialize and expire stale buffs on load (Bug Fix #3)
		struct TempBuff {
			uint32_t avIndex;
			float    amount;
			double   expireTime;  // std::chrono epoch in seconds
		};

		// Called periodically to expire temporary active skill buffs
		void TickTempBuffs();

		// Public accessors for SaveManager serialization (Bug Fixes #2/#3)
		std::vector<TempBuff>& GetTempBuffs() { return _tempBuffs; }
		double NowSecondsPublic() const { return NowSeconds(); }
		void ExpireBuffPublic(const TempBuff& buff) { ExpireBuff(buff); }

	private:
		SkillManager() = default;
		~SkillManager() = default;

		std::vector<TempBuff> _tempBuffs;

		double NowSeconds() const;
		void ExpireBuff(const TempBuff& buff);
	};
}
