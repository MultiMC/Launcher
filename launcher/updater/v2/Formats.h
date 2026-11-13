#pragma once
#include <QDate>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "updater/v2/SystemRequirements.h"

/**
 * The documents of updater v2's tree, format 1 (Design/UpdaterFormats.md in the MultiMC
 * repository): the keys documents, the tags document, the mirror list, the build manifests
 * and their detached signature files.
 *
 * Strict on purpose, like hopper's validator (lib/updater_formats.py): anything not fully
 * understood is refused, with a message saying where. Documents are only ever parsed after
 * their signature has been checked over the raw bytes (checkSignatureFile).
 *
 * Qt's JSON parser can't tell duplicate keys or an integer written as 1.0 or 1e3 apart from
 * the plain form; the validator refuses those before anything is signed, so here the
 * signature vouches for them.
 */
namespace UpdaterV2
{

// Size limits, checked before the signature
const int MaxDocumentSize = 64 * 1024;
const int MaxManifestSize = 8 * 1024 * 1024;
const int MaxSignatureFileSize = 8 * 194;

struct KeysDocument
{
    qint64 version = 0;
    QDateTime expires;
    // hex, as in the document
    QStringList rootKeys;
    QStringList onlineKeys;
};

struct TagsDocument
{
    qint64 serial = 0;
    QDateTime issued;
    qint64 latest = 0;
    qint64 testing = 0;
    qint64 stable = 0;
    QList<qint64> withdrawn;
};

struct MirrorsDocument
{
    QDateTime issued;
    // groups of base URLs, in order of priority
    QList<QStringList> groups;
};

struct FileEntry
{
    QString path;
    QString sha256;
    qint64 size = 0;
    bool executable = false;
};

struct LinkEntry
{
    QString path;
    QString target;
};

struct Variant
{
    QString id;
    // shown to users
    QString name;
    QStringList supersedes;
    // the last day builds of it are made, when it is deprecated (invalid otherwise)
    QDate deprecated;
    SystemRequirements::Requirements requirements;
    QList<FileEntry> files;
    QList<LinkEntry> links;
};

struct BuildManifest
{
    qint64 build = 0;
    QString commit;
    QDateTime built;
    QMap<QString, Variant> variants;
    // variants this Launcher can't read (a newer format, a requirement described another way),
    // with why: they never run here, but don't make the whole build unreadable
    QMap<QString, QString> unreadable;
};

/**
 * Parse a document from its bytes. `expected` is the number in the file name (the keys
 * version or the build number); the document must say the same.
 * Return false and set `error` when it isn't valid.
 */
bool parseKeys(const QByteArray &data, qint64 expected, KeysDocument &out, QString &error);
bool parseTags(const QByteArray &data, TagsDocument &out, QString &error);
bool parseMirrors(const QByteArray &data, MirrorsDocument &out, QString &error);
bool parseBuild(const QByteArray &data, qint64 expected, BuildManifest &out, QString &error);

/**
 * Check a detached signature file against the exact bytes of its document: at least one line
 * by a key in `trustedKeys` (hex), and every such line must verify. Lines by other keys are
 * ignored.
 */
bool checkSignatureFile(const QByteArray &document, const QByteArray &signatureFile, const QStringList &trustedKeys, QString &error);

/**
 * Whether a path from a manifest is safe to use relative to the installation root:
 * parts separated by /, no empty, . or .. parts (.. allowed when `dotdot`), printable ASCII
 * without \ : * ? " < > |, at most 512 characters.
 */
bool isValidPath(const QString &path, bool dotdot = false);

/**
 * Where the content file with this SHA-256 is, relative to the tree root.
 */
QString contentPath(const QString &sha256);

/**
 * A file of the tree on the files site whose base URL is `site`: the tree is at updater/.
 */
QUrl treeUrl(const QString &site, const QString &name);

}
