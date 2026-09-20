#pragma once

// Phase 6 (tunnel companion process) + Fork-inspired scheduled restart,
// combined into one "advanced settings" dialog since both are optional,
// per-server, set-once-and-forget settings edited after a server already
// exists (unlike AddServerDialog's fields, which matter at creation time).

#include "config/ConfigManager.hpp"
#include "server/ServerManager.hpp"

#include <QDialog>

#include <memory>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QLabel;

namespace ui
{

class AdvancedSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    AdvancedSettingsDialog(
        std::shared_ptr<server::MinecraftServer> server,
        std::shared_ptr<server::ServerManager> serverManager,
        std::shared_ptr<config::ConfigManager> configManager,
        QWidget* parent = nullptr);

private slots:
    void OnPresetChanged();
    void OnBrowseTunnelExecutable();
    void OnStartTunnelClicked();
    void OnStopTunnelClicked();
    void OnSaveClicked();

private:
    void RefreshTunnelStatus();

    std::shared_ptr<server::MinecraftServer> server_;
    std::shared_ptr<server::ServerManager> serverManager_;
    std::shared_ptr<config::ConfigManager> configManager_;

    QComboBox* comboPreset_ = nullptr;
    QLineEdit* editTunnelExecutable_ = nullptr;
    QLineEdit* editTunnelArguments_ = nullptr;
    QCheckBox* checkTunnelEnabled_ = nullptr;
    QCheckBox* checkTunnelAutoStart_ = nullptr;
    QPushButton* buttonStartTunnel_ = nullptr;
    QPushButton* buttonStopTunnel_ = nullptr;
    QLabel* labelTunnelStatus_ = nullptr;
    QLabel* labelPresetHint_ = nullptr;

    QCheckBox* checkScheduledRestartEnabled_ = nullptr;
    QSpinBox* spinRestartIntervalHours_ = nullptr;

    QLabel* labelSaveStatus_ = nullptr;
};

} // namespace ui
