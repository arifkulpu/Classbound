#include <SKSE/SKSE.h>
#include <RE/M/Main.h>
#include "PrismaUIManager.h"
#include "ClassChangeManager.h"
#include "ClassManager.h"
#include "ClassRegistry.h"
#include "XPManager.h"
#include "FollowerManager.h"
#include "SettingsManager.h"
#include "UniversalSkillManager.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cctype>
#include <chrono>
#include <string>
#include "PrismaUI_API.h"
#include "SKSEMenuFramework.h"

namespace Classbound
{
	namespace
	{
		std::atomic_bool g_leftShiftDown{ false };
		std::atomic_bool g_rightShiftDown{ false };

		void NotifyOnGameThread(std::string message)
		{
			PrismaUIManager::Notify(std::move(message));
		}

		void PressEscapeForFramework()
		{
			INPUT events[2]{};
			for (auto& event : events) event.type = INPUT_KEYBOARD;
			events[0].ki.wVk = VK_ESCAPE;
			events[1].ki.wVk = VK_ESCAPE;
			events[1].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(2, events, sizeof(INPUT));
		}

		void OnPrismaConsoleMessage(PrismaView view, PRISMA_UI_API::ConsoleMessageLevel level, const char* message)
		{
			if (level == PRISMA_UI_API::ConsoleMessageLevel::Error) {
				SKSE::log::error("PrismaUI view {}: {}", view, message ? message : "(no message)");
			} else if (level == PRISMA_UI_API::ConsoleMessageLevel::Warning) {
				SKSE::log::warn("PrismaUI view {}: {}", view, message ? message : "(no message)");
			}
		}
	}

	bool PrismaUIManager::Initialize()
	{
		auto* apiV2 = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI2>();
		_api = apiV2 ? static_cast<PRISMA_UI_API::IVPrismaUI1*>(apiV2) :
			PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI1>();
		if (!_api) {
			SKSE::log::warn("PrismaUI was not found. The existing ImGui menu remains available.");
			return false;
		}

		_view = _api->CreateView("ClassBound/index.html", [](PrismaView view) {
			SKSE::log::info("ClassBound PrismaUI DOM is ready (view {}).", view);
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([]() { PrismaUIManager::GetSingleton()->SendState(); });
			}
		});

