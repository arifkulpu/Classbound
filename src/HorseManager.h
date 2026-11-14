#pragma once
#include <atomic>
#include <cstdint>

namespace RE
{
	class Actor;
	class BGSLocation;
}

namespace Classbound
{
	// Sınıf atını (Knight / Merchant) şehre girince / şehre ışınlanınca en yakın ahıra park eder.
	// Takip işini vanilla "son binilen sahipli at" mantığı yapar.
	class HorseManager
	{
	public:
		static HorseManager* GetSingleton()
		{
			static HorseManager singleton;
			return &singleton;
		}

		void Start();                 // kDataLoaded'da bir kez çağır
		void Track(RE::Actor* horse); // at spawn / load sonrası çağır
		void Update();                // Ana oyun döngüsünden güvenli çağrı

	private:
		HorseManager() = default;
		void Tick();
		RE::Actor* ResolveHorse();
		bool IsClassHorse(RE::Actor* actor) const;
		static bool IsTownLocation(const RE::BGSLocation* loc);
		void ParkAtNearestStable(RE::Actor* horse);

		std::atomic<bool> _started{ false };
		std::uint32_t _horseFormID{ 0 };
		bool _parked{ false };
		std::uint32_t _scanCooldown{ 0 };
		uint64_t _lastTickMs{ 0 };
		uint64_t _lastSpawnOrDismountMs{ 0 };
	};
}
