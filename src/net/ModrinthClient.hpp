#pragma once

// Thin async wrapper around the public Modrinth API (docs.modrinth.com)
// used by ui::LibraryDialog to search and download mods/plugins/modpacks/
// resource packs/shaders. Every call is asynchronous: the supplied
// callback is invoked later, on the Qt UI thread (QNetworkAccessManager's
// signals are delivered there since this object lives on it), never
// synchronously from within the call itself.
//
// Endpoints used (verified against docs.modrinth.com/api on 2026):
//   GET /v2/search?query=..&facets=[["project_type:X"]]&limit=..&index=..
//   GET /v2/project/{id}/version?include_changelog=false

#include <QList>
#include <QObject>
#include <QString>

#include <functional>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;

namespace net
{

struct ModrinthProjectHit
{
    QString projectId;
    QString slug;
    QString title;
    QString description;
    QString author;
    QString projectType; // "mod" | "modpack" | "resourcepack" | "shader" (primary type)
    QString iconUrl;
    qint64 downloads = 0;
};

struct ModrinthVersionFile
{
    QString url;
    QString filename;
    bool primary = false;
};

class ModrinthClient : public QObject
{
    Q_OBJECT

public:
    explicit ModrinthClient(QObject* parent = nullptr);

    // `projectTypeFacet` is a raw Modrinth facet value: "mod", "plugin",
    // "modpack", "resourcepack", or "shader".
    void Search(
        const QString& query,
        const QString& projectTypeFacet,
        std::function<void(QList<ModrinthProjectHit> hits, QString errorMessage)> onResult);

    // Picks the newest version's primary file (or its first file, if none
    // is marked primary) for the given project.
    void GetLatestPrimaryFile(
        const QString& projectId,
        std::function<void(std::optional<ModrinthVersionFile> file, QString errorMessage)> onResult);

    void DownloadFile(
        const ModrinthVersionFile& file,
        const QString& destinationFilePath,
        std::function<void(bool success, QString errorMessage)> onFinished);

private:
    QNetworkAccessManager* manager_;
};

} // namespace net
