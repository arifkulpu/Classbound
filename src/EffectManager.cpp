#include "EffectManager.h"
#include "ClassRegistry.h"
#include "ClassManager.h"
#include "PrismaUIManager.h"
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <chrono>
#include <windows.h>
#include <mmsystem.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wrl/client.h>
#include <filesystem>
#include <mutex>
#include <vector>
#include <cstring>

namespace {
	using Microsoft::WRL::ComPtr;
	std::mutex g_audioMutex;
	std::vector<BYTE> g_levelUpWave;
	bool g_customAudioReady = false;

	void AppendU16(std::vector<BYTE>& out, WORD value) {
		out.push_back(static_cast<BYTE>(value));
		out.push_back(static_cast<BYTE>(value >> 8));
	}
	void AppendU32(std::vector<BYTE>& out, DWORD value) {
		for (int i = 0; i < 4; ++i) out.push_back(static_cast<BYTE>(value >> (i * 8)));
	}

	bool DecodeMp3ToWave(const std::filesystem::path& path, std::vector<BYTE>& wave) {
		HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
		if (FAILED(hr)) return false;
		bool success = false;
		ComPtr<IMFSourceReader> reader;
		hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
		if (SUCCEEDED(hr)) {
			ComPtr<IMFMediaType> outputType;
			hr = MFCreateMediaType(&outputType);
			if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
			if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, outputType.Get());
			ComPtr<IMFMediaType> actualType;
			if (SUCCEEDED(hr)) hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);
			UINT32 channels = 0, sampleRate = 0, bits = 0;
			if (SUCCEEDED(hr)) hr = actualType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
			if (SUCCEEDED(hr)) hr = actualType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
			if (SUCCEEDED(hr)) hr = actualType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits);
			std::vector<BYTE> pcm;
			while (SUCCEEDED(hr)) {
				DWORD flags = 0;
				ComPtr<IMFSample> sample;
				hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
				if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) break;
				if (!sample) continue;
				ComPtr<IMFMediaBuffer> buffer;
				hr = sample->ConvertToContiguousBuffer(&buffer);
				if (FAILED(hr)) break;
				BYTE* data = nullptr;
				DWORD length = 0;
				hr = buffer->Lock(&data, nullptr, &length);
				if (SUCCEEDED(hr)) {
					pcm.insert(pcm.end(), data, data + length);
					buffer->Unlock();
				}
			}
			if (SUCCEEDED(hr) && !pcm.empty() && channels && sampleRate && bits == 16 && pcm.size() <= UINT32_MAX - 36) {
				const auto blockAlign = static_cast<WORD>(channels * (bits / 8));
				wave.clear(); wave.reserve(44 + pcm.size());
				wave.insert(wave.end(), {'R','I','F','F'}); AppendU32(wave, static_cast<DWORD>(36 + pcm.size()));
				wave.insert(wave.end(), {'W','A','V','E','f','m','t',' '}); AppendU32(wave, 16);
				AppendU16(wave, 1); AppendU16(wave, static_cast<WORD>(channels)); AppendU32(wave, sampleRate);
				AppendU32(wave, sampleRate * blockAlign); AppendU16(wave, blockAlign); AppendU16(wave, static_cast<WORD>(bits));
				wave.insert(wave.end(), {'d','a','t','a'}); AppendU32(wave, static_cast<DWORD>(pcm.size()));
				wave.insert(wave.end(), pcm.begin(), pcm.end());
				success = true;
			}
		}
		MFShutdown();
		return success;
	}
}

