#pragma once

// A modal dialog for entering a new server profile's fields, built on
// Qt's QDialog. Call exec(); if it returns QDialog::Accepted, GetResult()
// holds the new core::ServerConfig (id is left empty - ServerManager::
// AddServer() assigns it).

#include "core/Types.hpp"

#include <QDialog>

#include <optional>

class QLineEdit;
class QLabel;

namespace ui
{

class AddServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddServerDialog(QWidget* parent = nullptr);

    [[nodiscard]] std::optional<core::ServerConfig> GetResult() const { return result_; }

private slots:
    void OnSaveClicked();

private:
    QLineEdit* editName_ = nullptr;
    QLineEdit* editDirectory_ = nullptr;
    QLineEdit* editJar_ = nullptr;
    QLineEdit* editJavaPath_ = nullptr;
    QLineEdit* editMinMemory_ = nullptr;
    QLineEdit* editMaxMemory_ = nullptr;
    QLineEdit* editPort_ = nullptr;
    QLabel* labelError_ = nullptr;

    std::optional<core::ServerConfig> result_;
};

} // namespace ui
