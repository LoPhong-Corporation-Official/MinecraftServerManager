#include "ui/AdvancedSettingsDialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <thread>

namespace ui
{

AdvancedSettingsDialog::AdvancedSettingsDialog(
    std::shared_ptr<server::MinecraftServer> server,
    std::shared_ptr<server::ServerManager> serverManager,
    std::shared_ptr<config::ConfigManager> configManager,
    QWidget* parent)
    : QDialog(parent)
    , server_(std::move(server))
    , serverManager_(std::move(serverManager))
    , configManager_(std::move(configManager))
{
    setWindowTitle(QStringLiteral("Cài đặt nâng cao — %1").arg(QString::fromStdWString(server_->GetConfig().name)));
    resize(520, 480);

    auto* layout = new QVBoxLayout(this);

    // --- Tunnel group (Phase 6) --------------------------------------
    auto* tunnelGroup = new QGroupBox(QStringLiteral("Network Tunnel (Playit.gg / Cloudflare Tunnel)"), this);
    auto* tunnelForm = new QFormLayout(tunnelGroup);

    comboPreset_ = new QComboBox(tunnelGroup);
    comboPreset_->addItems({QStringLiteral("Tuỳ chỉnh"), QStringLiteral("Playit.gg"), QStringLiteral("Cloudflare Tunnel")});
    tunnelForm->addRow(QStringLiteral("Loại:"), comboPreset_);

    labelPresetHint_ = new QLabel(tunnelGroup);
    labelPresetHint_->setObjectName(QStringLiteral("hintLabel"));
    labelPresetHint_->setWordWrap(true);
    tunnelForm->addRow(QString(), labelPresetHint_);

    auto* execRow = new QWidget(tunnelGroup);
    auto* execLayout = new QHBoxLayout(execRow);
    execLayout->setContentsMargins(0, 0, 0, 0);
    execLayout->setSpacing(6);
    editTunnelExecutable_ = new QLineEdit(execRow);
    execLayout->addWidget(editTunnelExecutable_, 1);
    auto* browseButton = new QPushButton(QStringLiteral("..."), execRow);
    browseButton->setObjectName(QStringLiteral("btnNeutral"));
    browseButton->setFixedWidth(36);
    execLayout->addWidget(browseButton);
    tunnelForm->addRow(QStringLiteral("File thực thi:"), execRow);
    connect(browseButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::OnBrowseTunnelExecutable);

    editTunnelArguments_ = new QLineEdit(tunnelGroup);
    tunnelForm->addRow(QStringLiteral("Tham số dòng lệnh:"), editTunnelArguments_);

    checkTunnelEnabled_ = new QCheckBox(QStringLiteral("Bật tunnel cho server này"), tunnelGroup);
    tunnelForm->addRow(QString(), checkTunnelEnabled_);

    checkTunnelAutoStart_ = new QCheckBox(QStringLiteral("Tự khởi động tunnel cùng lúc Start server"), tunnelGroup);
    tunnelForm->addRow(QString(), checkTunnelAutoStart_);

    auto* tunnelButtonRow = new QWidget(tunnelGroup);
    auto* tunnelButtonLayout = new QHBoxLayout(tunnelButtonRow);
    tunnelButtonLayout->setContentsMargins(0, 0, 0, 0);
    buttonStartTunnel_ = new QPushButton(QStringLiteral("▶ Start tunnel ngay"), tunnelButtonRow);
    buttonStartTunnel_->setObjectName(QStringLiteral("btnStart"));
    buttonStopTunnel_ = new QPushButton(QStringLiteral("■ Stop tunnel"), tunnelButtonRow);
    buttonStopTunnel_->setObjectName(QStringLiteral("btnStop"));
    tunnelButtonLayout->addWidget(buttonStartTunnel_);
    tunnelButtonLayout->addWidget(buttonStopTunnel_);
    tunnelForm->addRow(QString(), tunnelButtonRow);
    connect(buttonStartTunnel_, &QPushButton::clicked, this, &AdvancedSettingsDialog::OnStartTunnelClicked);
    connect(buttonStopTunnel_, &QPushButton::clicked, this, &AdvancedSettingsDialog::OnStopTunnelClicked);

    labelTunnelStatus_ = new QLabel(tunnelGroup);
    labelTunnelStatus_->setObjectName(QStringLiteral("hintLabel"));
    tunnelForm->addRow(QString(), labelTunnelStatus_);

    layout->addWidget(tunnelGroup);

    // --- Scheduled restart group --------------------------------------
    auto* restartGroup = new QGroupBox(QStringLiteral("Tự động Restart định kỳ"), this);
    auto* restartForm = new QFormLayout(restartGroup);

    checkScheduledRestartEnabled_ = new QCheckBox(QStringLiteral("Bật restart định kỳ"), restartGroup);
    restartForm->addRow(QString(), checkScheduledRestartEnabled_);

    spinRestartIntervalHours_ = new QSpinBox(restartGroup);
    spinRestartIntervalHours_->setRange(1, 168);
    spinRestartIntervalHours_->setSuffix(QStringLiteral(" giờ"));
    restartForm->addRow(QStringLiteral("Chu kỳ:"), spinRestartIntervalHours_);

    auto* restartHint = new QLabel(
        QStringLiteral("Sẽ gửi \"say [Manager] Server sẽ tự khởi động lại sau 60 giây.\" vào chat 60 giây trước khi restart."),
        restartGroup);
    restartHint->setObjectName(QStringLiteral("hintLabel"));
    restartHint->setWordWrap(true);
    restartForm->addRow(QString(), restartHint);

    layout->addWidget(restartGroup);

    labelSaveStatus_ = new QLabel(this);
    labelSaveStatus_->setObjectName(QStringLiteral("hintLabel"));
    layout->addWidget(labelSaveStatus_);

    auto* buttons = new QDialogButtonBox(this);
    auto* saveButton = buttons->addButton(QStringLiteral("Lưu"), QDialogButtonBox::AcceptRole);
    saveButton->setObjectName(QStringLiteral("btnStart"));
    auto* closeButton = buttons->addButton(QStringLiteral("Đóng"), QDialogButtonBox::RejectRole);
    closeButton->setObjectName(QStringLiteral("btnNeutral"));
    connect(saveButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::OnSaveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(comboPreset_, &QComboBox::currentIndexChanged, this, &AdvancedSettingsDialog::OnPresetChanged);

    // Load current values.
    const auto& config = server_->GetConfig();
    editTunnelExecutable_->setText(QString::fromStdWString(config.tunnel.executablePath.wstring()));
    editTunnelArguments_->setText(QString::fromStdWString(config.tunnel.arguments));
    checkTunnelEnabled_->setChecked(config.tunnel.enabled);
    checkTunnelAutoStart_->setChecked(config.tunnel.autoStartWithServer);
    checkScheduledRestartEnabled_->setChecked(config.scheduledRestart.enabled);
    spinRestartIntervalHours_->setValue(config.scheduledRestart.intervalHours > 0 ? config.scheduledRestart.intervalHours : 24);

    OnPresetChanged();
    RefreshTunnelStatus();
}

void AdvancedSettingsDialog::OnPresetChanged()
{
    switch (comboPreset_->currentIndex())
    {
        case 1: // Playit.gg
            labelPresetHint_->setText(
                QStringLiteral("Tải playit.exe từ playit.gg, chọn file đó ở dưới. Không cần tham số. "
                                "Lần đầu chạy sẽ in ra link để liên kết tài khoản trong console."));
            break;
        case 2: // Cloudflare Tunnel
            labelPresetHint_->setText(
                QStringLiteral("Cần đã tạo Tunnel + route DNS trên Cloudflare Dashboard trước. "
                                "Tham số ví dụ: tunnel run <tên-tunnel-của-bạn>"));
            break;
        default:
            labelPresetHint_->setText(QStringLiteral("Chọn 1 file thực thi bất kỳ và tham số dòng lệnh tương ứng."));
            break;
    }
}

void AdvancedSettingsDialog::OnBrowseTunnelExecutable()
{
    const QString chosen = QFileDialog::getOpenFileName(
        this, QStringLiteral("Chọn file thực thi tunnel"), QString(), QStringLiteral("Executable (*.exe)"));
    if (!chosen.isEmpty())
    {
        editTunnelExecutable_->setText(chosen);
    }
}

void AdvancedSettingsDialog::RefreshTunnelStatus()
{
    labelTunnelStatus_->setText(server_->IsTunnelRunning()
        ? QStringLiteral("Trạng thái: đang chạy")
        : QStringLiteral("Trạng thái: đã dừng"));
}

void AdvancedSettingsDialog::OnStartTunnelClicked()
{
    auto srv = server_;
    std::thread([srv]() { srv->StartTunnel(); }).detach();
    labelTunnelStatus_->setText(QStringLiteral("Đang khởi động..."));
}

void AdvancedSettingsDialog::OnStopTunnelClicked()
{
    auto srv = server_;
    std::thread([srv]() { srv->StopTunnel(); }).detach();
    labelTunnelStatus_->setText(QStringLiteral("Đang dừng..."));
}

void AdvancedSettingsDialog::OnSaveClicked()
{
    core::ServerConfig config = server_->GetConfig();
    config.tunnel.executablePath = editTunnelExecutable_->text().toStdWString();
    config.tunnel.arguments = editTunnelArguments_->text().toStdWString();
    config.tunnel.enabled = checkTunnelEnabled_->isChecked();
    config.tunnel.autoStartWithServer = checkTunnelAutoStart_->isChecked();
    config.scheduledRestart.enabled = checkScheduledRestartEnabled_->isChecked();
    config.scheduledRestart.intervalHours = spinRestartIntervalHours_->value();

    server_->UpdateConfig(config);
    serverManager_->SaveAll(*configManager_);

    labelSaveStatus_->setText(QStringLiteral("Đã lưu."));
}

} // namespace ui
