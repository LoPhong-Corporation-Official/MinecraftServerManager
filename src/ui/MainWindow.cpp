#include "ui/MainWindow.hpp"

#include "ui/AddServerDialog.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

#include <thread>

namespace ui
{
namespace
{

// Small helper so RefreshControlsForState() can pick a status colour that
// matches the state, instead of every state looking identical.
QString ColorForState(core::ServerState state)
{
    switch (state)
    {
        case core::ServerState::Running: return QStringLiteral("#4caf50");
        case core::ServerState::Crashed: return QStringLiteral("#e5484d");
        case core::ServerState::Stopped: return QStringLiteral("#9aa0ac");
        default: return QStringLiteral("#e0a72e"); // Starting / Stopping / Restarting
    }
}

} // namespace

MainWindow::MainWindow(
    std::shared_ptr<server::ServerManager> serverManager,
    std::shared_ptr<config::ConfigManager> configManager,
    std::shared_ptr<core::EventDispatcher> events,
    QWidget* parent)
    : QMainWindow(parent)
    , serverManager_(std::move(serverManager))
    , configManager_(std::move(configManager))
    , events_(std::move(events))
{
    setWindowTitle(QStringLiteral("Minecraft Server Manager"));
    resize(1080, 680);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(16, 12, 16, 16);
    outerLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QStringLiteral("🎮  Minecraft Server Manager"), central);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    outerLayout->addWidget(titleLabel);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);

    // --- Left panel: server list -----------------------------------
    auto* leftPanel = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* serversLabel = new QLabel(QStringLiteral("SERVERS"), leftPanel);
    serversLabel->setObjectName(QStringLiteral("sectionLabel"));
    leftLayout->addWidget(serversLabel);

    listServers_ = new QListWidget(leftPanel);
    leftLayout->addWidget(listServers_, 1);

    auto* leftButtons = new QHBoxLayout();
    leftButtons->setSpacing(8);
    buttonAdd_ = new QPushButton(QStringLiteral("+ Thêm"), leftPanel);
    buttonAdd_->setObjectName(QStringLiteral("btnStart"));
    buttonRemove_ = new QPushButton(QStringLiteral("Xoá"), leftPanel);
    buttonRemove_->setObjectName(QStringLiteral("btnNeutral"));
    leftButtons->addWidget(buttonAdd_);
    leftButtons->addWidget(buttonRemove_);
    leftLayout->addLayout(leftButtons);

    splitter->addWidget(leftPanel);

    // --- Right panel: status, console, commands, actions ------------
    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* statusRow = new QHBoxLayout();
    labelStatus_ = new QLabel(QStringLiteral("Chưa chọn server"), rightPanel);
    labelStatus_->setObjectName(QStringLiteral("statusLabel"));
    labelStats_ = new QLabel(QStringLiteral(""), rightPanel);
    labelStats_->setObjectName(QStringLiteral("statsLabel"));
    statusRow->addWidget(labelStatus_);
    statusRow->addStretch(1);
    statusRow->addWidget(labelStats_);
    rightLayout->addLayout(statusRow);

    editConsole_ = new QPlainTextEdit(rightPanel);
    editConsole_->setReadOnly(true);
    editConsole_->setObjectName(QStringLiteral("consoleView"));
    QFont consoleFont(QStringLiteral("Consolas"), 10);
    consoleFont.setStyleHint(QFont::Monospace);
    editConsole_->setFont(consoleFont);
    rightLayout->addWidget(editConsole_, 1);

    auto* commandLayout = new QHBoxLayout();
    commandLayout->setSpacing(8);
    editCommand_ = new QLineEdit(rightPanel);
    editCommand_->setPlaceholderText(QStringLiteral("Nhập lệnh console (vd: say hello, op Steve...)"));
    buttonSend_ = new QPushButton(QStringLiteral("Gửi"), rightPanel);
    buttonSend_->setObjectName(QStringLiteral("btnSend"));
    commandLayout->addWidget(editCommand_, 1);
    commandLayout->addWidget(buttonSend_);
    rightLayout->addLayout(commandLayout);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);
    buttonStart_ = new QPushButton(QStringLiteral("▶  Start"), rightPanel);
    buttonStart_->setObjectName(QStringLiteral("btnStart"));
    buttonStop_ = new QPushButton(QStringLiteral("■  Stop"), rightPanel);
    buttonStop_->setObjectName(QStringLiteral("btnStop"));
    buttonRestart_ = new QPushButton(QStringLiteral("⟳  Restart"), rightPanel);
    buttonRestart_->setObjectName(QStringLiteral("btnRestart"));
    actionLayout->addWidget(buttonStart_);
    actionLayout->addWidget(buttonStop_);
    actionLayout->addWidget(buttonRestart_);
    actionLayout->addStretch(1);
    rightLayout->addLayout(actionLayout);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 780});

    outerLayout->addWidget(splitter, 1);

    connect(buttonAdd_, &QPushButton::clicked, this, &MainWindow::OnAddServerClicked);
    connect(buttonRemove_, &QPushButton::clicked, this, &MainWindow::OnRemoveServerClicked);
    connect(buttonStart_, &QPushButton::clicked, this, &MainWindow::OnStartClicked);
    connect(buttonStop_, &QPushButton::clicked, this, &MainWindow::OnStopClicked);
    connect(buttonRestart_, &QPushButton::clicked, this, &MainWindow::OnRestartClicked);
    connect(buttonSend_, &QPushButton::clicked, this, &MainWindow::OnSendCommandClicked);
    // Enter-to-send: something the raw Win32 version did not have without
    // extra plumbing - Qt gives it to us for free via QLineEdit::returnPressed.
    connect(editCommand_, &QLineEdit::returnPressed, this, &MainWindow::OnSendCommandClicked);
    connect(listServers_, &QListWidget::itemSelectionChanged, this, &MainWindow::OnServerSelectionChanged);

    // Marshal every server event onto the UI thread. The lambda below runs
    // on whichever background thread published the event (a console
    // reader, the stats sampler, or MinecraftServer's watcher thread);
    // NotifyEvent() is the only thing touching `this` from that thread,
    // and it is safe to call from any thread by design (see below).
    events_->Subscribe([this](const core::AppEvent& event) { NotifyEvent(event); });

    RefreshServerList();
}

