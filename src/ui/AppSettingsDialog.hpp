#pragma once

// Global (not per-server) app settings: whether the app launches when
// Windows starts, and in which mode (Normal = window visible, Silent =
// tray icon only). Backed by platform::StartupManager (a per-user
// Registry Run key, no admin rights needed).

#include <QDialog>

class QCheckBox;
class QLabel;

namespace ui
{

class AppSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppSettingsDialog(QWidget* parent = nullptr);

private slots:
    void OnSaveClicked();

private:
    QCheckBox* checkStartWithWindows_ = nullptr;
    QCheckBox* checkSilent_ = nullptr;
    QLabel* labelStatus_ = nullptr;
};

} // namespace ui
