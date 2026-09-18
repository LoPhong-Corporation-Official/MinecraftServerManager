#include "net/ModrinthClient.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace net
{
namespace
{
// Modrinth asks API consumers to identify themselves with a descriptive
// User-Agent so they can reach out if a client misbehaves.
constexpr const char* kUserAgent = "MinecraftServerManager/1.0 (desktop app)";
}

ModrinthClient::ModrinthClient(QObject* parent)
    : QObject(parent)
    , manager_(new QNetworkAccessManager(this))
{
}

void ModrinthClient::Search(
    const QString& query,
    const QString& projectTypeFacet,
    std::function<void(QList<ModrinthProjectHit> hits, QString errorMessage)> onResult)
{
    QUrl url(QStringLiteral("https://api.modrinth.com/v2/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("query"), query);
    urlQuery.addQueryItem(QStringLiteral("facets"), QStringLiteral("[[\"project_type:%1\"]]").arg(projectTypeFacet));
    urlQuery.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
    urlQuery.addQueryItem(QStringLiteral("index"), QStringLiteral("relevance"));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", kUserAgent);

    QNetworkReply* reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, onResult]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            onResult({}, reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject())
        {
            onResult({}, QStringLiteral("Phản hồi không hợp lệ từ Modrinth."));
            return;
        }

        const QJsonArray hitsArray = doc.object().value(QStringLiteral("hits")).toArray();
        QList<ModrinthProjectHit> hits;
        hits.reserve(hitsArray.size());
        for (const auto& item : hitsArray)
        {
            const QJsonObject o = item.toObject();
            ModrinthProjectHit hit;
            hit.projectId = o.value(QStringLiteral("project_id")).toString();
            hit.slug = o.value(QStringLiteral("slug")).toString();
            hit.title = o.value(QStringLiteral("title")).toString();
            hit.description = o.value(QStringLiteral("description")).toString();
            hit.author = o.value(QStringLiteral("author")).toString();
            hit.projectType = o.value(QStringLiteral("project_type")).toString();
            hit.iconUrl = o.value(QStringLiteral("icon_url")).toString();
            hit.downloads = static_cast<qint64>(o.value(QStringLiteral("downloads")).toDouble());
            hits.push_back(hit);
        }
        onResult(hits, QString());
    });
}

void ModrinthClient::GetLatestPrimaryFile(
    const QString& projectId,
    std::function<void(std::optional<ModrinthVersionFile> file, QString errorMessage)> onResult)
{
    QUrl url(QStringLiteral("https://api.modrinth.com/v2/project/%1/version").arg(projectId));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("include_changelog"), QStringLiteral("false"));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", kUserAgent);

    QNetworkReply* reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, onResult]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            onResult(std::nullopt, reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray() || doc.array().isEmpty())
        {
            onResult(std::nullopt, QStringLiteral("Không tìm thấy bản build nào cho project này."));
            return;
        }

        // Modrinth returns versions newest-first, so [0] is "latest".
        const QJsonObject latestVersion = doc.array().first().toObject();
        const QJsonArray files = latestVersion.value(QStringLiteral("files")).toArray();
        if (files.isEmpty())
        {
            onResult(std::nullopt, QStringLiteral("Bản build mới nhất không có file để tải."));
            return;
        }

        QJsonObject chosen = files.first().toObject();
        for (const auto& fileValue : files)
        {
            const QJsonObject fileObject = fileValue.toObject();
            if (fileObject.value(QStringLiteral("primary")).toBool())
            {
                chosen = fileObject;
                break;
            }
        }

        ModrinthVersionFile file;
        file.url = chosen.value(QStringLiteral("url")).toString();
        file.filename = chosen.value(QStringLiteral("filename")).toString();
        file.primary = chosen.value(QStringLiteral("primary")).toBool();
        onResult(file, QString());
    });
}

void ModrinthClient::DownloadFile(
    const ModrinthVersionFile& file,
    const QString& destinationFilePath,
    std::function<void(bool success, QString errorMessage)> onFinished)
{
    QNetworkRequest request((QUrl(file.url)));
    request.setRawHeader("User-Agent", kUserAgent);

    QNetworkReply* reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, destinationFilePath, onFinished]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            onFinished(false, reply->errorString());
            return;
        }

        const QByteArray data = reply->readAll();

        const QFileInfo info(destinationFilePath);
        QDir().mkpath(info.absolutePath());

        QFile outFile(destinationFilePath);
        if (!outFile.open(QIODevice::WriteOnly))
        {
            onFinished(false, QStringLiteral("Không thể ghi file: %1").arg(destinationFilePath));
            return;
        }
        outFile.write(data);
        outFile.close();
        onFinished(true, QString());
    });
}

} // namespace net
