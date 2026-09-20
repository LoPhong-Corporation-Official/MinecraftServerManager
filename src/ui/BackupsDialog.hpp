#pragma once

// Phase 4: create/list/delete world backups for one server. Restore is
// deliberately not offered here yet - see backup::BackupManager's header
// comment for why. "Mở thư mục backups" lets the user restore manually
// via Windows Explorer in the meantime.

#include "core/Types.hpp"

#include <QDialog>
#include <QList>

#include <thread>

class QListWidget;
class QPushButton;
class QLabel;

namespace ui
{

class BackupsDialog : public QDialog
{
    Q_OBJECT

public:
    BackupsDialog(core::ServerConfig server, QWidget* parent = nullptr);
    ~BackupsDialog() override;

private slots:
    void OnCreateClicked();
    void OnDeleteClicked();
    void OnOpenFolderClicked();
    void OnSelectionChanged();

private:
    void Refresh();

    core::ServerConfig server_;

    QListWidget* listBackups_ = nullptr;
    QPushButton* buttonCreate_ = nullptr;
    QPushButton* buttonDelete_ = nullptr;
    QPushButton* buttonOpenFolder_ = nullptr;
    QLabel* labelStatus_ = nullptr;

    // Backup creation streams a whole world folder through disk I/O, so
    // it runs off the UI thread. Kept as a joinable member (not a
    // detached thread) so the destructor can guarantee it has fully
    // finished - including its QMetaObject::invokeMethod call back to
    // this dialog's UI-thread members - before any of those members
    // start being torn down. See the class's .cpp for the full reasoning.
    std::jthread workerThread_;
};

} // namespace ui
