#pragma once
#include "ClassData.h"

namespace Classbound
{
	class ClassManager
	{
	public:
		static ClassManager* GetSingleton()
		{
			static ClassManager singleton;
			return &singleton;
		}

		PlayerClassData& GetPlayerData();
		const PlayerClassData& GetPlayerData() const;

		void SetPlayerClass(ClassType newClass, bool isInitialPick = false);
		bool HasClass() const;

	private:
		ClassManager() = default;
		~ClassManager() = default;

		PlayerClassData _playerData;
	};
}
