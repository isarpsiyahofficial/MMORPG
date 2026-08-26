#pragma once

// Official game-client side of Premium Plus Combo.
// Link/include this in the game client itself; no DLL injection or process hooking.
// Call Receiver::DrainKeyTaps() once from the normal game update/input tick and
// dispatch the supplied virtual key through the same action path as a physical key.

#include <windows.h>
#include <cstdint>
#include <utility>
#include "bridge_protocol.hpp"

namespace ppc_game {

class Receiver {
public:
    Receiver() = default;
    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;

    ~Receiver() { Close(); }

    bool Open() noexcept {
        if (shared_) return true;
        HANDLE map = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, ppc_bridge::kMappingName);
        if (!map) return false;
        void* view = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ppc_bridge::SharedState));
        if (!view) {
            CloseHandle(map);
            return false;
        }
        auto* s = static_cast<ppc_bridge::SharedState*>(view);
        if (!ppc_bridge::IsHeaderValid(s)) {
            UnmapViewOfFile(view);
            CloseHandle(map);
            return false;
        }
        map_ = map;
        shared_ = s;
        readSequence_ = InterlockedCompareExchange64(&shared_->writeSequence, 0, 0);
        Heartbeat();
        return true;
    }

    void Close() noexcept {
        if (shared_) {
            InterlockedExchange64(&shared_->gameHeartbeatMs, 0);
            UnmapViewOfFile(shared_);
            shared_ = nullptr;
        }
        if (map_) {
            CloseHandle(map_);
            map_ = nullptr;
        }
        readSequence_ = 0;
    }

    bool Connected() noexcept {
        if (!shared_ && !Open()) return false;
        Heartbeat();
        return true;
    }

    void Heartbeat() noexcept {
        if (shared_)
            InterlockedExchange64(&shared_->gameHeartbeatMs, static_cast<LONG64>(GetTickCount64()));
    }

    template <typename Fn>
    std::size_t Drain(Fn&& fn, std::size_t maxEvents = ppc_bridge::kRingSize) noexcept {
        if (!shared_ && !Open()) return 0;
        Heartbeat();

        LONG64 write = InterlockedCompareExchange64(&shared_->writeSequence, 0, 0);
        if (write <= readSequence_) return 0;

        // If the game was stalled for longer than the ring capacity, resume from
        // the oldest event still guaranteed to exist instead of replaying corrupt data.
        if (write - readSequence_ > static_cast<LONG64>(ppc_bridge::kRingSize))
            readSequence_ = write - static_cast<LONG64>(ppc_bridge::kRingSize);

        std::size_t drained = 0;
        while (readSequence_ < write && drained < maxEvents) {
            const LONG64 seq = readSequence_ + 1;
            const ppc_bridge::Event& slot =
                shared_->events[static_cast<std::size_t>(seq) % ppc_bridge::kRingSize];

            const LONG64 published = InterlockedCompareExchange64(
                const_cast<volatile LONG64*>(&slot.sequence), 0, 0);
            if (published != seq) break;

            MemoryBarrier();
            fn(static_cast<int>(slot.virtualKey), slot.flags);
            readSequence_ = seq;
            ++drained;
        }
        Heartbeat();
        return drained;
    }

    template <typename Fn>
    std::size_t DrainKeyTaps(Fn&& onKeyDown, std::size_t maxEvents = ppc_bridge::kRingSize) noexcept {
        return Drain([&](int vk, std::uint32_t flags) {
            if ((flags & ppc_bridge::KeyDown) != 0)
                onKeyDown(vk);
        }, maxEvents);
    }

private:
    HANDLE map_{};
    ppc_bridge::SharedState* shared_{};
    LONG64 readSequence_{};
};

} // namespace ppc_game
