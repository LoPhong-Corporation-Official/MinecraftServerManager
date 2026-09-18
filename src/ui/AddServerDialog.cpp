#include "ui/AddServerDialog.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui
{
namespace
{

// Wraps a QLineEdit and a small "..." browse button into one row widget,
// so QFormLayout::addRow() still only sees a single field per row.
QWidget* MakeBrowsableRow(QWidget* parent, QLineEdit* field, QPushButton*& outBrowseButton)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    field->setParent(row);
    layout->addWidget(field, 1);
    outBrowseButton = new QPushButton(QStringLiteral("..."), row);
    outBrowseButton->setObjectName(QStringLiteral("btnNeutral"));
    outBrowseButton->setFixedWidth(36);
    layout->addWidget(outBrowseButton);
    return row;
}

} // namespace

AddServerDialog::AddServerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Thêm Server Mới"));
    setModal(true);

    auto* form = new QFormLayout();

    editName_ = new QLineEdit(this);
    form->addRow(QStringLiteral("Tên server:"), editName_);

    editDirectory_ = new QLineEdit(this);
    QPushButton* browseDirectoryButton = nullptr;
    form->addRow(QStringLiteral("Thư mục server:"), MakeBrowsableRow(this, editDirectory_, browseDirectoryButton));
    connect(browseDirectoryButton, &QPushButton::clicked, this, &AddServerDialog::OnBrowseDirectory);

    editJar_ = new QLineEdit(this);
    editJar_->setText(QStringLiteral("server.jar"));
    QPushButton* browseJarButton = nullptr;
    form->addRow(QStringLiteral("Tên file jar:"), MakeBrowsableRow(this, editJar_, browseJarButton));
    connect(browseJarButton, &QPushButton::clicked, this, &AddServerDialog::OnBrowseJar);

    editJavaPath_ = new QLineEdit(this);
    editJavaPath_->setText(QStringLiteral("java.exe"));
    QPushButton* browseJavaButton = nullptr;
    form->addRow(QStringLiteral("Đường dẫn java.exe:"), MakeBrowsableRow(this, editJavaPath_, browseJavaButton));
    connect(browseJavaButton, &QPushButton::clicked, this, &AddServerDialog::OnBrowseJavaPath);

    comboServerType_ = new QComboBox(this);
    comboServerType_->addItems({
        QStringLiteral("Vanilla"),
        QStringLiteral("Paper"),
        QStringLiteral("Spigot"),
        QStringLiteral("Purpur"),
        QStringLiteral("Folia"),
        QStringLiteral("Fabric"),
        QStringLiteral("Forge"),
        QStringLiteral("Bukkit"),
        QStringLiteral("Khác"),
    });
    form->addRow(QStringLiteral("Loại server:"), comboServerType_);

    auto* tpsHint = new QLabel(
        QStringLiteral("Chỉ Paper/Spigot/Purpur/Bukkit/Folia mới tự động hiện TPS (server tự trả lời lệnh /tps)."),
        this);
    tpsHint->setObjectName(QStringLiteral("hintLabel"));
    tpsHint->setWordWrap(true);
    form->addRow(QString(), tpsHint);

    editMinMemory_ = new QLineEdit(this);
    editMinMemory_->setText(QStringLiteral("1024"));
    editMinMemory_->setValidator(new QIntValidator(1, 1000000, this));
    form->addRow(QStringLiteral("RAM tối thiểu (MB):"), editMinMemory_);

    editMaxMemory_ = new QLineEdit(this);
    editMaxMemory_->setText(QStringLiteral("2048"));
    editMaxMemory_->setValidator(new QIntValidator(1, 1000000, this));
    form->addRow(QStringLiteral("RAM tối đa (MB):"), editMaxMemory_);

    editPort_ = new QLineEdit(this);
    editPort_->setText(QStringLiteral("25565"));
    editPort_->setValidator(new QIntValidator(1, 65535, this));
    form->addRow(QStringLiteral("Port:"), editPort_);

    labelError_ = new QLabel(this);
    labelError_->setStyleSheet(QStringLiteral("color: #d9534f;"));
    labelError_->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("btnStart"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("btnNeutral"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &AddServerDialog::OnSaveClicked);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(labelError_);
    layout->addWidget(buttons);

    resize(480, 400);
}

