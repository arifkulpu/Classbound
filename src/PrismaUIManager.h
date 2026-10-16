#pragma once

#include <cstdint>
#include <atomic>

namespace SKSEMenuFramework::Model
{
	class InputEvent;
	class HudElement;
}

namespace RE
{
	class InputEvent;
	class Actor;
}

namespace PRISMA_UI_API
{
	class IVPrismaUI1;
}

namespace Classbound
{
	class PrismaUIManager
	{
	public:
		static PrismaUIManager* GetSingleton()
		{
			static PrismaUIManager singleton;
			return &singleton;
		}

		bool Initialize();
		bool InitializeHotkey();
		void Open();
		void ShowLevelUpText(RE::Actor* actor);
		bool IsAvailable() const noexcept { return _api != nullptr && _view != 0; }

	private:
		PrismaUIManager() = default;
		static void OnClassSelected(const char* data);
		static void OnClassConfirmed(const char* data);
		static void OnClose(const char* data);
		static void OnFollowerAssign(const char* data);
		static void OnUniversalSkillAction(const char* data);
		static void OnTestAction(const char* data);
		static void OnLevelUpOverlayReady(uint64_t view);
		static void __stdcall OnLevelUpHudFrame();
		static void UpdateLevelUpOverlay(uint64_t token, uint32_t actorFormID);
		static void OnSettingsChanged(const char* data);
		static bool __stdcall OnFrameworkInput(RE::InputEvent* event);
		void RefreshTrainerAccess();
		void SendState();
		void ShowView();
		void CloseView();
		void BeginFrameworkClose();
		static void WaitForFrameworkClose(uint32_t attemptsRemaining);

		PRISMA_UI_API::IVPrismaUI1* _api{ nullptr };
		uint64_t _view{ 0 };
		uint64_t _levelUpView{ 0 };
		uint64_t _levelUpToken{ 0 };
		bool _levelUpViewReady{ false };
		std::atomic_bool _viewOpen{ false };
		uint32_t _selectedClass{ 0 };
		SKSEMenuFramework::Model::InputEvent* _hotkeyListener{ nullptr };
		SKSEMenuFramework::Model::HudElement* _levelUpHudListener{ nullptr };
		std::atomic<uint32_t> _trackedLevelUpActor{ 0 };
		std::atomic<uint64_t> _trackLevelUpUntilMs{ 0 };
		std::atomic<uint64_t> _nextLevelUpUpdateMs{ 0 };
	};
}
