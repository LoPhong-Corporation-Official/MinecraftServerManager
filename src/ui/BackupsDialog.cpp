#include "ui/BackupsDialog.hpp"

#include "backup/BackupManager.hpp"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include <filesystem>

namespace ui
{
namespace
{
QString FormatSize(std::uint64_t bytes)
{
    constexpr double kKB = 1024.0;
    constexpr double kMB = kKB * 1024.0;
    constexpr double kGB = kMB * 1024.0;
    const auto b = static_cast<double>(bytes);
    if (b >= kGB) return QStringLiteral("%1 GB").arg(QString::number(b / kGB, 'f', 2));
    if (b >= kMB) return QStringLiteral("%1 MB").arg(QString::number(b / kMB, 'f', 1));
    if (b >= kKB) return QStringLiteral("%1 KB").arg(QString::number(b / kKB, 'f', 1));
    return QStringLiteral("%1 B").arg(bytes);
}
} // namespace

BackupsDialog::BackupsDialog(core::ServerConfig server, QWidget* parent)
    : QDialog(parent)
    , server_(std::move(server))
{
    setWindowTitle(QStringLiteral("Backups — %1").arg(QString::fromStdWString(server_.name)));
    resize(560, 460);

    auto* layout = new QVBoxLayout(this);

    auto* hint = new QLabel(
        QStringLiteral("Sao lưu world (world/world_nether/world_the_end) + server.properties thành file .zip. "
                        "Tự động giữ lại %1 backup gần nhất, các bản cũ hơn sẽ bị xoá.")
            .arg(backup::kMaxBackupsToKeep),
        this);
    hint->setObjectName(QStringLiteral("hintLabel"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    listBackups_ = new QListWidget(this);
    layout->addWidget(listBackups_, 1);

    auto* buttonRow = new QHBoxLayout();
    buttonCreate_ = new QPushButton(QStringLiteral("+ Tạo backup mới"), this);
    buttonCreate_->setObjectName(QStringLiteral("btnStart"));
    buttonDelete_ = new QPushButton(QStringLiteral("Xoá"), this);
    buttonDelete_->setObjectName(QStringLiteral("btnStop"));
    buttonDelete_->setEnabled(false);
    buttonOpenFolder_ = new QPushButton(QStringLiteral("📂 Mở thư mục backups"), this);
    buttonOpenFolder_->setObjectName(QStringLiteral("btnNeutral"));
    buttonRow->addWidget(buttonCreate_);
    buttonRow->addWidget(buttonDelete_);
    buttonRow->addWidget(buttonOpenFolder_);
    buttonRow->addStretch(1);
    layout->addLayout(buttonRow);

    labelStatus_ = new QLabel(this);
    labelStatus_->setObjectName(QStringLiteral("hintLabel"));
    labelStatus_->setWordWrap(true);
    layout->addWidget(labelStatus_);

    connect(buttonCreate_, &QPushButton::clicked, this, &BackupsDialog::OnCreateClicked);
    connect(buttonDelete_, &QPushButton::clicked, this, &BackupsDialog::OnDeleteClicked);
    connect(buttonOpenFolder_, &QPushButton::clicked, this, &BackupsDialog::OnOpenFolderClicked);
    connect(listBackups_, &QListWidget::itemSelectionChanged, this, &BackupsDialog::OnSelectionChanged);

    Refresh();
}

BackupsDialog::~BackupsDialog()
{
    // workerThread_'s own std::jthread destructor requests a stop and
    // joins automatically - but since our lambda doesn't poll a
    // stop_token (there's no safe way to interrupt mid-zip anyway), this
    // just blocks here until any in-flight backup finishes. That is the
    // point: it guarantees the background thread cannot still be calling
    // QMetaObject::invokeMethod(this, ...) after this dialog's widgets
    // start being destroyed.
}

void BackupsDialog::Refresh()
{
    listBackups_->clear();
    for (const auto& info : backup::ListBackups(server_))
    {
        const QString label = QStringLiteral("%1   ·   %2")
            .arg(QString::fromStdWString(info.name), FormatSize(info.sizeBytes));
        auto* item = new QListWidgetItem(label, listBackups_);
        item->setData(Qt::UserRole, QString::fromStdWString(info.name));
    }
    buttonDelete_->setEnabled(false);
}

void BackupsDialog::OnSelectionChanged()
{
    buttonDelete_->setEnabled(listBackups_->currentItem() != nullptr);
}

void BackupsDialog::OnCreateClicked()
{
    buttonCreate_->setEnabled(false);
    labelStatus_->setText(QStringLiteral("Đang tạo backup... (có thể mất một lúc nếu world lớn)"));

    const core::ServerConfig configCopy = server_;
    workerThread_ = std::jthread([this, configCopy](std::stop_token) {
        auto result = backup::CreateBackup(configCopy);
        QMetaObject::invokeMethod(
            this,
            [this, result]() {
                buttonCreate_->setEnabled(true);
                if (result.has_value())
                {
                    labelStatus_->setText(QStringLiteral("Đã tạo: %1").arg(QString::fromStdWString(*result)));
                }
                else
                {
                    labelStatus_->setText(
                        QStringLiteral("Không tạo được backup — server có world chưa (đã Start ít nhất 1 lần chưa)?"));
                }
                Refresh();
            },
            Qt::QueuedConnection);
    });
}

void BackupsDialog::OnDeleteClicked()
{
    auto* item = listBackups_->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString name = item->data(Qt::UserRole).toString();

    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("Xác nhận"),
        QStringLiteral("Xoá backup \"%1\"? Không thể hoàn tác.").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    if (backup::DeleteBackup(server_, name.toStdWString()))
    {
        labelStatus_->setText(QStringLiteral("Đã xoá %1.").arg(name));
    }
    else
    {
        labelStatus_->setText(QStringLiteral("Không xoá được %1.").arg(name));
    }
    Refresh();
}

void BackupsDialog::OnOpenFolderClicked()
{
    const auto dir = backup::BackupsDir(server_);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdWString(dir.wstring())));
}

} // namespace ui
