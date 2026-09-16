#include "app/Application.hpp"

#include "core/Logger.hpp"

#include <QApplication>
#include <QStyleFactory>

#include <chrono>

namespace app
{
namespace
{

// A dark theme in the spirit of a terminal/server console, applied once
// at the QApplication level so MainWindow, AddServerDialog and every
// QMessageBox share it automatically. Object names (set on specific
// widgets in MainWindow/AddServerDialog) are used to give action buttons
// distinct colours (start = green, stop = red, restart = amber, send =
// blue) instead of one flat grey button style.
constexpr const char* kStyleSheet = R"(
QWidget {
    background-color: #16181d;
    color: #e7e9ee;
    font-family: "Segoe UI", sans-serif;
    font-size: 10pt;
}
QLabel#titleLabel {
    font-size: 15pt;
    font-weight: 600;
    color: #f2f3f5;
    padding-bottom: 2px;
}
QLabel#sectionLabel {
    color: #9aa0ac;
    font-size: 8pt;
    font-weight: 600;
    letter-spacing: 1px;
}
QLabel#statsLabel {
    color: #9aa0ac;
    font-family: Consolas, monospace;
}
QSplitter::handle {
    background-color: #16181d;
    width: 8px;
}
QListWidget {
    background-color: #1e2129;
    border: 1px solid #333844;
    border-radius: 8px;
    padding: 6px;
    outline: 0;
}
QListWidget::item {
    padding: 8px 10px;
    border-radius: 6px;
    margin: 2px 0;
}
QListWidget::item:hover {
    background-color: #262a34;
}
QListWidget::item:selected {
    background-color: #2c5628;
    color: #ffffff;
}
QPlainTextEdit#consoleView {
    background-color: #0d0e12;
    color: #c9d1d9;
    border: 1px solid #333844;
    border-radius: 8px;
    padding: 10px;
}
QLineEdit {
    background-color: #1e2129;
    border: 1px solid #333844;
    border-radius: 6px;
    padding: 7px 10px;
    selection-background-color: #4a90e2;
}
QLineEdit:focus {
    border: 1px solid #4a90e2;
}
QPushButton {
    background-color: #565c68;
    border: none;
    border-radius: 6px;
    padding: 9px 18px;
    font-weight: 600;
    color: #ffffff;
}
QPushButton:hover {
    background-color: #6a7180;
}
QPushButton:pressed {
    background-color: #4d525c;
}
QPushButton:disabled {
    background-color: #2a2d34;
    color: #6a6f79;
}
QPushButton#btnStart {
    background-color: #43a047;
}
QPushButton#btnStart:hover {
    background-color: #4fb553;
}
QPushButton#btnStop {
    background-color: #e5484d;
}
QPushButton#btnStop:hover {
    background-color: #f0595e;
}
QPushButton#btnRestart {
    background-color: #e0a72e;
    color: #221a00;
}
QPushButton#btnRestart:hover {
    background-color: #ecb648;
}
QPushButton#btnSend {
    background-color: #4a90e2;
}
QPushButton#btnSend:hover {
    background-color: #5c9de8;
}
QPushButton#btnNeutral {
    background-color: #565c68;
}
QScrollBar:vertical {
    background: #1e2129;
    width: 12px;
    border-radius: 6px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #444a58;
    border-radius: 6px;
    min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QMessageBox, QDialog {
    background-color: #1b1e25;
}
QDialogButtonBox QPushButton {
    min-width: 84px;
}
)";

} // namespace

int Application::Run(int argc, char** argv)
{
    QApplication qtApp(argc, argv);
    QApplication::setApplicationName("Minecraft Server Manager");
    // Fusion renders custom QSS colours consistently across Windows
    // versions; the native "windowsvista" style ignores several of the
    // rules above (list/scrollbar colours in particular).
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    qtApp.setStyleSheet(QString::fromUtf8(kStyleSheet));

    events_ = std::make_shared<core::EventDispatcher>();
    configManager_ = std::make_shared<config::ConfigManager>();
    serverManager_ = std::make_shared<server::ServerManager>(events_);
    serverManager_->LoadFromConfig(*configManager_);

    mainWindow_ = std::make_unique<ui::MainWindow>(serverManager_, configManager_, events_);
    mainWindow_->show();

    const int exitCode = qtApp.exec();

    // Stop every running server BEFORE mainWindow_/events_ are destroyed
    // (which happens right after Run() returns, when Application itself
    // goes out of scope in main()). MinecraftServer's watcher/console/
    // stats threads call events_->Publish(), whose subscriber calls
    // QMetaObject::invokeMethod(mainWindow_.get(), ...) - so as long as no
    // server is left running, no background thread can touch a MainWindow
    // that is about to be destroyed.
    ShutdownServers();

    return exitCode;
}

void Application::ShutdownServers()
{
    for (const auto& srv : serverManager_->GetAll())
    {
        if (srv->GetState() == core::ServerState::Running || srv->GetState() == core::ServerState::Starting)
        {
            srv->Stop(std::chrono::seconds(20));
        }
    }
}

} // namespace app
