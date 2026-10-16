#include <spdlog/sinks/basic_file_sink.h>
#include <SKSE/SKSE.h>
#include "ClassRegistry.h"
#include "SaveManager.h"
#include "EventManager.h"
#include "SkillManager.h"
#include "SettingsManager.h"
#include "UIManager.h"
#include "GlobalCharacterDB.h"
#include "SKSEMenuFramework.h"
#include "KingCoreAdapter.h"
#include "PrismaUIManager.h"
#include "EffectManager.h"
#include "KingCore/KingCoreAPI.h"

using namespace std::literals;

namespace {
	void InitializeLog() {
		auto path = SKSE::log::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v"s);
	}
}

extern "C" __declspec(dllexport) SKSE::PluginVersionData SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version{1, 0, 0, 0});
	v.PluginName("Classbound");
	v.AuthorName("ClassboundTeam");
	v.UsesAddressLibrary(true);
	v.UsesNoStructs(true);
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });
	return v;
}();

void RenderClassBoundMenu() {
	Classbound::UIManager::GetSingleton()->Render();
}

void RenderPrismaClassLauncher() {
	ImGuiMCP::TextColored({ 0.82f, 0.66f, 0.24f, 1.0f }, "CLASSBOUND CODEX");
	ImGuiMCP::Text("Open the modern class catalog and character progression screen.");
	ImGuiMCP::Spacing();
	if (ImGuiMCP::Button("OPEN PRISMA CLASS CODEX", ImGuiMCP::ImVec2(300.0f, 42.0f))) {
		Classbound::PrismaUIManager::GetSingleton()->Open();
	}
}

void DispatchKingCoreAPI(const char* target)
{
    auto* msgIface = SKSE::GetMessagingInterface();
    if (!msgIface) return;

    // Build a fresh stack-local exchange each call — no 'static', no stale pointer.
    KingCore::KingCoreInterfaceExchange exchange{
        .apiVersion = KingCore::API_VERSION,
        .classAPI   = Classbound::KingCoreAdapter::GetSingleton()
    };
    msgIface->Dispatch(
        KingCore::MESSAGE_TYPE_PROVIDE_API,
        &exchange,
        sizeof(exchange),
        target
    );
}

void HandleSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
{
    if (!a_msg) return;

    if (a_msg->type == SKSE::MessagingInterface::kPostLoad) {
		const bool hasPrismaUI = Classbound::PrismaUIManager::GetSingleton()->Initialize();
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("ClassBound");
			if (hasPrismaUI) {
				SKSEMenuFramework::AddSectionItem("Class Codex (Prisma UI)", RenderPrismaClassLauncher);
			}
			SKSEMenuFramework::AddSectionItem("Legacy Settings (ImGui)", RenderClassBoundMenu);
            SKSE::log::info("Registered with SKSEMenuFramework.");
        } else {
            SKSE::log::warn("SKSEMenuFramework not found! Menu will not be available.");
        }

        // Initial Handshake with King Core at kPostLoad
        DispatchKingCoreAPI("KingCore");
        DispatchKingCoreAPI(nullptr); // Broadcast
        SKSE::log::info("Dispatched King Core API provider registration (kPostLoad).");
    }
    else if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
        // Re-dispatch handshake at kDataLoaded to ensure King Core receives it even if loaded after
        DispatchKingCoreAPI("KingCore");
        DispatchKingCoreAPI(nullptr);
        SKSE::log::info("Dispatched King Core API provider registration (kDataLoaded).");
    }
    else if (a_msg->type == KingCore::MESSAGE_TYPE_REQUEST_API) {
        // King Core or another consumer explicitly requested the API
        DispatchKingCoreAPI(a_msg->sender);
        SKSE::log::info("Responded to King Core API request from {}", (a_msg->sender ? a_msg->sender : "unknown"));
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
	InitializeLog();

	auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	SKSE::log::info("{} {} is loading...", plugin->GetName(), plugin->GetVersion());

	SKSE::Init(skse);
	Classbound::EffectManager::InitializeAudio();
	
	Classbound::GlobalCharacterDB::GetSingleton()->LoadDB();
	Classbound::SettingsManager::GetSingleton()->LoadSettings();
	
	Classbound::ClassRegistry::GetSingleton()->Initialize();
	Classbound::SaveManager::Register();
	Classbound::EventManager::GetSingleton()->Register();
	
	auto messaging = SKSE::GetMessagingInterface();
	if (messaging) {
		// SKSE 2.0+ mesaj sistemi gönderici bazlıdır: her gönderici için ayrı kayıt gerekir.
		// "SKSE"     → SKSE yaşam döngüsü mesajları (kPostLoad, kDataLoaded, ...)
		// "KingCore" → King Core'un REQUEST_API istekleri (API sağlayıcı yanıtının ulaşması için şart)
		if (!messaging->RegisterListener(HandleSKSEMessage)) {
			spdlog::error("Failed to register SKSE messaging listener.");
		}
		if (!messaging->RegisterListener("KingCore", HandleSKSEMessage)) {
			spdlog::warn("KingCore listener registration failed (King Core not loaded?).");
		}
		spdlog::info("Registered SKSE Messaging listeners (SKSE + KingCore).");
	}

	SKSE::log::info("{} has finished loading.", plugin->GetName());
	return true;
}

