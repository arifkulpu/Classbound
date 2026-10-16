#pragma once

#include "KingCore/KingCoreAPI.h"
#include "ClassData.h"
#include <vector>
#include <memory>
#include <mutex>

namespace RE
{
    class Actor;
}

namespace Classbound
{
    class KingCoreAdapter : public KingCore::IClassBoundAPI
    {
    public:
        static KingCoreAdapter* GetSingleton()
        {
            static KingCoreAdapter instance;
            return &instance;
        }

        // IClassBoundAPI implementation
        [[nodiscard]] uint32_t GetVersion() const noexcept override;
        [[nodiscard]] bool IsActorEligible(RE::Actor* actor) const noexcept override;

        [[nodiscard]] KingCore::ClassID GetFollowerClass(RE::Actor* actor) const noexcept override;
        [[nodiscard]] uint32_t GetFollowerClassLevel(RE::Actor* actor) const noexcept override;
        [[nodiscard]] std::string_view GetFollowerSpecialization(RE::Actor* actor) const noexcept override;
        [[nodiscard]] bool GetFollowerClassInfo(RE::Actor* actor, KingCore::ClassInfo& outInfo) const noexcept override;

        [[nodiscard]] bool GetBehaviorProfile(RE::Actor* actor, KingCore::BehaviorProfile& outProfile) const noexcept override;

        void RegisterEventListener(KingCore::IClassEventListener* listener) noexcept override;
        void UnregisterEventListener(KingCore::IClassEventListener* listener) noexcept override;

        bool AssignFollowerClass(RE::Actor* actor, KingCore::ClassID newClass) noexcept override;

        // Dispatch helper called by ClassBound when changes occur
        void NotifyClassEvent(KingCore::ClassEventType type, RE::Actor* actor, KingCore::ClassID oldClass, KingCore::ClassID newClass, uint32_t newLevel);

        // Helper conversion
        static KingCore::ClassID ToKingCoreClass(ClassType ct);
        static ClassType ToClassBoundClass(KingCore::ClassID id);

    private:
        KingCoreAdapter() = default;
        ~KingCoreAdapter() override = default;

        // Guards _listeners against concurrent Register/Unregister/Notify calls.
        // NotifyClassEvent is called lock-free by King Core's BroadcastEvent, so we
        // must protect the vector ourselves.
        mutable std::mutex _listenerMutex;
        std::vector<KingCore::IClassEventListener*> _listeners;
    };
}