namespace Classbound
{
	void EffectManager::InitializeAudio()
	{
		HMODULE module = nullptr;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&EffectManager::InitializeAudio), &module)) {
			SKSE::log::warn("Could not locate ClassBound DLL for level-up audio.");
			return;
		}
		wchar_t modulePath[MAX_PATH]{};
		const auto length = GetModuleFileNameW(module, modulePath, MAX_PATH);
		if (!length || length >= MAX_PATH) return;
		const auto mp3 = std::filesystem::path(modulePath).parent_path() / L"ClassBound" / L"Audio" / L"level-up.mp3";
		std::vector<BYTE> decoded;
		if (DecodeMp3ToWave(mp3, decoded)) {
			std::lock_guard lock(g_audioMutex);
			g_levelUpWave = std::move(decoded);
			g_customAudioReady = true;
			SKSE::log::info("Loaded custom level-up MP3: {} ({} bytes decoded).", mp3.string(), g_levelUpWave.size());
		} else {
			SKSE::log::warn("Could not decode custom level-up MP3 at {}; Skyrim sound will be used as fallback.", mp3.string());
		}
	}

	void EffectManager::PlayLevelUpSound()
	{
		static auto lastPlayed = std::chrono::steady_clock::time_point{};
		const auto now = std::chrono::steady_clock::now();
		if (lastPlayed.time_since_epoch().count() != 0 && now - lastPlayed < std::chrono::milliseconds(900)) return;

		{
			std::lock_guard lock(g_audioMutex);
			if (g_customAudioReady && !g_levelUpWave.empty() &&
				PlaySoundW(reinterpret_cast<LPCWSTR>(g_levelUpWave.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT)) {
				lastPlayed = now;
				SKSE::log::info("Played the custom ClassBound level-up MP3.");
				return;
			}
		}
		// Skyrim.esm UILevelUpSD remains a fallback if the custom MP3 is absent.
		if (auto* audio = RE::BSAudioManager::GetSingleton(); audio && audio->Play(0x00057B44)) {
			lastPlayed = now;
			SKSE::log::info("Played Skyrim UILevelUpSD through BSAudioManager.");
		} else {
			SKSE::log::warn("Could not play Skyrim UILevelUpSD (FormID 00057B44).");
		}
	}

	void EffectManager::PlayLevelUpEffect(RE::Actor* actor)
	{
		if (!actor || actor->IsDead()) return;
		PrismaUIManager::GetSingleton()->ShowLevelUpText(actor);

		// Vanilla Skyrim heal-circle shader gives a short rising, luminous sweep.
		// Fall back to the health enchantment aura if the primary form is unavailable.
		auto* shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x0010CDC9);
		if (!shader) shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x00092DEC);
		if (shader) {
			actor->ApplyEffectShader(shader, 2.75f);
			SKSE::log::info("Applied level-up effect shader {:08X} to actor {:08X}.", shader->GetFormID(), actor->GetFormID());
		} else {
			SKSE::log::warn("Could not find a vanilla effect shader for the ClassBound level-up effect.");
		}
	}

	// ── ComputeBonus ─────────────────────────────────────────────────────────
	// Total bonus = base + perLevel * floor(level / 5)
	float EffectManager::ComputeBonus(const AVBonus& avb, uint32_t level)
	{
		uint32_t tiers = level / 5;  // Level 1-4 = tier 0, 5-9 = tier 1, etc.
		return avb.base + avb.perLevel * static_cast<float>(tiers);
	}

	// ── ModifyAVBase ─────────────────────────────────────────────────────────
	// Skyrim stores actor values as:
	//   GetActorValue()  = base + permanent_modifiers + temporary_modifiers - damage
	//
	// To permanently change stats we modify the BASE value via SetActorValue.
	// We read the current base, add/subtract delta, and write it back.
	void EffectManager::ModifyAVBase(uint32_t avIndex, float delta)
	{
		if (delta == 0.0f) return;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		auto av = static_cast<RE::ActorValue>(avIndex);
		auto* avo = player->AsActorValueOwner();
		if (!avo) return;

		float currentBase = avo->GetBaseActorValue(av);
		avo->SetActorValue(av, currentBase + delta);

		SKSE::log::info("EffectManager::ModifyAVBase: AV[{}] {} -> {}", avIndex,
			currentBase, currentBase + delta);
	}

	// ── ApplyClassEffects ────────────────────────────────────────────────────
	void EffectManager::ApplyClassEffects(ClassType type, uint32_t classLevel)
	{
		auto defOpt = ClassRegistry::GetSingleton()->GetClass(type);
		if (!defOpt) {
			SKSE::log::warn("EffectManager: No class definition for type {}", (uint32_t)type);
			return;
		}

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

		// Clear any previously tracked bonuses (shouldn't happen, but safety first)
		playerData.appliedAVBonuses.clear();

		for (const auto& avb : defOpt->avBonuses) {
			float total = ComputeBonus(avb, classLevel);
			if (total == 0.0f) continue;

			ModifyAVBase(avb.avIndex, total);
			playerData.appliedAVBonuses.emplace_back(avb.avIndex, total);

			SKSE::log::info("EffectManager: Applied +{:.1f} to AV[{}] ({}) for class {} lv{}",
				total, avb.avIndex, avb.label, defOpt->name, classLevel);
		}

		SKSE::log::info("EffectManager: Applied {} bonuses for {} (Level {})",
			playerData.appliedAVBonuses.size(), defOpt->name, classLevel);
	}

	// ── RemoveClassEffects ───────────────────────────────────────────────────
	void EffectManager::RemoveClassEffects(ClassType type)
	{
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

		if (playerData.appliedAVBonuses.empty()) return;

		for (auto& [avIdx, amount] : playerData.appliedAVBonuses) {
			ModifyAVBase(avIdx, -amount);  // Exactly undo what we applied
			SKSE::log::info("EffectManager: Removed {:.1f} from AV[{}]", amount, avIdx);
		}

		playerData.appliedAVBonuses.clear();

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(type);
		SKSE::log::info("EffectManager: Removed all bonuses for {}",
			defOpt ? defOpt->name : "Unknown");
	}

	// ── ApplyLevelUpBonus ────────────────────────────────────────────────────
	// Called when the player's class level increases.
	// Only applies the *increment* since last tier, to avoid double-applying.
	void EffectManager::ApplyLevelUpBonus(ClassType type, uint32_t newLevel)
	{
		// We only grant bonus when crossing a multiple of 5
		if (newLevel % 5 != 0) return;

		auto defOpt = ClassRegistry::GetSingleton()->GetClass(type);
		if (!defOpt) return;

		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

		for (const auto& avb : defOpt->avBonuses) {
			if (avb.perLevel == 0.0f) continue;

			// The increment for going from (newLevel - 1) to newLevel is exactly perLevel
			float increment = avb.perLevel;
			ModifyAVBase(avb.avIndex, increment);

			// Update tracked amount
			bool found = false;
			for (auto& [idx, amt] : playerData.appliedAVBonuses) {
				if (idx == avb.avIndex) {
					amt += increment;
					found = true;
					break;
				}
			}
			if (!found) {
				playerData.appliedAVBonuses.emplace_back(avb.avIndex, increment);
			}

			SKSE::log::info("EffectManager: Level-up bonus +{:.1f} to AV[{}] ({}) at level {}",
				increment, avb.avIndex, avb.label, newLevel);
		}
	}

	// ── ReApplyEffectsOnLoad ─────────────────────────────────────────────────
	void EffectManager::ReApplyEffectsOnLoad(ClassType type, uint32_t classLevel)
	{
		auto& playerData = ClassManager::GetSingleton()->GetPlayerData();

		if (type == ClassType::None) return;

		if (playerData.appliedAVBonuses.empty()) {
			// Native Skyrim saves already contain these AV changes. Rebuild bookkeeping only;
			// applying again here would permanently stack the class bonuses on every load.
			SKSE::log::info("EffectManager: Rebuilding bonus tracking for class {} lv{}",
				(uint32_t)type, classLevel);
			auto defOpt = ClassRegistry::GetSingleton()->GetClass(type);
			if (!defOpt) return;
			for (size_t i = 0; i < defOpt->avBonuses.size(); ++i) {
				const auto& avb = defOpt->avBonuses[i];
				float amount = ComputeBonus(avb, classLevel);
				if (amount != 0.0f) playerData.appliedAVBonuses.emplace_back(avb.avIndex, amount);
			}
		} else {
			SKSE::log::info("EffectManager: Save load OK, {} bonuses already tracked.",
				playerData.appliedAVBonuses.size());
		}
	}

}
