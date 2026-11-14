#include "HorseManager.h"
#include "ClassManager.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <chrono>
#include <cmath>
#include <limits>

namespace Classbound
{
	namespace
	{
		constexpr const char*   kHorsePlugin           = "ClassBoundHorse.esl";
		constexpr std::uint32_t kHorseBaseLocalID      = 0x800;
		constexpr std::uint32_t kStableListLocalID     = 0x801;
		constexpr std::uint32_t kVanillaHorseBase      = 0x00023AB2;
		constexpr std::uint32_t kPlayerHorseFaction    = 0x00068D78;

		// Fallback hardcoded 5 major Skyrim city stable horse markers
		constexpr std::uint32_t kFallbackStableMarkers[] = {
			0x00109826, // Whiterun Stables
			0x00109828, // Solitude Stables
			0x00109825, // Windhelm Stables
			0x0010982C, // Markarth Stables
			0x00109829  // Riften Stables
		};

		template <class T>
		T* LookupLight(RE::TESDataHandler* dh, std::uint32_t localID)
		{
			if (!dh) return nullptr;
			if (auto* f = dh->LookupForm<T>(localID, kHorsePlugin)) return f;
			if (const auto idx = dh->GetLoadedLightModIndex(kHorsePlugin)) {
				const RE::FormID rt = 0xFE000000u | (static_cast<RE::FormID>(*idx) << 12) | (localID & 0xFFFu);
				if (auto* f = RE::TESForm::LookupByID(rt)) return f->As<T>();
			}
			return nullptr;
		}
	}

	void HorseManager::Start()
	{
		_started.store(true, std::memory_order_relaxed);
		SKSE::log::info("HorseManager: started.");
	}

	void HorseManager::Track(RE::Actor* horse)
	{
		if (!horse) return;
		_horseFormID = horse->GetFormID();
		_parked = true; // Yeni spawn olduğunda hemen ahıra ışınlanmasın
		_lastSpawnOrDismountMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
		SKSE::log::info("HorseManager: tracking horse {:08X}.", _horseFormID);
	}

	bool HorseManager::IsClassHorse(RE::Actor* actor) const
	{
		if (!actor || actor->IsDead()) return false;
		auto* base = actor->GetActorBase();
		if (!base) return false;
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (auto* eslBase = LookupLight<RE::TESNPC>(dh, kHorseBaseLocalID); eslBase && base == eslBase) return true;
		if (base->GetFormID() == kVanillaHorseBase) {
			auto* faction = dh ? dh->LookupForm<RE::TESFaction>(kPlayerHorseFaction, "Skyrim.esm") : nullptr;
			return faction && actor->IsInFaction(faction);
		}
		return false;
	}

	RE::Actor* HorseManager::ResolveHorse()
	{
		if (_horseFormID) {
			if (auto* a = RE::TESForm::LookupByID<RE::Actor>(_horseFormID); a && IsClassHorse(a)) return a;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			if (auto ptr = player->GetInfoRuntimeData().lastRiddenMount.get(); ptr && IsClassHorse(ptr.get())) {
				_horseFormID = ptr->GetFormID();
				return ptr.get();
			}
		}
		if (_scanCooldown > 0) { --_scanCooldown; return nullptr; }
		_scanCooldown = 6; // ~9 saniyede bir tarama
		RE::Actor* found = nullptr;
		if (auto* lists = RE::ProcessLists::GetSingleton()) {
			lists->ForAllActors([&](RE::Actor* actor) {
				if (IsClassHorse(actor)) { found = actor; return RE::BSContainer::ForEachResult::kStop; }
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
		if (found) _horseFormID = found->GetFormID();
		return found;
	}

	bool HorseManager::IsTownLocation(const RE::BGSLocation* loc)
	{
		for (int depth = 0; loc && depth < 6; ++depth, loc = loc->parentLoc) {
			if (loc->HasKeywordString("LocTypeCity") || loc->HasKeywordString("LocTypeTown")) return true;
		}
		return false;
	}

	void HorseManager::ParkAtNearestStable(RE::Actor* horse)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !horse) return;

		auto* dh = RE::TESDataHandler::GetSingleton();
		auto* list = LookupLight<RE::BGSListForm>(dh, kStableListLocalID);

		RE::TESObjectREFR* best = nullptr;
		float bestDist = (std::numeric_limits<float>::max)();

		if (list && !list->forms.empty()) {
			list->ForEachForm([&](RE::TESForm* form) {
				auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (ref && ref->GetWorldspace() == player->GetWorldspace()) {
					const float d = ref->GetPosition().GetDistance(player->GetPosition());
					if (d < bestDist) { bestDist = d; best = ref; }
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

		// Fallback to hardcoded Skyrim.esm stable markers if list is empty or unloaded
		if (!best && dh) {
			for (auto formId : kFallbackStableMarkers) {
				auto* ref = dh->LookupForm<RE::TESObjectREFR>(formId, "Skyrim.esm");
				if (ref && ref->GetWorldspace() == player->GetWorldspace()) {
					const float d = ref->GetPosition().GetDistance(player->GetPosition());
					if (d < bestDist) { bestDist = d; best = ref; }
				}
			}
		}

		if (best) {
			horse->MoveTo(best);
			SKSE::log::info("HorseManager: parked horse at nearest stable marker {:08X} ({:.0f} units from player).", best->GetFormID(), bestDist);
		} else {
			SKSE::log::warn("HorseManager: No stable marker resolved in this worldspace; horse remains at current location.");
		}
	}

	void HorseManager::Update()
	{
		if (!_started.load(std::memory_order_relaxed)) return;

		const auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
		if (nowMs - _lastTickMs < 1500) return;
		_lastTickMs = nowMs;

		Tick();
	}

	void HorseManager::Tick()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* ui = RE::UI::GetSingleton();
		if (!player || !player->Is3DLoaded() || !player->GetParentCell()) return;
		if (ui && (ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME))) return;

		const auto cls = ClassManager::GetSingleton()->GetPlayerData().currentClass;
		if (cls != ClassType::Knight && cls != ClassType::Merchant) return;

		auto* horse = ResolveHorse();
		if (!horse) return;

		// Ata biniyorsak dokunma
		RE::NiPointer<RE::Actor> mount;
		if (player->GetMount(mount) && mount) {
			_parked = false;
			_lastSpawnOrDismountMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count());
			return;
		}

		// At ile oyuncu arasındaki mesafeyi kontrol et
		const float distToHorse = player->GetPosition().GetDistance(horse->GetPosition());

		// Oyuncunun yakınındaysa (örneğin 2500 birim) veya yeni spawn olduysa / attan yeni indiyse asla ahıra ışınlama
		const auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
		if (nowMs - _lastSpawnOrDismountMs < 60000) { // 60 saniye koruma süresi
			return;
		}

		if (distToHorse < 2500.0f) {
			return;
		}

		const bool inTown = IsTownLocation(player->GetCurrentLocation());
		if (inTown) {
			if (!_parked) {
				ParkAtNearestStable(horse);
				_parked = true;
			}
		} else {
			_parked = false; // Şehirden çıktık; normal takip/bekleme mantığı devam eder
		}
	}
}
