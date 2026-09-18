#include "ui/LibraryDialog.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui
{

LibraryDialog::LibraryDialog(core::ServerConfig targetServer, QWidget* parent)
    : QDialog(parent)
    , targetServer_(std::move(targetServer))
    , client_(new net::ModrinthClient(this))
{
    setWindowTitle(QStringLiteral("Thư viện Mod / Plugin / Modpack (Modrinth)"));
    resize(760, 580);

    auto* layout = new QVBoxLayout(this);

    auto* targetLabel = new QLabel(
        QStringLiteral("Cài vào server: <b>%1</b>").arg(QString::fromStdWString(targetServer_.name)),
        this);
    layout->addWidget(targetLabel);

    auto* searchRow = new QHBoxLayout();
    comboType_ = new QComboBox(this);
    comboType_->addItem(QStringLiteral("Mod"), QStringLiteral("mod"));
    comboType_->addItem(QStringLiteral("Plugin"), QStringLiteral("plugin"));
    comboType_->addItem(QStringLiteral("Modpack"), QStringLiteral("modpack"));
    comboType_->addItem(QStringLiteral("Resource Pack"), QStringLiteral("resourcepack"));
    comboType_->addItem(QStringLiteral("Shader"), QStringLiteral("shader"));
    searchRow->addWidget(comboType_);

    editQuery_ = new QLineEdit(this);
    editQuery_->setPlaceholderText(QStringLiteral("Tìm trên Modrinth... (vd: sodium, worldedit, terralith)"));
    searchRow->addWidget(editQuery_, 1);

    buttonSearch_ = new QPushButton(QStringLiteral("Tìm kiếm"), this);
    buttonSearch_->setObjectName(QStringLiteral("btnSend"));
    searchRow->addWidget(buttonSearch_);
    layout->addLayout(searchRow);

    listResults_ = new QListWidget(this);
    layout->addWidget(listResults_, 1);

    labelDetail_ = new QLabel(this);
    labelDetail_->setWordWrap(true);
    labelDetail_->setObjectName(QStringLiteral("hintLabel"));
    labelDetail_->setMinimumHeight(36);
    layout->addWidget(labelDetail_);

    auto* bottomRow = new QHBoxLayout();
    labelStatus_ = new QLabel(this);
    labelStatus_->setObjectName(QStringLiteral("hintLabel"));
    bottomRow->addWidget(labelStatus_, 1);
    buttonInstall_ = new QPushButton(QStringLiteral("⬇  Cài vào server này"), this);
    buttonInstall_->setObjectName(QStringLiteral("btnStart"));
    buttonInstall_->setEnabled(false);
    bottomRow->addWidget(buttonInstall_);
    layout->addLayout(bottomRow);

    connect(buttonSearch_, &QPushButton::clicked, this, &LibraryDialog::OnSearchClicked);
    connect(editQuery_, &QLineEdit::returnPressed, this, &LibraryDialog::OnSearchClicked);
    connect(listResults_, &QListWidget::itemSelectionChanged, this, &LibraryDialog::OnResultSelectionChanged);
    connect(buttonInstall_, &QPushButton::clicked, this, &LibraryDialog::OnInstallClicked);
}

QString LibraryDialog::FacetForCurrentType() const
{
    return comboType_->currentData().toString();
}

QString LibraryDialog::SubfolderForProjectType(const QString& projectType) const
{
    if (projectType == QStringLiteral("mod")) return QStringLiteral("mods");
    if (projectType == QStringLiteral("plugin")) return QStringLiteral("plugins");
    if (projectType == QStringLiteral("resourcepack")) return QStringLiteral("resourcepacks");
    if (projectType == QStringLiteral("shader")) return QStringLiteral("shaderpacks");
    return QString(); // modpack (and anything unrecognised): drop in the server root
}

void LibraryDialog::OnSearchClicked()
{
    const QString query = editQuery_->text().trimmed();
    const QString facet = FacetForCurrentType();

    buttonSearch_->setEnabled(false);
    labelStatus_->setText(QStringLiteral("Đang tìm kiếm..."));
    listResults_->clear();
    currentHits_.clear();
    buttonInstall_->setEnabled(false);
    labelDetail_->clear();

    client_->Search(query, facet, [this](QList<net::ModrinthProjectHit> hits, QString error) {
        buttonSearch_->setEnabled(true);
        if (!error.isEmpty())
        {
            labelStatus_->setText(QStringLiteral("Lỗi: %1").arg(error));
            return;
        }
        PopulateResults(hits);
        labelStatus_->setText(hits.isEmpty()
            ? QStringLiteral("Không tìm thấy kết quả nào.")
            : QStringLiteral("Tìm thấy %1 kết quả.").arg(hits.size()));
    });
}

void LibraryDialog::PopulateResults(const QList<net::ModrinthProjectHit>& hits)
{
    currentHits_ = hits;
    listResults_->clear();
    for (const auto& hit : currentHits_)
    {
        const QString label = QStringLiteral("%1  —  %2  ·  %3 lượt tải")
            .arg(hit.title, hit.author, QString::number(hit.downloads));
        auto* item = new QListWidgetItem(label, listResults_);
        item->setData(Qt::UserRole, hit.projectId);
    }
}

std::optional<net::ModrinthProjectHit> LibraryDialog::CurrentSelection() const
{
    const auto* item = listResults_->currentItem();
    if (item == nullptr)
    {
        return std::nullopt;
    }
    const QString projectId = item->data(Qt::UserRole).toString();
    for (const auto& hit : currentHits_)
    {
        if (hit.projectId == projectId)
        {
            return hit;
        }
    }
    return std::nullopt;
}

void LibraryDialog::OnResultSelectionChanged()
{
    auto selected = CurrentSelection();
    if (!selected.has_value())
    {
        labelDetail_->clear();
        buttonInstall_->setEnabled(false);
        return;
    }
    labelDetail_->setText(selected->description);
    buttonInstall_->setEnabled(true);
}

void LibraryDialog::OnInstallClicked()
{
    auto selected = CurrentSelection();
    if (!selected.has_value())
    {
        return;
    }

    buttonInstall_->setEnabled(false);
    labelStatus_->setText(QStringLiteral("Đang lấy thông tin bản build mới nhất..."));

    const net::ModrinthProjectHit hit = *selected;
    client_->GetLatestPrimaryFile(hit.projectId, [this, hit](std::optional<net::ModrinthVersionFile> file, QString error) {
        if (!file.has_value())
        {
            labelStatus_->setText(QStringLiteral("Lỗi: %1").arg(error));
            buttonInstall_->setEnabled(true);
            return;
        }

        const QString subfolder = SubfolderForProjectType(hit.projectType);
        const QString serverDir = QString::fromStdWString(targetServer_.directory.wstring());
        const QString destinationDir = subfolder.isEmpty() ? serverDir : (serverDir + QStringLiteral("/") + subfolder);
        const QString destinationPath = destinationDir + QStringLiteral("/") + file->filename;

        labelStatus_->setText(QStringLiteral("Đang tải %1...").arg(file->filename));

        client_->DownloadFile(*file, destinationPath, [this, hit, destinationPath](bool success, QString downloadError) {
            buttonInstall_->setEnabled(true);
            if (!success)
            {
                labelStatus_->setText(QStringLiteral("Lỗi tải file: %1").arg(downloadError));
                return;
            }

            if (hit.projectType == QStringLiteral("modpack"))
            {
                labelStatus_->setText(
                    QStringLiteral("Đã tải modpack về %1 — modpack cần cài đặt/giải nén thủ công.").arg(destinationPath));
            }
            else
            {
                labelStatus_->setText(
                    QStringLiteral("Đã cài vào %1. Khởi động lại server để áp dụng.").arg(destinationPath));
            }
        });
    });
}

} // namespace ui
