#include "ui/PropertiesDialog.hpp"

#include "config/ServerProperties.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace ui
{
namespace
{
// Helpers to bridge QString <-> the std::map<wstring,wstring> shape
// config::ReadServerProperties/WriteServerProperties use.
QString Get(const std::map<std::wstring, std::wstring>& props, const wchar_t* key, const QString& fallback)
{
    const auto it = props.find(key);
    return it == props.end() ? fallback : QString::fromStdWString(it->second);
}
bool GetBool(const std::map<std::wstring, std::wstring>& props, const wchar_t* key, bool fallback)
{
    const auto it = props.find(key);
    if (it == props.end())
    {
        return fallback;
    }
    return it->second == L"true";
}
int GetInt(const std::map<std::wstring, std::wstring>& props, const wchar_t* key, int fallback)
{
    const auto it = props.find(key);
    if (it == props.end())
    {
        return fallback;
    }
    bool ok = false;
    const int value = QString::fromStdWString(it->second).toInt(&ok);
    return ok ? value : fallback;
}
} // namespace

PropertiesDialog::PropertiesDialog(core::ServerConfig server, QWidget* parent)
    : QDialog(parent)
    , server_(std::move(server))
{
    setWindowTitle(QStringLiteral("server.properties — %1").arg(QString::fromStdWString(server_.name)));
    resize(460, 480);

    auto* form = new QFormLayout();

    editMotd_ = new QLineEdit(this);
    form->addRow(QStringLiteral("MOTD:"), editMotd_);

    spinMaxPlayers_ = new QSpinBox(this);
    spinMaxPlayers_->setRange(1, 2000);
    form->addRow(QStringLiteral("Số người chơi tối đa:"), spinMaxPlayers_);

    comboDifficulty_ = new QComboBox(this);
    comboDifficulty_->addItems({QStringLiteral("peaceful"), QStringLiteral("easy"), QStringLiteral("normal"), QStringLiteral("hard")});
    form->addRow(QStringLiteral("Độ khó:"), comboDifficulty_);

    comboGamemode_ = new QComboBox(this);
    comboGamemode_->addItems({QStringLiteral("survival"), QStringLiteral("creative"), QStringLiteral("adventure"), QStringLiteral("spectator")});
    form->addRow(QStringLiteral("Chế độ chơi:"), comboGamemode_);

    checkPvp_ = new QCheckBox(QStringLiteral("Cho phép PvP"), this);
    form->addRow(QString(), checkPvp_);

    checkOnlineMode_ = new QCheckBox(QStringLiteral("Online mode (xác thực tài khoản Mojang)"), this);
    form->addRow(QString(), checkOnlineMode_);

    checkWhitelist_ = new QCheckBox(QStringLiteral("Bật whitelist"), this);
    form->addRow(QString(), checkWhitelist_);

    spinViewDistance_ = new QSpinBox(this);
    spinViewDistance_->setRange(2, 32);
    form->addRow(QStringLiteral("View distance (chunk):"), spinViewDistance_);

    spinSpawnProtection_ = new QSpinBox(this);
    spinSpawnProtection_->setRange(0, 500);
    form->addRow(QStringLiteral("Bán kính bảo vệ spawn:"), spinSpawnProtection_);

    editLevelName_ = new QLineEdit(this);
    form->addRow(QStringLiteral("Tên world:"), editLevelName_);

    editLevelSeed_ = new QLineEdit(this);
    editLevelSeed_->setPlaceholderText(QStringLiteral("(để trống = ngẫu nhiên)"));
    form->addRow(QStringLiteral("Seed:"), editLevelSeed_);

    auto* hint = new QLabel(
        QStringLiteral("Chỉ các mục trên được quản lý ở đây; những dòng khác trong server.properties vẫn được giữ nguyên. "
                        "Cần khởi động lại server để áp dụng."),
        this);
    hint->setObjectName(QStringLiteral("hintLabel"));
    hint->setWordWrap(true);

    labelStatus_ = new QLabel(this);
    labelStatus_->setObjectName(QStringLiteral("hintLabel"));

    auto* buttons = new QDialogButtonBox(this);
    auto* reloadButton = buttons->addButton(QStringLiteral("Tải lại"), QDialogButtonBox::ResetRole);
    reloadButton->setObjectName(QStringLiteral("btnNeutral"));
    auto* saveButton = buttons->addButton(QStringLiteral("Lưu"), QDialogButtonBox::AcceptRole);
    saveButton->setObjectName(QStringLiteral("btnStart"));
    auto* closeButton = buttons->addButton(QStringLiteral("Đóng"), QDialogButtonBox::RejectRole);
    closeButton->setObjectName(QStringLiteral("btnNeutral"));
    connect(reloadButton, &QPushButton::clicked, this, &PropertiesDialog::OnReloadClicked);
    connect(saveButton, &QPushButton::clicked, this, &PropertiesDialog::OnSaveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addWidget(labelStatus_);
    layout->addWidget(buttons);

    LoadFromFile();
}

void PropertiesDialog::LoadFromFile()
{
    const auto props = config::ReadServerProperties(server_);
    if (props.empty())
    {
        labelStatus_->setText(
            QStringLiteral("Chưa tìm thấy server.properties — hãy Start server ít nhất 1 lần để Minecraft tự tạo file này."));
    }
    else
    {
        labelStatus_->setText(QString());
    }

    editMotd_->setText(Get(props, L"motd", QStringLiteral("A Minecraft Server")));
    spinMaxPlayers_->setValue(GetInt(props, L"max-players", 20));
    comboDifficulty_->setCurrentText(Get(props, L"difficulty", QStringLiteral("easy")));
    comboGamemode_->setCurrentText(Get(props, L"gamemode", QStringLiteral("survival")));
    checkPvp_->setChecked(GetBool(props, L"pvp", true));
    checkOnlineMode_->setChecked(GetBool(props, L"online-mode", true));
    checkWhitelist_->setChecked(GetBool(props, L"white-list", false));
    spinViewDistance_->setValue(GetInt(props, L"view-distance", 10));
    spinSpawnProtection_->setValue(GetInt(props, L"spawn-protection", 16));
    editLevelName_->setText(Get(props, L"level-name", QStringLiteral("world")));
    editLevelSeed_->setText(Get(props, L"level-seed", QString()));
}

void PropertiesDialog::OnReloadClicked()
{
    LoadFromFile();
}

void PropertiesDialog::OnSaveClicked()
{
    std::map<std::wstring, std::wstring> updates;
    updates[L"motd"] = editMotd_->text().toStdWString();
    updates[L"max-players"] = QString::number(spinMaxPlayers_->value()).toStdWString();
    updates[L"difficulty"] = comboDifficulty_->currentText().toStdWString();
    updates[L"gamemode"] = comboGamemode_->currentText().toStdWString();
    updates[L"pvp"] = checkPvp_->isChecked() ? L"true" : L"false";
    updates[L"online-mode"] = checkOnlineMode_->isChecked() ? L"true" : L"false";
    updates[L"white-list"] = checkWhitelist_->isChecked() ? L"true" : L"false";
    updates[L"view-distance"] = QString::number(spinViewDistance_->value()).toStdWString();
    updates[L"spawn-protection"] = QString::number(spinSpawnProtection_->value()).toStdWString();
    updates[L"level-name"] = editLevelName_->text().toStdWString();
    if (!editLevelSeed_->text().isEmpty())
    {
        updates[L"level-seed"] = editLevelSeed_->text().toStdWString();
    }

    if (config::WriteServerProperties(server_, updates))
    {
        labelStatus_->setText(QStringLiteral("Đã lưu. Khởi động lại server để áp dụng."));
    }
    else
    {
        labelStatus_->setText(QStringLiteral("Lỗi: không ghi được server.properties."));
    }
}

} // namespace ui