void MainWindow::NotifyEvent(const core::AppEvent& event)
{
    // QMetaObject::invokeMethod with Qt::QueuedConnection posts the call
    // onto this object's thread affinity (the UI thread, since MainWindow
    // was constructed there) and returns immediately - this is Qt's
    // equivalent of PostMessageW, and is safe to call from any thread.
    QMetaObject::invokeMethod(this, [this, event]() { HandleAppEvent(event); }, Qt::QueuedConnection);
}

void MainWindow::HandleAppEvent(core::AppEvent event)
{
    switch (event.type)
    {
        case core::EventType::ServerStateChanged:
            // The list shows each server's state inline, so any state
            // change needs the whole list re-rendered, not just the
            // selected row.
            RefreshServerList();
            break;
        case core::EventType::ConsoleLine:
            if (event.serverId == GetSelectedServerId())
            {
                AppendConsoleLine(QString::fromStdWString(event.message));
            }
            break;
        case core::EventType::StatsUpdated:
            if (event.serverId == GetSelectedServerId())
            {
                const double memoryMB = static_cast<double>(event.memoryBytes) / (1024.0 * 1024.0);
                labelStats_->setText(QStringLiteral("CPU %1%   ·   RAM %2 MB")
                    .arg(QString::number(event.cpuPercent, 'f', 1))
                    .arg(QString::number(memoryMB, 'f', 0)));
            }
            break;
        case core::EventType::ServerCrashed:
            // The corresponding console line and state change are handled
            // by the two cases above; nothing extra needed for the MVP.
            break;
    }
}

void MainWindow::OnAddServerClicked()
{
    AddServerDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    auto config = dialog.GetResult();
    if (!config.has_value())
    {
        return;
    }
    serverManager_->AddServer(*config, *configManager_);
    RefreshServerList();
}

void MainWindow::OnRemoveServerClicked()
{
    const std::wstring id = GetSelectedServerId();
    if (id.empty())
    {
        return;
    }
    auto srv = serverManager_->Get(id);
    const QString name = srv ? QString::fromStdWString(srv->GetConfig().name) : QString::fromStdWString(id);

    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("Xác nhận"),
        QStringLiteral("Xoá server \"%1\"? Server sẽ bị dừng nếu đang chạy.").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    serverManager_->RemoveServer(id, *configManager_);
    RefreshServerList();
    editConsole_->clear();
}

