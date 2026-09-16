#include "ui/AddServerDialog.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui
{

AddServerDialog::AddServerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Thêm Server Mới"));
    setModal(true);

    auto* form = new QFormLayout();

    editName_ = new QLineEdit(this);
    form->addRow(QStringLiteral("Tên server:"), editName_);

    editDirectory_ = new QLineEdit(this);
    form->addRow(QStringLiteral("Thư mục server:"), editDirectory_);

    editJar_ = new QLineEdit(this);
    editJar_->setText(QStringLiteral("server.jar"));
    form->addRow(QStringLiteral("Tên file jar:"), editJar_);

    editJavaPath_ = new QLineEdit(this);
    editJavaPath_->setText(QStringLiteral("java.exe"));
    form->addRow(QStringLiteral("Đường dẫn java.exe:"), editJavaPath_);

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

    resize(440, 340);
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
    config.minMemoryMB = minMemory;
    config.maxMemoryMB = maxMemory;
    config.port = static_cast<std::uint16_t>(port);

    result_ = config;
    accept();
}

} // namespace ui
