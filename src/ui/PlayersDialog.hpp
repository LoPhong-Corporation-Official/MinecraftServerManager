#pragma once

// Phase 3 "Whitelist/OP UI": rather than hand-editing ops.json/
// whitelist.json (which store UUIDs the server itself resolves from
// usernames), this sends the equivalent built-in console commands
// (op/deop/whitelist add/whitelist remove/kick/ban/pardon) to the
// already-running server - the server does the username->UUID lookup
// itself, exactly as if an admin typed the command directly.
//
// Only usable while the server is Running (these are live console
// commands); MainWindow checks this before opening the dialog.

#include <QDialog>

#include <memory>

class QLineEdit;
class QLabel;

namespace server
{
class MinecraftServer;
}

namespace ui
{

class PlayersDialog : public QDialog
{
    Q_OBJECT

public:
    PlayersDialog(std::shared_ptr<server::MinecraftServer> server, QWidget* parent = nullptr);

private slots:
    void OnOpClicked();
    void OnDeopClicked();
    void OnWhitelistAddClicked();
    void OnWhitelistRemoveClicked();
    void OnKickClicked();
    void OnBanClicked();
    void OnPardonClicked();

private:
    void SendPlayerCommand(const QString& commandTemplate);

    std::shared_ptr<server::MinecraftServer> server_;
    QLineEdit* editPlayerName_ = nullptr;
    QLabel* labelStatus_ = nullptr;
};

} // namespace ui
