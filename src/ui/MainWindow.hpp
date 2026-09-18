#pragma once

// The application's main window, now built on Qt Widgets instead of raw
// Win32 controls. The process/console/server backend underneath is
// unchanged (still native Win32 CreateProcessW + Job Objects) - only this
// UI layer talks Qt.
//
// Threading contract: every method on this class must only run on the Qt
// UI (main) thread. Background threads (console readers, the watcher
// thread in MinecraftServer) notify this window exclusively through
// NotifyEvent(), which marshals onto the UI thread via
// QMetaObject::invokeMethod - the Qt equivalent of the PostMessageW
// pattern used by a raw Win32 window.

#include "config/ConfigManager.hpp"
#include "core/EventDispatcher.hpp"
#include "server/ServerManager.hpp"

#include <QMainWindow>
#include <QString>

#include <memory>
#include <string>

class QListWidget;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;
class QStackedWidget;
class QToolBar;

namespace ui
{

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(
        std::shared_ptr<server::ServerManager> serverManager,
        std::shared_ptr<config::ConfigManager> configManager,
        std::shared_ptr<core::EventDispatcher> events,
        QWidget* parent = nullptr);

    // Thread-safe: callable from any thread. Marshals the actual handling
    // onto the Qt UI thread.
    void NotifyEvent(const core::AppEvent& event);

private slots:
    void OnAddServerClicked();
    void OnRemoveServerClicked();
    void OnStartClicked();
    void OnStopClicked();
    void OnRestartClicked();
    void OnSendCommandClicked();
    void OnServerSelectionChanged();
    void OnOpenLibraryClicked(); // Modrinth mods/plugins/modpacks browser

private:
    void BuildToolbar();
    void HandleAppEvent(core::AppEvent event); // always runs on the UI thread
    void RefreshServerList();
    void RefreshSelectedServerConsole();
    void RefreshControlsForState();
    void UpdateStatsLabel(); // rebuilds labelStats_ from the two cached parts below
    void AppendConsoleLine(const QString& line);
    [[nodiscard]] std::wstring GetSelectedServerId() const;

    std::shared_ptr<server::ServerManager> serverManager_;
    std::shared_ptr<config::ConfigManager> configManager_;
    std::shared_ptr<core::EventDispatcher> events_;

    QToolBar* toolbar_ = nullptr;

    // Startup/empty-state page (index 0) vs. the normal server view
    // (index 1) - shown depending on whether any server profile exists
    // yet, so a brand new install greets the user with a call-to-action
    // instead of a blank console.
    QStackedWidget* stackedPages_ = nullptr;

    QListWidget* listServers_ = nullptr;
    QPushButton* buttonAdd_ = nullptr;
    QPushButton* buttonRemove_ = nullptr;
    QLabel* labelStatus_ = nullptr;
    QLabel* labelStats_ = nullptr;
    QPlainTextEdit* editConsole_ = nullptr;
    QLineEdit* editCommand_ = nullptr;
    QPushButton* buttonSend_ = nullptr;
    QPushButton* buttonStart_ = nullptr;
    QPushButton* buttonStop_ = nullptr;
    QPushButton* buttonRestart_ = nullptr;

    // Cached pieces of the stats line, so a cpu/ram sample and a
    // separately-arriving TPS reply (see core::AppEvent's cpuPercent
    // sentinel) don't clobber each other - each updates only its own
    // piece, then UpdateStatsLabel() joins them for display.
    QString cachedCpuRamText_;
    QString cachedTpsText_;
};

} // namespace ui
