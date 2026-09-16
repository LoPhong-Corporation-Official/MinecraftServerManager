#pragma once

// Owns everything needed to run ONE Minecraft server: its configuration,
// the underlying OS process, the two console readers (stdout/stderr) and
// the state machine described in spec section 7. ServerManager owns a
// collection of these; the UI talks to a specific server through this
// class, never through Process/ConsoleController directly.

#include "console/ConsoleBuffer.hpp"
#include "console/ConsoleController.hpp"
#include "core/EventDispatcher.hpp"
#include "core/Types.hpp"
#include "monitor/ProcessMonitor.hpp"
#include "process/Process.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace server
{

class MinecraftServer
{
public:
    MinecraftServer(core::ServerConfig config, std::shared_ptr<core::EventDispatcher> events);
    ~MinecraftServer();

    MinecraftServer(const MinecraftServer&) = delete;
    MinecraftServer& operator=(const MinecraftServer&) = delete;

    [[nodiscard]] bool Start();

    // Sends "stop" over stdin and waits up to `timeout` for the process to
    // exit gracefully before hard-killing it. Runs synchronously on
    // whichever thread calls it - the UI wraps this call in its own
    // worker thread so the window stays responsive (spec Rule 3).
    bool Stop(std::chrono::milliseconds timeout = std::chrono::seconds(30));

    bool Restart();

    bool SendCommand(const std::wstring& command);

    [[nodiscard]] core::ServerState GetState() const { return state_.load(); }
    [[nodiscard]] const core::ServerConfig& GetConfig() const { return config_; }
    void UpdateConfig(core::ServerConfig config) { config_ = std::move(config); }

    [[nodiscard]] std::vector<std::wstring> GetConsoleLines() const { return consoleBuffer_.GetAll(); }

private:
    void SetState(core::ServerState newState);
    void OnConsoleLine(const std::wstring& line);
    void OnStatsSample(const monitor::ProcessMonitor::Sample& sample); // Phase 2
    void WatchForExit();       // runs on watcherThread_
    void HandleUnexpectedExit();
    [[nodiscard]] bool ShouldAutoRestart();

    core::ServerConfig config_;
    std::shared_ptr<core::EventDispatcher> events_;

    process::Process process_;
    monitor::ProcessMonitor processMonitor_; // declared right after process_ so it is
                                              // destroyed (and its thread stopped) BEFORE
                                              // process_ closes the handle it borrows
    console::ConsoleController stdoutController_;
    console::ConsoleController stderrController_;
    console::ConsoleBuffer consoleBuffer_;

    std::atomic<core::ServerState> state_{core::ServerState::Stopped};
    std::atomic<bool> stopRequested_{false};

    std::jthread watcherThread_;

    // Auto-restart bookkeeping (spec section "Maximum automatic restarts:
    // 5 within 10 minutes").
    std::mutex restartMutex_;
    std::vector<std::chrono::steady_clock::time_point> recentRestarts_;
};

} // namespace server