void AddServerDialog::OnBrowseDirectory()
{
    const QString startDir = editDirectory_->text().isEmpty() ? QDir::homePath() : editDirectory_->text();
    const QString chosen = QFileDialog::getExistingDirectory(this, QStringLiteral("Chọn thư mục server"), startDir);
    if (!chosen.isEmpty())
    {
        editDirectory_->setText(QDir::toNativeSeparators(chosen));
    }
}

void AddServerDialog::OnBrowseJar()
{
    const QString startDir = editDirectory_->text().isEmpty() ? QDir::homePath() : editDirectory_->text();
    const QString chosen = QFileDialog::getOpenFileName(
        this, QStringLiteral("Chọn file server .jar"), startDir, QStringLiteral("Java Archive (*.jar)"));
    if (chosen.isEmpty())
    {
        return;
    }

    const QFileInfo info(chosen);
    // We only ever store the *filename* here, because MinecraftServer
    // launches java with this as a relative path inside the server's
    // working directory (see MinecraftServer::Start()).
    editJar_->setText(info.fileName());

    // Convenience: if the directory field is still empty, infer it from
    // wherever the chosen jar actually lives.
    if (editDirectory_->text().isEmpty())
    {
        editDirectory_->setText(QDir::toNativeSeparators(info.absolutePath()));
    }
}

void AddServerDialog::OnBrowseJavaPath()
{
    const QString chosen = QFileDialog::getOpenFileName(
        this, QStringLiteral("Chọn java.exe"), QString(), QStringLiteral("java.exe;;Executable (*.exe)"));
    if (!chosen.isEmpty())
    {
        editJavaPath_->setText(QDir::toNativeSeparators(chosen));
    }
}

void AddServerDialog::OnSaveClicked()
{
    const QString name = editName_->text().trimmed();
    const QString directory = editDirectory_->text().trimmed();
    const QString jar = editJar_->text().trimmed();
    const QString javaPath = editJavaPath_->text().trimmed();

    if (name.isEmpty() || directory.isEmpty() || jar.isEmpty() || javaPath.isEmpty())
    {
        labelError_->setText(QStringLiteral("Vui lòng điền đầy đủ Tên, Thư mục, Jar và đường dẫn Java."));
        return;
    }

    bool minOk = false;
    bool maxOk = false;
    bool portOk = false;
    const unsigned long minMemory = editMinMemory_->text().toULong(&minOk);
    const unsigned long maxMemory = editMaxMemory_->text().toULong(&maxOk);
    const unsigned long port = editPort_->text().toULong(&portOk);

    if (!minOk || !maxOk)
    {
        labelError_->setText(QStringLiteral("RAM tối thiểu/tối đa phải là số nguyên (MB)."));
        return;
    }
    if (!portOk || port == 0 || port > 65535)
    {
        labelError_->setText(QStringLiteral("Port phải là số từ 1 đến 65535."));
        return;
    }
    if (minMemory > maxMemory)
    {
        labelError_->setText(QStringLiteral("RAM tối thiểu không được lớn hơn RAM tối đa."));
        return;
    }

    core::ServerConfig config;
    config.name = name.toStdWString();
    config.directory = directory.toStdWString();
    config.serverJar = jar.toStdWString();
    config.javaExecutable = javaPath.toStdWString();
    config.serverType = comboServerType_->currentText().toStdWString();
    config.minMemoryMB = minMemory;
    config.maxMemoryMB = maxMemory;
    config.port = static_cast<std::uint16_t>(port);

    result_ = config;
    accept();
}

} // namespace ui
