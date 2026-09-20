#pragma once

// Phase 3: a friendly editor for the most commonly-tweaked
// server.properties settings, instead of making the user hand-edit a
// text file. Anything not listed here (the file has ~50 possible keys)
// is left completely untouched on save - see config::WriteServerProperties.

#include "core/Types.hpp"

#include <QDialog>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLabel;

namespace ui
{

class PropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    PropertiesDialog(core::ServerConfig server, QWidget* parent = nullptr);

private slots:
    void OnSaveClicked();
    void OnReloadClicked();

private:
    void LoadFromFile();

    core::ServerConfig server_;

    QLineEdit* editMotd_ = nullptr;
    QSpinBox* spinMaxPlayers_ = nullptr;
    QComboBox* comboDifficulty_ = nullptr;
    QComboBox* comboGamemode_ = nullptr;
    QCheckBox* checkPvp_ = nullptr;
    QCheckBox* checkOnlineMode_ = nullptr;
    QCheckBox* checkWhitelist_ = nullptr;
    QSpinBox* spinViewDistance_ = nullptr;
    QSpinBox* spinSpawnProtection_ = nullptr;
    QLineEdit* editLevelSeed_ = nullptr;
    QLineEdit* editLevelName_ = nullptr;
    QLabel* labelStatus_ = nullptr;
};

} // namespace ui
