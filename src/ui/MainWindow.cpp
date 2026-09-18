#include "ui/MainWindow.hpp"

#include "ui/AddServerDialog.hpp"
#include "ui/LibraryDialog.hpp"

#include <QAction>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTextCursor>
#include <QToolBar>
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

// Every distinct section of the window (server list, console, controls)
// lives inside one of these "widget cards" - a QFrame with its own
// rounded background (see QSS in Application.cpp), so sections read as
// separate panels with breathing room instead of one flat wall of
// controls touching the window edges.
QFrame* MakeCard(QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("card"));
    card->setFrameShape(QFrame::NoFrame); // the rounded border comes from QSS, not Qt's native frame drawing
    return card;
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
    resize(1120, 700);

    BuildToolbar();

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(24, 20, 24, 24);
    outerLayout->setSpacing(18);

    auto* titleLabel = new QLabel(QStringLiteral("🎮  Minecraft Server Manager"), central);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    outerLayout->addWidget(titleLabel);

    stackedPages_ = new QStackedWidget(central);

    // --- Page 0: startup / empty-state page --------------------------
    auto* emptyPage = new QWidget(stackedPages_);
    auto* emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto* emptyIcon = new QLabel(QStringLiteral("🚀"), emptyPage);
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setStyleSheet(QStringLiteral("font-size: 48pt;"));

    auto* emptyTitle = new QLabel(QStringLiteral("Chào mừng đến với Minecraft Server Manager"), emptyPage);
    emptyTitle->setObjectName(QStringLiteral("titleLabel"));
    emptyTitle->setAlignment(Qt::AlignCenter);

    auto* emptySubtitle = new QLabel(
        QStringLiteral("Bạn chưa có server nào. Tạo server đầu tiên để bắt đầu quản lý, xem console\n"
                        "thời gian thực, và cài mod/plugin trực tiếp từ Modrinth."),
        emptyPage);
    emptySubtitle->setObjectName(QStringLiteral("hintLabel"));
    emptySubtitle->setAlignment(Qt::AlignCenter);
    emptySubtitle->setWordWrap(true);
    emptySubtitle->setMaximumWidth(460);

    auto* emptyButton = new QPushButton(QStringLiteral("+  Tạo Server Đầu Tiên"), emptyPage);
    emptyButton->setObjectName(QStringLiteral("btnStart"));
    emptyButton->setMinimumSize(220, 42);
    connect(emptyButton, &QPushButton::clicked, this, &MainWindow::OnAddServerClicked);

    emptyLayout->addStretch(1);
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addSpacing(4);
    emptyLayout->addWidget(emptySubtitle, 0, Qt::AlignHCenter);
    emptyLayout->addSpacing(16);
    emptyLayout->addWidget(emptyButton, 0, Qt::AlignHCenter);
    emptyLayout->addStretch(1);

    stackedPages_->addWidget(emptyPage); // index 0

    // --- Page 1: the normal server-list + console + controls view ----
    auto* splitter = new QSplitter(Qt::Horizontal, stackedPages_);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(18);

    // Left card: server list
    auto* serversCard = MakeCard(splitter);
    auto* leftLayout = new QVBoxLayout(serversCard);
    leftLayout->setContentsMargins(18, 16, 18, 18);
    leftLayout->setSpacing(12);

    auto* serversLabel = new QLabel(QStringLiteral("SERVERS"), serversCard);
    serversLabel->setObjectName(QStringLiteral("sectionLabel"));
    leftLayout->addWidget(serversLabel);

    listServers_ = new QListWidget(serversCard);
    leftLayout->addWidget(listServers_, 1);

    auto* leftButtons = new QHBoxLayout();
    leftButtons->setSpacing(10);
    buttonAdd_ = new QPushButton(QStringLiteral("+ Thêm"), serversCard);
    buttonAdd_->setObjectName(QStringLiteral("btnStart"));
    buttonRemove_ = new QPushButton(QStringLiteral("Xoá"), serversCard);
    buttonRemove_->setObjectName(QStringLiteral("btnNeutral"));
    leftButtons->addWidget(buttonAdd_);
    leftButtons->addWidget(buttonRemove_);
    leftLayout->addLayout(leftButtons);

    splitter->addWidget(serversCard);

    // Right side: two stacked cards (console, then controls)
    auto* rightContainer = new QWidget(splitter);
    auto* rightOuter = new QVBoxLayout(rightContainer);
    rightOuter->setContentsMargins(0, 0, 0, 0);
    rightOuter->setSpacing(18);

    auto* consoleCard = MakeCard(rightContainer);
    auto* consoleLayout = new QVBoxLayout(consoleCard);
    consoleLayout->setContentsMargins(18, 16, 18, 18);
    consoleLayout->setSpacing(12);

    auto* statusRow = new QHBoxLayout();
    labelStatus_ = new QLabel(QStringLiteral("Chưa chọn server"), consoleCard);
    labelStatus_->setObjectName(QStringLiteral("statusLabel"));
    labelStats_ = new QLabel(QStringLiteral(""), consoleCard);
    labelStats_->setObjectName(QStringLiteral("statsLabel"));
    statusRow->addWidget(labelStatus_);
    statusRow->addStretch(1);
    statusRow->addWidget(labelStats_);
    consoleLayout->addLayout(statusRow);

    editConsole_ = new QPlainTextEdit(consoleCard);
    editConsole_->setReadOnly(true);
    editConsole_->setObjectName(QStringLiteral("consoleView"));
    QFont consoleFont(QStringLiteral("Consolas"), 10);
    consoleFont.setStyleHint(QFont::Monospace);
    editConsole_->setFont(consoleFont);
    consoleLayout->addWidget(editConsole_, 1);

    rightOuter->addWidget(consoleCard, 1);

    auto* controlsCard = MakeCard(rightContainer);
    auto* controlsLayout = new QVBoxLayout(controlsCard);
    controlsLayout->setContentsMargins(18, 16, 18, 16);
    controlsLayout->setSpacing(12);

    auto* commandLayout = new QHBoxLayout();
    commandLayout->setSpacing(10);
    editCommand_ = new QLineEdit(controlsCard);
    editCommand_->setPlaceholderText(QStringLiteral("Nhập lệnh console (vd: say hello, op Steve...)"));
    buttonSend_ = new QPushButton(QStringLiteral("Gửi"), controlsCard);
    buttonSend_->setObjectName(QStringLiteral("btnSend"));
    commandLayout->addWidget(editCommand_, 1);
    commandLayout->addWidget(buttonSend_);
    controlsLayout->addLayout(commandLayout);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);
    buttonStart_ = new QPushButton(QStringLiteral("▶  Start"), controlsCard);
    buttonStart_->setObjectName(QStringLiteral("btnStart"));
    buttonStop_ = new QPushButton(QStringLiteral("■  Stop"), controlsCard);
    buttonStop_->setObjectName(QStringLiteral("btnStop"));
    buttonRestart_ = new QPushButton(QStringLiteral("⟳  Restart"), controlsCard);
    buttonRestart_->setObjectName(QStringLiteral("btnRestart"));
    actionLayout->addWidget(buttonStart_);
    actionLayout->addWidget(buttonStop_);
    actionLayout->addWidget(buttonRestart_);
    actionLayout->addStretch(1);
    controlsLayout->addLayout(actionLayout);

    rightOuter->addWidget(controlsCard);

    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 800});

    stackedPages_->addWidget(splitter); // index 1

    outerLayout->addWidget(stackedPages_, 1);

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

