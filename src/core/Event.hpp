#pragma once

#include "core/Types.hpp"

#include <cstdint>
#include <string>

namespace core
{

enum class EventType
{
    ServerStateChanged,
    ConsoleLine,
    ServerCrashed,
    StatsUpdated, // Phase 2: periodic CPU%/RAM sample from monitor::ProcessMonitor
};

// A single application event. Kept as one flat struct (rather than a
// polymorphic hierarchy) because every event type here is small and the
// UI layer only ever needs to switch on `type` - no virtual dispatch
// needed, no extra allocations for simple cases.
struct AppEvent
{
    EventType type;
    std::wstring serverId;
    std::wstring message;              // console line text, or a human-readable detail
    ServerState state = ServerState::Stopped; // meaningful when type == ServerStateChanged
    double cpuPercent = 0.0;           // meaningful when type == StatsUpdated
    std::uint64_t memoryBytes = 0;     // meaningful when type == StatsUpdated
};

} // namespace core

