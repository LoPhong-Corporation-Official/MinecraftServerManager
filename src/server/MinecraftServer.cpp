#include "server/MinecraftServer.hpp"

#include "core/Logger.hpp"

#include <format>

namespace server
{
namespace
{
constexpr int kMaxAutoRestarts = 5;
constexpr auto kAutoRestartWindow = std::chrono::minutes(10);
constexpr auto kAutoRestartDelay = std::chrono::seconds(2);
} // namespace

MinecraftServer::MinecraftServer(core::ServerConfig config, std::shared_ptr<core::EventDispatcher> events)
    : config_(std::move(config))
    , events_(std::move(events))
{
}

MinecraftServer::~MinecraftServer()
{
    if (GetState() == core::ServerState::Running || GetState() == core::ServerState::Starting)
    {
        // Best-effort graceful shutdown; the destructor cannot report
        // failures, but Stop() still logs internally via process::Process.
        Stop(std::chrono::seconds(10));
    }
}

bool MinecraftServer::Start()
{
    if (GetState() == core::ServerState::Running || GetState() == core::ServerState::Starting)
    {
        core::Logger::Instance().Warning(
            std::format(L"Start() ignored for '{}': already {}", config_.name, ToString(GetState())));
        return false;
    }

    SetState(core::ServerState::Starting);
    stopRequested_ = false;

    process::Process::StartOptions options;
    options.executablePath = config_.javaExecutable;
    options.workingDirectory = config_.directory;
    options.arguments = std::format(
        L"-Xms{}M -Xmx{}M -jar \"{}\" --nogui",
        config_.minMemoryMB,
        config_.maxMemoryMB,
        config_.serverJar.wstring());

    if (!process_.Start(options))
    {
        SetState(core::ServerState::Crashed);
        return false;
    }

    consoleBuffer_.Append(L"[manager] Starting server process...");

    stdoutController_.Start(process_.GetStdoutReadHandle(), [this](const std::wstring& line) {
        OnConsoleLine(line);
    });
    stderrController_.Start(process_.GetStderrReadHandle(), [this](const std::wstring& line) {
        OnConsoleLine(line);
    });

    // Phase 2: CPU%/RAM sampling every 2s while the process is running.
    processMonitor_.Start(process_.GetProcessHandle(), [this](const monitor::ProcessMonitor::Sample& sample) {
        OnStatsSample(sample);
    });

    watcherThread_ = std::jthread([this](std::stop_token) { WatchForExit(); });

    SetState(core::ServerState::Running);
    return true;
}

bool MinecraftServer::Stop(std::chrono::milliseconds timeout)
{
    if (GetState() != core::ServerState::Running)
    {
        return false;
    }

    SetState(core::ServerState::Stopping);
    stopRequested_ = true;

    process_.WriteLine(L"stop");

    const auto timeoutMs = static_cast<DWORD>(timeout.count());
    if (!process_.Wait(timeoutMs))
    {
        consoleBuffer_.Append(L"[manager] Graceful stop timed out, killing process...");
        process_.Terminate(1);
        process_.Wait(5000); // bounded wait for the watcher thread to observe the exit
    }

    // WatchForExit (running on watcherThread_) is responsible for the
    // final state transition to Stopped once GetExitCodeProcess()
    // confirms the process has actually exited, so we don't set state
    // here - doing so from two places would risk racing with it.
    return true;
}

bool MinecraftServer::Restart()
{
    consoleBuffer_.Append(L"[manager] Restarting server...");
    if (GetState() == core::ServerState::Running || GetState() == core::ServerState::Stopping)
    {
        Stop();
    }
    return Start();
}

bool MinecraftServer::SendCommand(const std::wstring& command)
{
    if (GetState() != core::ServerState::Running)
    {
        return false;
    }
    consoleBuffer_.Append(L"> " + command);
    return process_.WriteLine(command);
}

void MinecraftServer::SetState(core::ServerState newState)
{
    state_.store(newState);
    if (events_)
    {
        core::AppEvent event;
        event.type = core::EventType::ServerStateChanged;
        event.serverId = config_.id;
        event.message = ToString(newState);
        event.state = newState;
        events_->Publish(event);
    }
}

void MinecraftServer::OnConsoleLine(const std::wstring& line)
{
    consoleBuffer_.Append(line);
    if (events_)
    {
        core::AppEvent event;
        event.type = core::EventType::ConsoleLine;
        event.serverId = config_.id;
        event.message = line;
        events_->Publish(event);
    }
}

void MinecraftServer::OnStatsSample(const monitor::ProcessMonitor::Sample& sample)
{
    if (events_)
    {
        core::AppEvent event;
        event.type = core::EventType::StatsUpdated;
        event.serverId = config_.id;
        event.cpuPercent = sample.cpuPercent;
        event.memoryBytes = sample.memoryBytes;
        events_->Publish(event);
    }
}

void MinecraftServer::WatchForExit()
{
    // Blocks this dedicated thread (never the UI thread) until the child
    // process exits, however that happens: graceful "stop", a crash, or
    // being killed after a timed-out graceful stop.
    process_.Wait(INFINITE);

    DWORD exitCode = 0;
    process_.GetExitCode(exitCode);

    const bool wasStopRequested = stopRequested_.load();

    if (wasStopRequested)
    {
        consoleBuffer_.Append(L"[manager] Server process stopped.");
        SetState(core::ServerState::Stopped);
    }
    else
    {
        consoleBuffer_.Append(
            std::format(L"[manager] Server process exited unexpectedly (exit code {}).", exitCode));
        HandleUnexpectedExit();
    }
}

void MinecraftServer::HandleUnexpectedExit()
{
    SetState(core::ServerState::Crashed);
    if (events_)
    {
        core::AppEvent event;
        event.type = core::EventType::ServerCrashed;
        event.serverId = config_.id;
        event.message = L"Server process exited unexpectedly.";
        events_->Publish(event);
    }

    if (!config_.autoRestart || !ShouldAutoRestart())
    {
        return;
    }

    SetState(core::ServerState::Restarting);
    consoleBuffer_.Append(L"[manager] Auto-restart scheduled...");

    // Restarting must NOT happen synchronously here: Start() will replace
    // watcherThread_ with a new std::jthread, and reassigning a jthread
    // joins its previous thread first. Since we ARE that previous thread
    // right now, joining ourselves would be undefined behaviour. Handing
    // the restart off to a separate, short-lived thread avoids that.
    std::thread([this]() {
        std::this_thread::sleep_for(kAutoRestartDelay);
        Start();
    }).detach();
}

bool MinecraftServer::ShouldAutoRestart()
{
    std::scoped_lock lock(restartMutex_);
    const auto now = std::chrono::steady_clock::now();

    // Drop restart timestamps older than the rolling window before
    // counting, so this is "N restarts in the last 10 minutes", not
    // "N restarts ever".
    std::erase_if(recentRestarts_, [&](const auto& timestamp) {
        return now - timestamp > kAutoRestartWindow;
    });

    if (static_cast<int>(recentRestarts_.size()) >= kMaxAutoRestarts)
    {
        consoleBuffer_.Append(L"[manager] Auto-restart limit reached (5 within 10 minutes). Giving up.");
        return false;
    }

    recentRestarts_.push_back(now);
    return true;
}

} // namespace server
