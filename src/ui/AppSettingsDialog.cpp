#include "ui/AppSettingsDialog.hpp"

#include "platform/StartupManager.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui
{

AppSettingsDialog::AppSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Cài đặt ứng dụng"));
    resize(420, 260);

    auto* layout = new QVBoxLayout(this);

    checkStartWithWindows_ = new QCheckBox(QStringLiteral("Khởi động cùng Windows"), this);
    checkStartWithWindows_->setChecked(platform::IsStartOnBootEnabled());
    layout->addWidget(checkStartWithWindows_);

    checkSilent_ = new QCheckBox(QStringLiteral("Chạy chế độ Silent (ẩn cửa sổ, chỉ hiện icon khay hệ thống)"), this);
    checkSilent_->setChecked(true);
    layout->addWidget(checkSilent_);

    auto* hint = new QLabel(
        QStringLiteral(
            "• Normal: mở cửa sổ bình thường như khi bạn tự mở app.\n"
            "• Silent: chạy ẩn xuống khay hệ thống (system tray), không hiện cửa sổ - phù hợp khi để "
            "máy tự chạy server nền mà không muốn cửa sổ bật lên mỗi lần khởi động máy.\n\n"
            "Dù chọn chế độ nào, các server có đánh dấu \"Tự động Start server này khi mở app\" "
            "(trong 🌐 Tunnel & Nâng cao của từng server) vẫn sẽ tự khởi động."),
        this);
    hint->setObjectName(QStringLiteral("hintLabel"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    labelStatus_ = new QLabel(this);
    labelStatus_->setObjectName(QStringLiteral("hintLabel"));
    layout->addWidget(labelStatus_);
    layout->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    auto* saveButton = buttons->addButton(QStringLiteral("Lưu"), QDialogButtonBox::AcceptRole);
    saveButton->setObjectName(QStringLiteral("btnStart"));
    auto* closeButton = buttons->addButton(QStringLiteral("Đóng"), QDialogButtonBox::RejectRole);
    closeButton->setObjectName(QStringLiteral("btnNeutral"));
    connect(saveButton, &QPushButton::clicked, this, &AppSettingsDialog::OnSaveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void AppSettingsDialog::OnSaveClicked()
{
    const bool ok = platform::SetStartOnBoot(checkStartWithWindows_->isChecked(), checkSilent_->isChecked());
    labelStatus_->setText(ok
        ? QStringLiteral("Đã lưu.")
        : QStringLiteral("Lỗi: không ghi được vào Registry."));
}

} // namespace ui