void MainWindow::BuildToolbar()
{
    toolbar_ = addToolBar(QStringLiteral("Chính"));
    toolbar_->setMovable(false);
    toolbar_->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto* actionNew = toolbar_->addAction(QStringLiteral("🆕  Server mới"));
    connect(actionNew, &QAction::triggered, this, &MainWindow::OnAddServerClicked);

    auto* actionRemove = toolbar_->addAction(QStringLiteral("🗑  Xoá server"));
    connect(actionRemove, &QAction::triggered, this, &MainWindow::OnRemoveServerClicked);

    toolbar_->addSeparator();

    auto* actionLibrary = toolbar_->addAction(QStringLiteral("📚  Thư viện Mod/Plugin"));
    connect(actionLibrary, &QAction::triggered, this, &MainWindow::OnOpenLibraryClicked);

    toolbar_->addSeparator();

    auto* actionRefresh = toolbar_->addAction(QStringLiteral("⟲  Làm mới"));
    connect(actionRefresh, &QAction::triggered, this, [this]() { RefreshServerList(); });
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
                // cpuPercent < 0 is the sentinel meaning "this event only
                // carries a fresh TPS reply, not a new cpu/mem sample" -
                // see core::AppEvent's comment for why the two are decoupled.
                if (event.cpuPercent >= 0.0)
                {
                    const double memoryMB = static_cast<double>(event.memoryBytes) / (1024.0 * 1024.0);
                    cachedCpuRamText_ = QStringLiteral("CPU %1%   ·   RAM %2 MB")
                        .arg(QString::number(event.cpuPercent, 'f', 1))
                        .arg(QString::number(memoryMB, 'f', 0));
                }
                if (!event.tpsText.empty())
                {
                    cachedTpsText_ = QString::fromStdWString(event.tpsText);
                }
                UpdateStatsLabel();
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
    // Clear stale stats immediately - otherwise the previously-selected
    // server's last CPU/RAM/TPS text would stay on screen until the newly
    // selected server's next periodic sample arrives.
    cachedCpuRamText_.clear();
    cachedTpsText_.clear();
    labelStats_->setText(QString());

    RefreshSelectedServerConsole();
    RefreshControlsForState();
}

void MainWindow::OnOpenLibraryClicked()
{
    auto srv = serverManager_->Get(GetSelectedServerId());
    if (!srv)
    {
        QMessageBox::information(
            this,
            QStringLiteral("Thư viện Mod/Plugin"),
            QStringLiteral("Hãy chọn (hoặc tạo) một server trước khi mở Thư viện."));
        return;
    }
    LibraryDialog dialog(srv->GetConfig(), this);
    dialog.exec();
}

void MainWindow::RefreshServerList()
{
    const std::wstring currentlySelected = GetSelectedServerId();

    listServers_->blockSignals(true);
    listServers_->clear();

    QListWidgetItem* itemToSelect = nullptr;
    const auto allServers = serverManager_->GetAll();
    for (const auto& srv : allServers)
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
        // explicit refresh here - including clearing stale stats, same as
        // OnServerSelectionChanged does for a user-driven selection change.
        cachedCpuRamText_.clear();
        cachedTpsText_.clear();
        labelStats_->setText(QString());
        RefreshSelectedServerConsole();
    }
    RefreshControlsForState();

    // Startup page: greet a fresh install (or one where every server was
    // removed) with a call-to-action instead of an empty console.
    stackedPages_->setCurrentIndex(allServers.empty() ? 0 : 1);
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
        cachedCpuRamText_.clear();
        cachedTpsText_.clear();
    }
}

void MainWindow::UpdateStatsLabel()
{
    if (cachedCpuRamText_.isEmpty())
    {
        labelStats_->setText(cachedTpsText_);
        return;
    }
    if (cachedTpsText_.isEmpty())
    {
        labelStats_->setText(cachedCpuRamText_);
        return;
    }
    labelStats_->setText(cachedCpuRamText_ + QStringLiteral("   ·   ") + cachedTpsText_);
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