void MainWindow::OnStartClicked()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        return;
    }
    // Moved off the UI thread so a slow antivirus scan on java.exe (or
    // any other CreateProcessW hiccup) cannot freeze the window.
    std::thread([srv]() { srv->Start(); }).detach();
}

void MainWindow::OnStopClicked()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        return;
    }
    // Stop() can block for up to ~30 seconds waiting for a graceful
    // shutdown, so it must never run on the UI thread.
    std::thread([srv]() { srv->Stop(); }).detach();
}

void MainWindow::OnRestartClicked()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        return;
    }
    std::thread([srv]() { srv->Restart(); }).detach();
}

void MainWindow::OnSendCommandClicked()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        return;
    }
    const QString text = editCommand_->text();
    if (text.isEmpty())
    {
        return;
    }
    srv->SendCommand(text.toStdWString());
    editCommand_->clear();
}

void MainWindow::OnServerSelectionChanged()
{
    RefreshSelectedServerConsole();
    RefreshControlsForState();
}

void MainWindow::RefreshServerList()
{
    const std::wstring currentlySelected = GetSelectedServerId();

    listServers_->blockSignals(true);
    listServers_->clear();

    QListWidgetItem* itemToSelect = nullptr;
    for (const auto& srv : serverManager_->GetAll())
    {
        const auto& cfg = srv->GetConfig();
        const QString label = QString::fromStdWString(cfg.name) +
            QStringLiteral("  ·  ") + QString::fromWCharArray(core::ToString(srv->GetState()));

        auto* item = new QListWidgetItem(label, listServers_);
        item->setData(Qt::UserRole, QString::fromStdWString(cfg.id));
        if (cfg.id == currentlySelected)
        {
            itemToSelect = item;
        }
    }

    if (itemToSelect != nullptr)
    {
        listServers_->setCurrentItem(itemToSelect);
    }
    else if (listServers_->count() > 0)
    {
        listServers_->setCurrentRow(0);
    }
    listServers_->blockSignals(false);

    if (itemToSelect == nullptr)
    {
        // Selection actually changed (or the list became empty) while
        // signals were blocked above, so the dependent panels need an
        // explicit refresh here.
        RefreshSelectedServerConsole();
    }
    RefreshControlsForState();
}

void MainWindow::RefreshSelectedServerConsole()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        editConsole_->clear();
        return;
    }

    QStringList lines;
    for (const auto& line : srv->GetConsoleLines())
    {
        lines << QString::fromStdWString(line);
    }
    editConsole_->setPlainText(lines.join(QStringLiteral("\n")));

    QTextCursor cursor = editConsole_->textCursor();
    cursor.movePosition(QTextCursor::End);
    editConsole_->setTextCursor(cursor);
}

void MainWindow::RefreshControlsForState()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        buttonStart_->setEnabled(false);
        buttonStop_->setEnabled(false);
        buttonRestart_->setEnabled(false);
        buttonSend_->setEnabled(false);
        buttonRemove_->setEnabled(false);
        labelStatus_->setText(QStringLiteral("Chưa chọn server"));
        labelStatus_->setStyleSheet(QString());
        labelStats_->setText(QString());
        return;
    }

    const auto state = srv->GetState();
    const bool running = state == core::ServerState::Running;
    const bool transitioning =
        state == core::ServerState::Starting ||
        state == core::ServerState::Stopping ||
        state == core::ServerState::Restarting;

    buttonStart_->setEnabled(!running && !transitioning);
    buttonStop_->setEnabled(running);
    buttonRestart_->setEnabled(running);
    buttonSend_->setEnabled(running);
    buttonRemove_->setEnabled(!transitioning);

    labelStatus_->setText(QStringLiteral("●  ") + QString::fromWCharArray(core::ToString(state)));
    labelStatus_->setStyleSheet(
        QStringLiteral("QLabel#statusLabel { color: %1; font-weight: 600; font-size: 11pt; }")
            .arg(ColorForState(state)));

    if (!running)
    {
        labelStats_->setText(QString());
    }
}

void MainWindow::AppendConsoleLine(const QString& line)
{
    editConsole_->appendPlainText(line);
}

std::wstring MainWindow::GetSelectedServerId() const
{
    const auto* item = listServers_->currentItem();
    if (item == nullptr)
    {
        return L"";
    }
    return item->data(Qt::UserRole).toString().toStdWString();
}

} // namespace ui
