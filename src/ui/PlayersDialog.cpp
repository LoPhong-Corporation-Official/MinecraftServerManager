#include "ui/PlayersDialog.hpp"

#include "server/MinecraftServer.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui
{

PlayersDialog::PlayersDialog(std::shared_ptr<server::MinecraftServer> server, QWidget* parent)
    : QDialog(parent)
    , server_(std::move(server))
{
    setWindowTitle(QStringLiteral("Quản lý người chơi — %1").arg(QString::fromStdWString(server_->GetConfig().name)));
    resize(420, 260);

    auto* layout = new QVBoxLayout(this);

    auto* nameLabel = new QLabel(QStringLiteral("Tên người chơi:"), this);
    layout->addWidget(nameLabel);

    editPlayerName_ = new QLineEdit(this);
    editPlayerName_->setPlaceholderText(QStringLiteral("Steve"));
    layout->addWidget(editPlayerName_);

    auto* grid = new QGridLayout();
    grid->setSpacing(10);

    auto* opButton = new QPushButton(QStringLiteral("Op"), this);
    opButton->setObjectName(QStringLiteral("btnStart"));
    auto* deopButton = new QPushButton(QStringLiteral("Deop"), this);
    deopButton->setObjectName(QStringLiteral("btnNeutral"));
    auto* whitelistAddButton = new QPushButton(QStringLiteral("Whitelist add"), this);
    whitelistAddButton->setObjectName(QStringLiteral("btnStart"));
    auto* whitelistRemoveButton = new QPushButton(QStringLiteral("Whitelist remove"), this);
    whitelistRemoveButton->setObjectName(QStringLiteral("btnNeutral"));
    auto* kickButton = new QPushButton(QStringLiteral("Kick"), this);
    kickButton->setObjectName(QStringLiteral("btnRestart"));
    auto* banButton = new QPushButton(QStringLiteral("Ban"), this);
    banButton->setObjectName(QStringLiteral("btnStop"));
    auto* pardonButton = new QPushButton(QStringLiteral("Pardon (unban)"), this);
    pardonButton->setObjectName(QStringLiteral("btnNeutral"));

    grid->addWidget(opButton, 0, 0);
    grid->addWidget(deopButton, 0, 1);
    grid->addWidget(whitelistAddButton, 1, 0);
    grid->addWidget(whitelistRemoveButton, 1, 1);
    grid->addWidget(kickButton, 2, 0);
    grid->addWidget(banButton, 2, 1);
    grid->addWidget(pardonButton, 3, 0);
    layout->addLayout(grid);

    labelStatus_ = new QLabel(this);
    labelStatus_->setObjectName(QStringLiteral("hintLabel"));
    labelStatus_->setWordWrap(true);
    layout->addWidget(labelStatus_);
    layout->addStretch(1);

    connect(opButton, &QPushButton::clicked, this, &PlayersDialog::OnOpClicked);
    connect(deopButton, &QPushButton::clicked, this, &PlayersDialog::OnDeopClicked);
    connect(whitelistAddButton, &QPushButton::clicked, this, &PlayersDialog::OnWhitelistAddClicked);
    connect(whitelistRemoveButton, &QPushButton::clicked, this, &PlayersDialog::OnWhitelistRemoveClicked);
    connect(kickButton, &QPushButton::clicked, this, &PlayersDialog::OnKickClicked);
    connect(banButton, &QPushButton::clicked, this, &PlayersDialog::OnBanClicked);
    connect(pardonButton, &QPushButton::clicked, this, &PlayersDialog::OnPardonClicked);
}

void PlayersDialog::SendPlayerCommand(const QString& commandTemplate)
{
    const QString name = editPlayerName_->text().trimmed();
    if (name.isEmpty())
    {
        labelStatus_->setText(QStringLiteral("Nhập tên người chơi trước đã."));
        return;
    }
    const QString command = commandTemplate.arg(name);
    if (server_->SendCommand(command.toStdWString()))
    {
        labelStatus_->setText(QStringLiteral("Đã gửi: %1  (xem kết quả ở tab Console)").arg(command));
    }
    else
    {
        labelStatus_->setText(QStringLiteral("Không gửi được — server có đang chạy không?"));
    }
}

void PlayersDialog::OnOpClicked() { SendPlayerCommand(QStringLiteral("op %1")); }
void PlayersDialog::OnDeopClicked() { SendPlayerCommand(QStringLiteral("deop %1")); }
void PlayersDialog::OnWhitelistAddClicked() { SendPlayerCommand(QStringLiteral("whitelist add %1")); }
void PlayersDialog::OnWhitelistRemoveClicked() { SendPlayerCommand(QStringLiteral("whitelist remove %1")); }
void PlayersDialog::OnKickClicked() { SendPlayerCommand(QStringLiteral("kick %1")); }
void PlayersDialog::OnBanClicked() { SendPlayerCommand(QStringLiteral("ban %1")); }
void PlayersDialog::OnPardonClicked() { SendPlayerCommand(QStringLiteral("pardon %1")); }

} // namespace ui
