#pragma once

// Modrinth mods/plugins/modpacks/resource-packs/shaders browser (spec
// Phase 5's "Server Providers" idea, scoped down to "browse + install one
// file into the currently selected server"). Full modpack installation
// (resolving the .mrpack index, downloading every dependency, applying
// overrides) is out of scope here - a modpack file is downloaded as-is
// and the user is told they still need to apply it manually.

#include "core/Types.hpp"
#include "net/ModrinthClient.hpp"

#include <QDialog>
#include <QList>

#include <optional>

class QComboBox;
class QLineEdit;
class QPushButton;
class QListWidget;
class QLabel;

namespace ui
{

class LibraryDialog : public QDialog
{
    Q_OBJECT

public:
    LibraryDialog(core::ServerConfig targetServer, QWidget* parent = nullptr);

private slots:
    void OnSearchClicked();
    void OnResultSelectionChanged();
    void OnInstallClicked();

private:
    void PopulateResults(const QList<net::ModrinthProjectHit>& hits);
    [[nodiscard]] QString FacetForCurrentType() const;
    [[nodiscard]] QString SubfolderForProjectType(const QString& projectType) const;
    [[nodiscard]] std::optional<net::ModrinthProjectHit> CurrentSelection() const;

    core::ServerConfig targetServer_;
    net::ModrinthClient* client_;
    QList<net::ModrinthProjectHit> currentHits_;

    QComboBox* comboType_ = nullptr;
    QLineEdit* editQuery_ = nullptr;
    QPushButton* buttonSearch_ = nullptr;
    QListWidget* listResults_ = nullptr;
    QLabel* labelDetail_ = nullptr;
    QPushButton* buttonInstall_ = nullptr;
    QLabel* labelStatus_ = nullptr;
};

} // namespace ui