		if (_view == 0 || !_api->IsValid(_view)) {
			SKSE::log::error("PrismaUI could not create the ClassBound view.");
			_api = nullptr;
			return false;
		}
		_api->RegisterJSListener(_view, "cbClassSelected", OnClassSelected);
		_api->RegisterJSListener(_view, "cbClassConfirmed", OnClassConfirmed);
		_api->RegisterJSListener(_view, "cbCloseClassBound", OnClose);
		_api->RegisterJSListener(_view, "cbFollowerAssign", OnFollowerAssign);
		_api->RegisterJSListener(_view, "cbUniversalSkillAction", OnUniversalSkillAction);
		_api->RegisterJSListener(_view, "cbTestAction", OnTestAction);
		_api->RegisterJSListener(_view, "cbSettingsChanged", OnSettingsChanged);
		if (apiV2) apiV2->RegisterConsoleCallback(_view, OnPrismaConsoleMessage);
		_api->Hide(_view);
		_levelUpView = _api->CreateView("ClassBound/levelup.html", [](PrismaView view) {
			PrismaUIManager::OnLevelUpOverlayReady(view);
		});
		if (_levelUpView != 0 && _api->IsValid(_levelUpView)) {
			_api->SetOrder(_levelUpView, 10000);
			_api->Hide(_levelUpView);
			if (apiV2) apiV2->RegisterConsoleCallback(_levelUpView, OnPrismaConsoleMessage);
		} else {
			_levelUpView = 0;
			SKSE::log::warn("Could not create ClassBound level-up text overlay.");
		}
		_notificationView = _api->CreateView("ClassBound/notifications.html", [](PrismaView view) {
			PrismaUIManager::OnNotificationOverlayReady(view);
		});
		if (_notificationView != 0 && _api->IsValid(_notificationView)) {
			_api->SetOrder(_notificationView, 9999);
			_api->Hide(_notificationView);
			if (apiV2) apiV2->RegisterConsoleCallback(_notificationView, OnPrismaConsoleMessage);
		} else {
			_notificationView = 0;
			SKSE::log::warn("Could not create ClassBound notification overlay.");
		}
		InitializeHotkey();
		SKSE::log::info("PrismaUI ClassBound view registered.");
		return true;
	}

	void PrismaUIManager::OnLevelUpOverlayReady(uint64_t view)
	{
		if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([view]() {
			auto* self = PrismaUIManager::GetSingleton();
			if (!self->_api || self->_levelUpView != view || !self->_api->IsValid(view)) return;
			self->_levelUpViewReady = true;
			self->_api->Hide(view);
			SKSE::log::info("ClassBound level-up text overlay is ready.");
		});
	}

	void PrismaUIManager::Notify(std::string message, std::string category)
	{
		if (message.empty()) return;
		if (auto* tasks = SKSE::GetTaskInterface()) {
			tasks->AddTask([message = std::move(message), category = std::move(category)]() mutable {
				GetSingleton()->DispatchNotification(std::move(message), std::move(category));
			});
		} else {
			SKSE::log::warn("ClassBound notification skipped because the SKSE task interface is unavailable: {}", message);
		}
	}

	void PrismaUIManager::OnNotificationOverlayReady(uint64_t view)
	{
		if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([view]() {
			auto* self = PrismaUIManager::GetSingleton();
			if (!self->_api || self->_notificationView != view || !self->_api->IsValid(view)) return;
			self->_notificationViewReady = true;
			while (!self->_pendingNotifications.empty()) {
				auto notification = std::move(self->_pendingNotifications.front());
				self->_pendingNotifications.pop_front();
				self->DispatchNotification(std::move(notification.first), std::move(notification.second));
			}
			SKSE::log::info("ClassBound PrismaUI notification overlay is ready.");
		});
	}

	void PrismaUIManager::DispatchNotification(std::string message, std::string category)
	{
		if (!_api || _notificationView == 0 || !_api->IsValid(_notificationView) || !_notificationViewReady) {
			if (_pendingNotifications.size() >= 32) _pendingNotifications.pop_front();
			_pendingNotifications.emplace_back(std::move(message), std::move(category));
			return;
		}
		const auto script = fmt::format("window.classboundNotify && window.classboundNotify({}, {})",
			nlohmann::json(message).dump(), nlohmann::json(category).dump());
		_api->Show(_notificationView);  // Transparent, non-interactive view; never takes focus.
		_api->Invoke(_notificationView, script.c_str());
	}

	void PrismaUIManager::ShowLevelUpText(RE::Actor* actor)
	{
		if (!actor || actor->IsDead()) return;
		// The detailed level-up toast is sent through the PrismaUI notification stream.
		if (!_api || !_levelUpViewReady || _levelUpView == 0 || !_api->IsValid(_levelUpView)) return;
		const auto token = ++_levelUpToken;
		_trackedLevelUpActor.store(actor->GetFormID(), std::memory_order_relaxed);
		const auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
		_trackLevelUpUntilMs.store(nowMs + 2600, std::memory_order_relaxed);
		_nextLevelUpUpdateMs.store(nowMs, std::memory_order_relaxed);
		_api->Show(_levelUpView);  // Deliberately never focused: no mouse or keyboard capture.
		UpdateLevelUpOverlay(token, actor->GetFormID());
	}

	void PrismaUIManager::UpdateLevelUpOverlay(uint64_t token, uint32_t actorFormID)
	{
		auto* self = GetSingleton();
		if (!self->_api || token != self->_levelUpToken || !self->_levelUpViewReady ||
			self->_levelUpView == 0 || !self->_api->IsValid(self->_levelUpView)) return;

		auto* actor = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
		auto* camera = RE::Main::WorldRootCamera();
		bool cuePlaced = false;
		if (actor && !actor->IsDead() && actor->Is3DLoaded() && camera) {
			// Anchor humanoid actors to the animated head node so follower height,
			// race, and animation do not leave the banner floating at a stale height.
			auto point = actor->GetPosition();
			if (auto* head = actor->GetNodeByName(RE::BSFixedString("NPC Head [Head]"))) {
				point = head->world.translate;
				point.z += 8.0f;
			} else {
				point.z += actor->GetHeight() + 18.0f;
			}
			float x = 0.0f, y = 0.0f, depth = 0.0f;
			const auto& cameraData = camera->GetRuntimeData();
			const auto& cameraData2 = camera->GetRuntimeData2();
			const bool projected = RE::NiCamera::WorldPtToScreenPt3(
				cameraData.worldToCam, cameraData2.port, point, x, y, depth, 0.0001f);
			if (projected && std::isfinite(x) && std::isfinite(y) &&
				x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f) {
				// Skyrim's normalized projection uses a bottom-origin Y axis, while
				// the PrismaUI view uses a top-origin CSS axis. Flip it, then lift the
				// banner slightly so it sits above the character rather than at the waist.
				const float textY = std::clamp(1.0f - y - 0.035f, 0.0f, 1.0f);
				const auto script = fmt::format("window.classboundShowLevelUp && window.classboundShowLevelUp({:.6f}, {:.6f}, {}, {})",
					x, textY, nlohmann::json("LEVEL UP!").dump(), nlohmann::json(std::to_string(token)).dump());
				self->_api->Invoke(self->_levelUpView, script.c_str());
				cuePlaced = true;
			}
		}
		if (!cuePlaced) {
			self->_api->Invoke(self->_levelUpView,
				"window.classboundHideLevelUp && window.classboundHideLevelUp()");
		}
	}

	bool PrismaUIManager::InitializeHotkey()
	{
		if (_hotkeyListener) return true;
		if (!SKSEMenuFramework::IsInstalled()) {
			SKSE::log::warn("SKSEMenuFramework is not installed; Shift+V PrismaUI shortcut is unavailable.");
			return false;
		}
		_hotkeyListener = SKSEMenuFramework::AddInputEvent(OnFrameworkInput);
		if (!_hotkeyListener) {
			SKSE::log::error("Could not register the Shift+V PrismaUI shortcut.");
			return false;
		}
		if (!_levelUpHudListener) {
			_levelUpHudListener = SKSEMenuFramework::AddHudElement(OnLevelUpHudFrame);
			if (_levelUpHudListener) SKSE::log::info("Registered lightweight level-up anchor tracker.");
			else SKSE::log::warn("Could not register level-up anchor tracker.");
		}
		SKSE::log::info("Registered Shift+V shortcut for PrismaUI ClassBound.");
		return true;
	}

	void __stdcall PrismaUIManager::OnLevelUpHudFrame()
	{
		auto* self = GetSingleton();
		const auto actorFormID = self->_trackedLevelUpActor.load(std::memory_order_relaxed);
		if (actorFormID == 0) return;

		const auto nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
		if (nowMs >= self->_trackLevelUpUntilMs.load(std::memory_order_relaxed)) {
			self->_trackedLevelUpActor.store(0, std::memory_order_relaxed);
			if (self->_api && self->_levelUpView && self->_api->IsValid(self->_levelUpView)) {
				self->_api->Invoke(self->_levelUpView, "window.classboundHideLevelUp && window.classboundHideLevelUp()");
				self->_api->Hide(self->_levelUpView);
			}
			return;
		}

		if (nowMs < self->_nextLevelUpUpdateMs.load(std::memory_order_relaxed)) return;
		// Reproject at 15 Hz only while the temporary banner is active.
		self->_nextLevelUpUpdateMs.store(nowMs + 66, std::memory_order_relaxed);
		UpdateLevelUpOverlay(self->_levelUpToken, actorFormID);
	}

	bool __stdcall PrismaUIManager::OnFrameworkInput(RE::InputEvent* event)
	{
		bool openRequested = false;
		bool closeRequested = false;
		for (auto* current = event; current; current = current->next) {
			if (current->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;
			auto* button = current->AsButtonEvent();
			if (!button) continue;

			const auto key = button->GetIDCode();
			if (key == RE::BSKeyboardDevice::Keys::kEscape && button->IsDown() &&
				GetSingleton()->_viewOpen.load(std::memory_order_relaxed)) {
				closeRequested = true;
				continue;
			}
			if (key == RE::BSKeyboardDevice::Keys::kLeftShift) {
				if (button->IsUp()) g_leftShiftDown.store(false, std::memory_order_relaxed);
				else if (button->IsPressed()) g_leftShiftDown.store(true, std::memory_order_relaxed);
				continue;
			}
			if (key == RE::BSKeyboardDevice::Keys::kRightShift) {
				if (button->IsUp()) g_rightShiftDown.store(false, std::memory_order_relaxed);
				else if (button->IsPressed()) g_rightShiftDown.store(true, std::memory_order_relaxed);
				continue;
			}
			if (key == RE::BSKeyboardDevice::Keys::kV && button->IsDown() &&
				(g_leftShiftDown.load(std::memory_order_relaxed) || g_rightShiftDown.load(std::memory_order_relaxed))) {
				openRequested = true;
			}
		}

		if (closeRequested) {
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([]() { PrismaUIManager::GetSingleton()->CloseView(); });
				return true;
			}
			return false;
		}
		if (!openRequested) return false;
		if (auto* tasks = SKSE::GetTaskInterface()) {
			tasks->AddTask([]() { PrismaUIManager::GetSingleton()->Open(); });
			return true;
		}
		return false;
	}

	void PrismaUIManager::Open()
	{
		if (!IsAvailable()) return;

		// This is invoked from SKSEMenuFramework's render callback. Defer the Escape
		// until that callback has returned so the framework can process it normally.
		if (SKSEMenuFramework::IsInstalled()) {
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([]() { GetSingleton()->BeginFrameworkClose(); });
				return;
			}
			SKSE::log::error("Could not queue PrismaUI launch after SKSEMenuFramework close.");
			return;
		}

		ShowView();
	}

	void PrismaUIManager::BeginFrameworkClose()
	{
		// Close the framework's actual main window through its API. Synthetic
		// keyboard input is unreliable while ImGui owns the input focus, so retain
		// Escape only as a fallback for older framework builds.
		if (auto* mainWindow = SKSEMenuFramework::GetMainWindow()) {
			const bool frameworkWasOpen = mainWindow->IsOpen.exchange(false, std::memory_order_acq_rel);
			SKSE::log::info("Requested SKSEMenuFramework main window close for PrismaUI launch.");
			// Closing ImGui's visibility flag alone can leave Skyrim's menu input mode active.
			// Send Escape only when the framework window was actually open.
			if (frameworkWasOpen) PressEscapeForFramework();
		} else {
			SKSE::log::warn("SKSEMenuFramework GetMainWindow API is unavailable; falling back to Escape.");
			PressEscapeForFramework();
		}
		WaitForFrameworkClose(120);
	}

	void PrismaUIManager::ShowView()
	{
		if (!IsAvailable()) return;
		RefreshTrainerAccess();
		_api->Show(_view);
		_api->Focus(_view, true, false);
		_viewOpen.store(true, std::memory_order_relaxed);
		SendState();
	}

	void PrismaUIManager::CloseView()
	{
		if (!IsAvailable()) return;
		_viewOpen.store(false, std::memory_order_relaxed);
		_api->Unfocus(_view);
		_api->Hide(_view);
		SKSE::log::info("Closed ClassBound PrismaUI with Escape or close action.");
	}

	void PrismaUIManager::WaitForFrameworkClose(uint32_t attemptsRemaining)
	{
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) return;
		tasks->AddTask([attemptsRemaining]() {
			auto* self = GetSingleton();
			if (SKSEMenuFramework::IsInstalled() && SKSEMenuFramework::IsAnyBlockingWindowOpened()) {
				if (attemptsRemaining == 0) {
					SKSE::log::error("SKSEMenuFramework did not close; PrismaUI was not opened to prevent mouse input from being trapped.");
					Notify("Close the SKSE menu, then open the ClassBound Prisma Codex again.", "warning");
					return;
				}
				// Older framework builds may not expose GetMainWindow, so retry the
				// fallback Escape while waiting for the close state to settle.
				if (attemptsRemaining % 20 == 0 && !SKSEMenuFramework::GetMainWindow()) PressEscapeForFramework();
				WaitForFrameworkClose(attemptsRemaining - 1);
				return;
			}
			SKSE::log::info("SKSEMenuFramework closed; opening PrismaUI ClassBound view.");
			self->ShowView();
		});
	}

	void PrismaUIManager::RefreshTrainerAccess()
	{
		auto checkActor = [](RE::Actor* actor) {
			if (!actor) return;
			const auto refID = actor->GetFormID();
			const auto baseID = actor->GetActorBase() ? actor->GetActorBase()->GetFormID() : 0;
			if (ClassChangeManager::IsTrainerNPC(refID) || ClassChangeManager::IsTrainerNPC(baseID)) {
				ClassChangeManager::GetSingleton()->GrantTrainerAccess();
			}
		};

		if (auto* topics = RE::MenuTopicManager::GetSingleton()) {
			if (auto speaker = topics->speaker.get(); speaker) checkActor(speaker->As<RE::Actor>());
			if (auto lastSpeaker = topics->lastSpeaker.get(); lastSpeaker) checkActor(lastSpeaker->As<RE::Actor>());
		}
		if (auto* crosshair = RE::CrosshairPickData::GetSingleton(); crosshair && crosshair->target) {
			if (crosshair->target.get()->Is(RE::FormType::ActorCharacter)) {
				checkActor(crosshair->target.get()->As<RE::Actor>());
			}
		}
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			if (auto* cell = player->GetParentCell()) {
				for (auto& ref : cell->GetRuntimeData().references) {
					if (!ref || !ref->Is(RE::FormType::ActorCharacter)) continue;
					auto* actor = ref->As<RE::Actor>();
					if (actor && actor != player && !actor->IsDead() &&
						player->GetPosition().GetDistance(actor->GetPosition()) < 400.0f) {
						checkActor(actor);
					}
				}
			}
		}
	}

	void PrismaUIManager::SendState()
	{
		if (!IsAvailable()) return;

		auto& player = ClassManager::GetSingleton()->GetPlayerData();
		nlohmann::json state;
		state["currentClass"] = static_cast<uint32_t>(player.currentClass);
		state["classLevel"] = player.classLevel;
		state["classXP"] = player.classXP;
		state["requiredXP"] = XPManager::GetSingleton()->GetXPForNextLevel(player.classLevel);
		state["selectedClass"] = _selectedClass;
		state["hasTrainer"] = ClassChangeManager::GetSingleton()->HasTrainerAccess();
		state["changeCost"] = ClassChangeManager::GetChangeCost(player.classLevel);
		auto makeSkills = [](const UniversalSkillProgress& progress) {
			nlohmann::json result;
			result["earned"] = progress.earnedPoints + progress.bonusPoints;
			result["spent"] = UniversalSkillManager::Spent(progress);
			result["available"] = UniversalSkillManager::Available(progress);
			result["attributes"] = nlohmann::json::array();
			const auto& definitions = UniversalSkillManager::Definitions();
			for (size_t i = 0; i < definitions.size(); ++i) {
				const auto& def = definitions[i];
				result["attributes"].push_back({{"id", i}, {"key", def.id}, {"name", def.name}, {"category", def.category},
					{"icon", def.icon}, {"description", def.description}, {"effect", def.effect}, {"rank", progress.ranks[i]}, {"maxRank", kMaxUniversalRank}});
			}
			return result;
		};
		state["universalSkills"] = makeSkills(player.universalSkills);
		state["classes"] = nlohmann::json::array();

		for (const auto& [type, def] : ClassRegistry::GetSingleton()->GetAllClasses()) {
			nlohmann::json entry;
			entry["id"] = static_cast<uint32_t>(type);
			entry["name"] = def.name;
			entry["category"] = static_cast<uint32_t>(def.category);
			entry["icon"] = def.icon;
			entry["description"] = def.description;
			entry["playstyle"] = def.playstyle;
			entry["bonuses"] = def.startingBonuses;
			entry["avBonuses"] = nlohmann::json::array();
			for (size_t i = 0; i < def.avBonuses.size(); ++i) {
				const auto& bonus = def.avBonuses[i];
				entry["avBonuses"].push_back({ {"label", bonus.label}, {"base", bonus.base}, {"perLevel", bonus.perLevel} });
			}
			entry["stats"] = {{"combat", def.stats.combat}, {"defense", def.stats.defense}, {"magic", def.stats.magic},
				{"stealth", def.stats.stealth}, {"trading", def.stats.trading}};
			state["classes"].push_back(std::move(entry));
		}

		state["stats"] = {{"enemiesKilled", player.stats.enemiesKilled}, {"meleeKills", player.stats.meleeKills},
			{"archeryKills", player.stats.archeryKills}, {"magicKills", player.stats.magicKills},
			{"sneakKills", player.stats.sneakKills}, {"bossesDefeated", player.stats.bossesDefeated},
			{"spellsCast", player.stats.spellsCast}, {"itemsSold", player.stats.itemsSold},
			{"itemsCrafted", player.stats.itemsCrafted}, {"locksPicked", player.stats.locksPicked},
			{"dungeonsCleared", player.stats.dungeonsCleared}};
		auto* settings = SettingsManager::GetSingleton();
		state["settings"] = {{"xpMultiplier", settings->xpMultiplier}, {"notifyClassXP", settings->notifyClassXP},
			{"notifyClassLevel", settings->notifyClassLevel}, {"showDamageNumbers", settings->showDamageNumbers},
			{"confirmClassChange", settings->confirmClassChange}, {"automaticClassXP", settings->automaticClassXP},
			{"limitClassLevelToPlayerLevel", settings->limitClassLevelToPlayerLevel}};

		auto* followerManager = FollowerManager::GetSingleton();
		followerManager->ScanActiveFollowers(true);
		state["followers"] = nlohmann::json::array();
		for (const auto& follower : followerManager->GetAllFollowers()) {
			auto* actor = followerManager->FindFollowerActor(follower.formID);
			if (actor && FollowerManager::IsAnimalActor(actor)) continue;
			std::string raceKey = "nord";
			std::string sexKey = "male";
			if (actor) {
				if (auto* race = actor->GetRace(); race && race->GetName()) {
					std::string raceName = race->GetName();
					std::transform(raceName.begin(), raceName.end(), raceName.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (raceName.find("imperial") != std::string::npos) raceKey = "imperial";
					else if (raceName.find("redguard") != std::string::npos) raceKey = "redguard";
					else if (raceName.find("breton") != std::string::npos) raceKey = "breton";
					else if (raceName.find("high elf") != std::string::npos || raceName.find("altmer") != std::string::npos) raceKey = "high_elf";
					else if (raceName.find("wood elf") != std::string::npos || raceName.find("bosmer") != std::string::npos) raceKey = "wood_elf";
					else if (raceName.find("dark elf") != std::string::npos || raceName.find("dunmer") != std::string::npos) raceKey = "dark_elf";
					else if (raceName.find("orc") != std::string::npos || raceName.find("orsimer") != std::string::npos) raceKey = "orc";
					else if (raceName.find("argonian") != std::string::npos) raceKey = "argonian";
					else if (raceName.find("khajiit") != std::string::npos) raceKey = "khajiit";
				}
				if (auto* base = actor->GetActorBase(); base && base->GetSex() == RE::SEX::kFemale) sexKey = "female";
			}
			auto followerClass = ClassRegistry::GetSingleton()->GetClass(follower.currentClass);
			auto followerEntry = nlohmann::json{{"formID", follower.formID}, {"name", follower.name},
				{"race", raceKey}, {"sex", sexKey},
				{"classID", static_cast<uint32_t>(follower.currentClass)}, {"className", followerClass ? followerClass->name : "Unassigned"},
				{"icon", followerClass ? followerClass->icon : "✦"}, {"level", follower.classLevel}, {"xp", follower.classXP},
				{"requiredXP", XPManager::GetSingleton()->GetXPForNextLevel(follower.classLevel)}, {"active", follower.isActiveFollower}};
			followerEntry["universalSkills"] = makeSkills(follower.universalSkills);
			if (follower.isActiveFollower) state["followers"].push_back(std::move(followerEntry));
		}

		const auto payload = state.dump();
		const auto script = "window.ClassBound && window.ClassBound.update(" + payload + ");";
		_api->Invoke(_view, script.c_str());
	}

	void PrismaUIManager::OnClassSelected(const char* data)
	{
		if (!data) return;
		uint32_t classID = 0;
		try { classID = static_cast<uint32_t>(std::stoul(data)); }
		catch (...) { return; }

		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) return;
		tasks->AddTask([classID]() {
			auto* self = GetSingleton();
			if (!ClassRegistry::GetSingleton()->GetClass(static_cast<ClassType>(classID))) return;
			self->_selectedClass = classID;
			self->SendState();
		});
	}

	void PrismaUIManager::OnClassConfirmed(const char*)
	{
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) return;
		tasks->AddTask([]() {
			auto* self = GetSingleton();
			auto selected = static_cast<ClassType>(self->_selectedClass);
			if (!ClassRegistry::GetSingleton()->GetClass(selected)) return;

			auto& data = ClassManager::GetSingleton()->GetPlayerData();
			bool changed = false;
			if (data.currentClass == ClassType::None) {
				ClassManager::GetSingleton()->SetPlayerClass(selected, true);
				changed = true;
			} else if (selected != data.currentClass && ClassChangeManager::GetSingleton()->HasTrainerAccess()) {
				changed = ClassChangeManager::GetSingleton()->ExecuteClassChange(selected);
			} else if (selected == data.currentClass) {
				Notify("That class is already active.", "warning");
			} else {
				Notify("Speak with a class trainer before changing class.", "warning");
			}

			if (changed) self->_selectedClass = 0;
			self->SendState();
		});
	}

	void PrismaUIManager::OnClose(const char*)
	{
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) return;
		tasks->AddTask([]() {
			auto* self = GetSingleton();
			self->CloseView();
		});
	}

	void PrismaUIManager::OnFollowerAssign(const char* data)
	{
		if (!data) {
			NotifyOnGameThread("Companion class selection failed: no selection data.");
			return;
		}
		try {
			auto parsed = nlohmann::json::parse(data);
			const auto formID = parsed.at("formID").get<uint32_t>();
			const auto classID = parsed.at("classID").get<uint32_t>();
			if (formID == 0 || (classID != 0 && !ClassRegistry::GetSingleton()->GetClass(static_cast<ClassType>(classID)))) {
				NotifyOnGameThread("Companion class selection is invalid.");
				return;
			}
			if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([formID, classID]() {
				if (!FollowerManager::GetSingleton()->AssignClass(formID, static_cast<ClassType>(classID))) {
					SKSE::log::warn("PrismaUI companion class assignment failed for {:08X} -> {}", formID, classID);
					Notify("Companion class was not changed. Check trainer access and gold.", "warning");
				}
				GetSingleton()->SendState();
			});
		} catch (const std::exception& e) {
			SKSE::log::error("PrismaUI companion class request could not be read: {}", e.what());
			NotifyOnGameThread("Companion class selection could not be read.");
		}
	}

	void PrismaUIManager::OnUniversalSkillAction(const char* data)
	{
		if (!data) return;
		try {
			auto parsed = nlohmann::json::parse(data);
			const auto target = parsed.value("target", std::string("player"));
			const auto action = parsed.value("action", std::string("upgrade"));
			const auto attributeID = parsed.value("attributeID", uint32_t{ kUniversalAttributeCount });
			const auto formID = parsed.value("formID", uint32_t{ 0 });
			if ((target != "player" && target != "follower") || (action != "upgrade" && action != "reset") ||
				(attributeID >= kUniversalAttributeCount && action == "upgrade")) return;
			if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([target, action, attributeID, formID]() {
				auto* skills = UniversalSkillManager::GetSingleton();
				bool success = false;
				if (target == "player") success = action == "reset" ? skills->ResetPlayer() : skills->UpgradePlayer(attributeID);
				else if (formID != 0) success = action == "reset" ? skills->ResetFollower(formID) : skills->UpgradeFollower(formID, attributeID);
				if (success) Notify(action == "reset" ? "Skill points refunded." : "Universal skill improved.", "success");
				else Notify("Unable to change skill: check available points and rank limit.", "warning");
				PrismaUIManager::GetSingleton()->SendState();
			});
		} catch (const std::exception& e) {
			SKSE::log::warn("PrismaUI skill action could not be read: {}", e.what());
		}
	}

	void PrismaUIManager::OnTestAction(const char* data)
	{
		if (!data) return;
		try {
			auto parsed = nlohmann::json::parse(data);
			const auto action = parsed.value("action", std::string{});
			const auto targetFormID = parsed.value("formID", uint32_t{ 0 });
			const auto amount = parsed.value("amount", uint32_t{ 0 });
			const bool validXP = action == "xp" && (amount == 100 || amount == 500 || amount == 1000);
			const bool validPoints = action == "points" && (amount == 1 || amount == 5 || amount == 10);
			const bool validLevels = (action == "levels" || action == "follower-levels") && (amount == 1 || amount == 5);
			if (action == "follower-levels" && targetFormID == 0) return;
			if (!validXP && !validPoints && !validLevels) return;
			if (targetFormID != 0) {
				auto* follower = FollowerManager::GetSingleton()->GetFollowerData(targetFormID);
				if (!follower || !follower->isActiveFollower) return;
			}

			// PrismaUI currently owns a pausing focus. Release it on the CEF callback
			// thread before queuing game work; otherwise the SKSE task can wait for the
			// very menu closure that is itself waiting for the queued task.
			if (action == "levels" || action == "follower-levels") {
				auto* self = PrismaUIManager::GetSingleton();
				if (self->_api && self->_view != 0 && self->_api->IsValid(self->_view)) {
					self->_api->Unfocus(self->_view);
					self->_api->Hide(self->_view);
					self->_viewOpen.store(false, std::memory_order_relaxed);
				}
			}

			if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([action, targetFormID, amount]() {
				SKSE::log::info("Progression test action '{}' started (target {:08X}, amount {}).", action, targetFormID, amount);

				uint32_t granted = 0;
				if (action == "xp") granted = XPManager::GetSingleton()->GrantTestXP(targetFormID, amount);
				else if (action == "levels" || action == "follower-levels") granted = XPManager::GetSingleton()->GrantTestLevels(targetFormID, amount);
				else granted = UniversalSkillManager::GetSingleton()->GrantTestPoints(targetFormID, amount);

				if (granted == 0) {
					Notify("Test action not applied: target may be at the progression cap.", "warning");
				} else if (action == "points") {
					char message[96];
					snprintf(message, sizeof(message), "Test skill points: +%u", granted);
					Notify(message, "success");
				}
				SKSE::log::info("Progression test action '{}' finished (granted {}).", action, granted);
				// Level-up closes this view. Avoid serializing state to its hidden CEF page.
				if (action != "levels" && action != "follower-levels") PrismaUIManager::GetSingleton()->SendState();
			});
		} catch (const std::exception& e) {
			SKSE::log::warn("PrismaUI test action could not be read: {}", e.what());
		}
	}

	void PrismaUIManager::OnSettingsChanged(const char* data)
	{
		if (!data) return;
		try {
			auto parsed = nlohmann::json::parse(data);
			if (auto* tasks = SKSE::GetTaskInterface()) tasks->AddTask([parsed]() {
				auto* s = SettingsManager::GetSingleton();
				s->xpMultiplier = std::clamp(parsed.value("xpMultiplier", s->xpMultiplier), 0.1f, 5.0f);
				s->notifyClassXP = parsed.value("notifyClassXP", s->notifyClassXP);
				s->notifyClassLevel = parsed.value("notifyClassLevel", s->notifyClassLevel);
				s->showDamageNumbers = parsed.value("showDamageNumbers", s->showDamageNumbers);
				s->confirmClassChange = parsed.value("confirmClassChange", s->confirmClassChange);
				s->automaticClassXP = parsed.value("automaticClassXP", s->automaticClassXP);
				s->limitClassLevelToPlayerLevel = parsed.value("limitClassLevelToPlayerLevel", s->limitClassLevelToPlayerLevel);
				s->SaveSettings();
				GetSingleton()->SendState();
			});
		} catch (...) {}
	}
}
