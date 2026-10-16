#pragma once
#include <SKSE/SKSE.h>

namespace Classbound
{
	class SaveManager
	{
	public:
		static void Register();

	private:
		static constexpr uint32_t kSerializationVersion = 13;
		static constexpr uint32_t kClassDataRecord = 'CLSD';
		static constexpr uint32_t kFollowerDataRecord = 'FLWR';

		static void SaveCallback(SKSE::SerializationInterface* a_intfc);
		static void LoadCallback(SKSE::SerializationInterface* a_intfc);
		static void RevertCallback(SKSE::SerializationInterface* a_intfc);
	};
}
