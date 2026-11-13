#include "Formats.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

#include "Json.h"
#include "updater/v2/Signature.h"

namespace UpdaterV2
{

namespace {

const int MaxKeys = 4;
const int MaxMirrors = 16;
const int MaxSignatureLines = 8;
const int MaxPath = 512;
const int MaxVariantId = 64;
const int MaxVariantName = 64;
// following links further than this counts as a loop
const int MaxLinkDepth = 40;

[[noreturn]] void fail(const QString &message)
{
    throw Json::JsonException(message);
}

const char *Commit = "[0-9a-f]{40}";
const char *Version = "(0|[1-9][0-9]*)(\\.(0|[1-9][0-9]*))*";
// os and arch may be ones this Launcher doesn't know: such a variant is not for this machine
const char *Word = "[a-z0-9_]+";
const char *VariantId = "([a-z0-9_]+)-([a-z0-9_]+)-[a-z0-9.]+(-[a-z0-9.]+)*";
// printable ASCII, not starting or ending with a space
const char *VariantName = "[\\x21-\\x7e]([\\x20-\\x7e]*[\\x21-\\x7e])?";
const char *BaseUrl = "https://[\\x21-\\x7e]+/";

QString at(const QString &where, const QString &key)
{
    return where.isEmpty() ? key : where + "." + key;
}

QString at(const QString &where, int index)
{
    return QString("%1[%2]").arg(where).arg(index);
}

QJsonObject parseObject(const QByteArray &data, int limit)
{
    if(data.size() > limit)
    {
        fail(QString("%1 bytes, more than %2").arg(data.size()).arg(limit));
    }
    auto doc = Json::requireTextDocument(data);
    return Json::requireObject(doc);
}

QJsonArray list(const QJsonValue &value, const QString &where, qint64 minimum, qint64 maximum)
{
    auto array = Json::requireIsType<QJsonArray>(value, where);
    if(array.size() < minimum || array.size() > maximum)
    {
        fail(QString("%1: %2 entries, allowed %3 to %4").arg(where).arg(array.size()).arg(minimum).arg(maximum));
    }
    return array;
}

QString hex(const QJsonValue &value, int bytes, const QString &where)
{
    return QString::fromLatin1(Json::requireValueHex(value, bytes, where).toHex());
}

QStringList keys(const QJsonValue &value, const QString &where)
{
    auto array = list(value, where, 1, MaxKeys);
    QStringList out;
    for(int i = 0; i < array.size(); i++)
    {
        auto key = hex(array[i], 32, at(where, i));
        if(out.contains(key))
        {
            fail(QString("%1: a key is listed twice").arg(where));
        }
        out.append(key);
    }
    return out;
}

void header(const QJsonObject &doc, const QString &type, QStringList names)
{
    // what it is and which format first, so a wrong document says so rather than what it lacks
    if(doc.value("type") != QJsonValue(type))
    {
        fail(QString("type isn't '%1'").arg(type));
    }
    auto format = doc.value("format");
    if(!format.isDouble() || format.toDouble() != 1.0)
    {
        fail("format isn't supported (only 1)");
    }
    names << "type" << "format";
    Json::requireKeys(doc, names, type);
}

void path(const QJsonValue &value, const QString &where, bool dotdot = false)
{
    auto p = Json::requireIsType<QString>(value, where);
    if(!isValidPath(p, dotdot))
    {
        fail(QString("%1: '%2' isn't a valid path").arg(where, p));
    }
}

SystemRequirements::Requirements requirements(const QJsonValue &value, const QString &where)
{
    auto req = Json::requireIsType<QJsonObject>(value, where);
    SystemRequirements::Requirements out;
    auto os = Json::requireValueMatching(req.value("os"), Word, at(where, "os"));
    out.os = os;
    out.arch = Json::requireValueMatching(req.value("arch"), Word, at(where, "arch"));
    QStringList known = {"os", "arch"};
    if(os == "windows" || os == "macos")
    {
        known << "os_version";
    }
    else if(os == "linux")
    {
        known << "glibc" << "glibcxx" << "qt";
    }
    Json::requireKeys(req, known, where);
    // a requirement this Launcher doesn't know can't be checked, so it isn't met (unmetRequirements)
    for(auto &key: req.keys())
    {
        if(!known.contains(key))
        {
            out.unknown.append(key);
        }
    }
    if(os == "linux")
    {
        out.glibc = Json::requireValueMatching(req.value("glibc"), Version, at(where, "glibc"));
        out.glibcxx = Json::requireValueMatching(req.value("glibcxx"), Version, at(where, "glibcxx"));
        out.qt = Json::requireValueMatching(req.value("qt"), Version, at(where, "qt"));
    }
    else if(os == "windows" || os == "macos")
    {
        out.osVersion = Json::requireValueMatching(req.value("os_version"), Version, at(where, "os_version"));
    }
    return out;
}

// where a link points, followed through other links: the path inside the installation
QString resolveLink(const QString &linkPath, const QMap<QString, QString> &links, const QString &where)
{
    auto current = linkPath.split('/');
    current.removeLast();
    auto pending = links.value(linkPath).split('/');
    int depth = 0;
    while(!pending.isEmpty())
    {
        auto part = pending.takeFirst();
        if(part == "..")
        {
            if(current.isEmpty())
            {
                fail(QString("%1: points outside the installation").arg(where));
            }
            current.removeLast();
            continue;
        }
        current.append(part);
        auto here = current.join('/');
        if(links.contains(here))
        {
            if(++depth > MaxLinkDepth)
            {
                fail(QString("%1: following the links loops").arg(where));
            }
            current.removeLast();
            pending = links.value(here).split('/') + pending;
        }
    }
    return current.join('/');
}

Variant variant(const QString &id, const QJsonValue &value, const QString &where)
{
    auto obj = Json::requireIsType<QJsonObject>(value, where);
    Json::requireKeys(obj, {"name", "supersedes", "requires", "files", "links"}, where);

    Variant out;
    out.id = id;
    out.name = Json::requireValueMatching(obj.value("name"), VariantName, at(where, "name"));
    if(out.name.size() > MaxVariantName)
    {
        fail(QString("%1: longer than %2 characters").arg(at(where, "name")).arg(MaxVariantName));
    }
    out.requirements = requirements(obj.value("requires"), at(where, "requires"));
    QRegularExpression idParts(QString("\\A(?:%1)\\z").arg(VariantId));
    auto m = idParts.match(id);
    if(m.captured(1) != out.requirements.os || m.captured(2) != out.requirements.arch)
    {
        fail(QString("%1: the ID doesn't match requires").arg(where));
    }

    auto supersedes = list(obj.value("supersedes"), at(where, "supersedes"), 0, Json::MaxExactInteger);
    for(int i = 0; i < supersedes.size(); i++)
    {
        auto w = at(at(where, "supersedes"), i);
        auto other = Json::requireValueMatching(supersedes[i], VariantId, w);
        if(other.size() > MaxVariantId)
        {
            fail(QString("%1: longer than %2 characters").arg(w).arg(MaxVariantId));
        }
        if(other == id)
        {
            fail(QString("%1: supersedes itself").arg(where));
        }
        if(out.supersedes.contains(other))
        {
            fail(QString("%1.supersedes: a variant is listed twice").arg(where));
        }
        out.supersedes.append(other);
    }

    // Optional (added 2026-11-13): the last day builds of the variant are made. One that
    // can't be read is left out, not the variant: it only decides a notice.
    auto deprecated = obj.value("deprecated");
    if(!deprecated.isUndefined())
    {
        out.deprecated = QDate::fromString(deprecated.toString(), "yyyy-MM-dd");
        if(!out.deprecated.isValid())
        {
            qWarning() << "Updater:" << at(where, "deprecated") << "isn't a date, ignored";
        }
    }

    QStringList paths;
    auto files = list(obj.value("files"), at(where, "files"), 1, Json::MaxExactInteger);
    for(int i = 0; i < files.size(); i++)
    {
        auto w = at(at(where, "files"), i);
        auto entry = Json::requireIsType<QJsonObject>(files[i], w);
        Json::requireKeys(entry, {"path", "sha256", "size", "executable"}, w);
        FileEntry file;
        path(entry.value("path"), at(w, "path"));
        file.path = entry.value("path").toString();
        file.sha256 = hex(entry.value("sha256"), 32, at(w, "sha256"));
        file.size = Json::requireValueBoundedInteger(entry.value("size"), 0, Json::MaxExactInteger, at(w, "size"));
        file.executable = Json::requireIsType<bool>(entry.value("executable"), at(w, "executable"));
        out.files.append(file);
        paths.append(file.path);
    }

    QMap<QString, QString> links;
    QStringList linkPaths;
    auto linkList = list(obj.value("links"), at(where, "links"), 0, Json::MaxExactInteger);
    if(!linkList.isEmpty() && out.requirements.os == "windows")
    {
        fail(QString("%1: a Windows variant has no links").arg(where));
    }
    for(int i = 0; i < linkList.size(); i++)
    {
        auto w = at(at(where, "links"), i);
        auto entry = Json::requireIsType<QJsonObject>(linkList[i], w);
        Json::requireKeys(entry, {"path", "target"}, w);
        LinkEntry link;
        path(entry.value("path"), at(w, "path"));
        path(entry.value("target"), at(w, "target"), true);
        link.path = entry.value("path").toString();
        link.target = entry.value("target").toString();
        out.links.append(link);
        links.insert(link.path, link.target);
        linkPaths.append(link.path);
    }

    // sorted by path in byte order; the paths are ASCII, so QString's order is the same
    for(auto listed: {paths, linkPaths})
    {
        auto sorted = listed;
        std::sort(sorted.begin(), sorted.end());
        if(listed != sorted)
        {
            fail(QString("%1: not sorted by path").arg(where));
        }
    }

    // one spelling per path and directory, compared case-insensitively; nothing is both a
    // file or link and a directory (so no path goes through a link)
    auto entries = paths + linkPaths;
    QMap<QString, QString> spelling;
    QSet<QString> directories;
    for(auto &entryPath: entries)
    {
        auto parts = entryPath.split('/');
        for(int n = 1; n <= parts.size(); n++)
        {
            auto prefix = QStringList(parts.mid(0, n)).join('/');
            if(n < parts.size())
            {
                directories.insert(prefix);
            }
            auto lower = prefix.toLower();
            if(!spelling.contains(lower))
            {
                spelling.insert(lower, prefix);
            }
            else if(spelling.value(lower) != prefix)
            {
                fail(QString("%1: '%2' and '%3' differ only in case").arg(where, spelling.value(lower), prefix));
            }
        }
    }
    auto entrySet = QSet<QString>::fromList(entries);
    if(entrySet.size() != entries.size())
    {
        fail(QString("%1: a path is listed twice").arg(where));
    }
    if(!QSet<QString>(directories).intersect(entrySet).isEmpty())
    {
        fail(QString("%1: a listed path is also a directory of other paths").arg(where));
    }

    auto fileSet = QSet<QString>::fromList(paths);
    for(auto &linkPath: linkPaths)
    {
        auto target = resolveLink(linkPath, links, QString("%1: link '%2'").arg(where, linkPath));
        if(!fileSet.contains(target) && !directories.contains(target))
        {
            fail(QString("%1: link '%2' points to '%3', which isn't a listed file or a directory with files").arg(where, linkPath, target));
        }
    }
    return out;
}

template <typename T>
bool guarded(QString &error, T body)
{
    try
    {
        body();
        return true;
    }
    catch(const Json::JsonException &e)
    {
        error = e.cause();
        return false;
    }
}

}

bool isValidPath(const QString &path, bool dotdot)
{
    if(path.isEmpty() || path.size() > MaxPath)
    {
        return false;
    }
    for(auto c: path)
    {
        if(c < QChar(0x20) || c > QChar(0x7e) || QString("\\:*?\"<>|").contains(c))
        {
            return false;
        }
    }
    for(auto &part: path.split('/'))
    {
        if(part.isEmpty() || part == "." || (part == ".." && !dotdot))
        {
            return false;
        }
    }
    return true;
}

QString contentPath(const QString &sha256)
{
    return QString("files/%1/%2").arg(sha256.left(2), sha256);
}

QUrl treeUrl(const QString &site, const QString &name)
{
    return QUrl(site + "updater/" + name);
}

bool parseKeys(const QByteArray &data, qint64 expected, KeysDocument &out, QString &error)
{
    return guarded(error, [&]()
    {
        auto doc = parseObject(data, MaxDocumentSize);
        header(doc, "keys", {"version", "expires", "root_keys", "online_keys"});
        KeysDocument keysDoc;
        keysDoc.version = Json::requireBoundedInteger(doc, "version", 1, Json::MaxExactInteger, "version");
        if(keysDoc.version != expected)
        {
            fail(QString("version is %1, but the file is for version %2").arg(keysDoc.version).arg(expected));
        }
        keysDoc.expires = Json::requireUtcTime(doc, "expires", "expires");
        keysDoc.rootKeys = keys(doc.value("root_keys"), "root_keys");
        keysDoc.onlineKeys = keys(doc.value("online_keys"), "online_keys");
        for(auto &key: keysDoc.rootKeys)
        {
            if(keysDoc.onlineKeys.contains(key))
            {
                fail("a key is both a root key and an online key");
            }
        }
        out = keysDoc;
    });
}

bool parseTags(const QByteArray &data, TagsDocument &out, QString &error)
{
    return guarded(error, [&]()
    {
        auto doc = parseObject(data, MaxDocumentSize);
        header(doc, "tags", {"serial", "issued", "tags", "withdrawn"});
        TagsDocument tags;
        tags.serial = Json::requireBoundedInteger(doc, "serial", 1, Json::MaxExactInteger, "serial");
        tags.issued = Json::requireUtcTime(doc, "issued", "issued");
        auto tagObj = Json::requireIsType<QJsonObject>(doc.value("tags"), "tags");
        Json::requireKeys(tagObj, {"latest", "testing", "stable"}, "tags");
        tags.latest = Json::requireBoundedInteger(tagObj, "latest", 1, Json::MaxExactInteger, "tags.latest");
        tags.testing = Json::requireBoundedInteger(tagObj, "testing", 1, Json::MaxExactInteger, "tags.testing");
        tags.stable = Json::requireBoundedInteger(tagObj, "stable", 1, Json::MaxExactInteger, "tags.stable");
        if(!(tags.stable <= tags.testing && tags.testing <= tags.latest))
        {
            fail(QString("tags: need stable <= testing <= latest, have %1, %2, %3").arg(tags.stable).arg(tags.testing).arg(tags.latest));
        }
        auto withdrawn = list(doc.value("withdrawn"), "withdrawn", 0, Json::MaxExactInteger);
        for(int i = 0; i < withdrawn.size(); i++)
        {
            auto build = Json::requireValueBoundedInteger(withdrawn[i], 1, Json::MaxExactInteger, at("withdrawn", i));
            if(!tags.withdrawn.isEmpty() && tags.withdrawn.last() >= build)
            {
                fail("withdrawn: not ascending, or a build is listed twice");
            }
            if(build == tags.latest || build == tags.testing || build == tags.stable)
            {
                fail(QString("withdrawn: build %1 is tagged").arg(build));
            }
            tags.withdrawn.append(build);
        }
        out = tags;
    });
}

bool parseMirrors(const QByteArray &data, MirrorsDocument &out, QString &error)
{
    return guarded(error, [&]()
    {
        auto doc = parseObject(data, MaxDocumentSize);
        header(doc, "mirrors", {"issued", "mirrors"});
        MirrorsDocument mirrors;
        mirrors.issued = Json::requireUtcTime(doc, "issued", "issued");
        auto groups = list(doc.value("mirrors"), "mirrors", 1, MaxMirrors);
        QSet<QString> seen;
        for(int g = 0; g < groups.size(); g++)
        {
            auto group = list(groups[g], at("mirrors", g), 1, MaxMirrors);
            QStringList urls;
            for(int i = 0; i < group.size(); i++)
            {
                auto where = at(at("mirrors", g), i);
                auto url = Json::requireIsType<QString>(group[i], where);
                QRegularExpression base(QString("\\A(?:%1)\\z").arg(BaseUrl));
                auto host = url.mid(8).section('/', 0, 0);
                if(!base.match(url).hasMatch() || url.contains('@') || url.contains('?') || url.contains('#') || url.contains('\\') || host.isEmpty())
                {
                    fail(QString("%1: '%2' isn't an https:// base URL ending in / without user info, query or fragment").arg(where, url));
                }
                if(seen.contains(url))
                {
                    fail(QString("%1: '%2' is listed twice").arg(where, url));
                }
                seen.insert(url);
                urls.append(url);
            }
            mirrors.groups.append(urls);
        }
        if(seen.size() > MaxMirrors)
        {
            fail(QString("mirrors: %1 URLs, at most %2").arg(seen.size()).arg(MaxMirrors));
        }
        out = mirrors;
    });
}

bool parseBuild(const QByteArray &data, qint64 expected, BuildManifest &out, QString &error)
{
    return guarded(error, [&]()
    {
        auto doc = parseObject(data, MaxManifestSize);
        header(doc, "build", {"build", "commit", "built", "variants"});
        BuildManifest manifest;
        manifest.build = Json::requireBoundedInteger(doc, "build", 1, Json::MaxExactInteger, "build");
        if(manifest.build != expected)
        {
            fail(QString("build is %1, but the file is for build %2").arg(manifest.build).arg(expected));
        }
        manifest.commit = Json::requireMatching(doc, "commit", Commit, "commit");
        manifest.built = Json::requireUtcTime(doc, "built", "built");
        auto variants = doc.value("variants");
        if(!variants.isObject() || variants.toObject().isEmpty())
        {
            fail("variants: not an object with at least one variant");
        }
        auto variantObj = variants.toObject();
        QRegularExpression variantId(QString("\\A(?:%1)\\z").arg(VariantId));
        // Each variant on its own: one this Launcher can't read is left out, so later builds
        // can add variants described in ways it doesn't know (a Qt 6 variant with library
        // requirements, say) without cutting it off from updates.
        for(auto it = variantObj.begin(); it != variantObj.end(); it++)
        {
            if(!variantId.match(it.key()).hasMatch() || it.key().size() > MaxVariantId)
            {
                // the ID isn't trusted text: shown only shortened, and kept under a fixed name
                manifest.unreadable.insert(QString("#%1").arg(manifest.unreadable.size() + 1),
                                           QString("'%1' isn't a valid variant ID").arg(it.key().left(MaxVariantId)));
                continue;
            }
            try
            {
                manifest.variants.insert(it.key(), variant(it.key(), it.value(), at("variants", it.key())));
            }
            catch(const Json::JsonException &e)
            {
                manifest.unreadable.insert(it.key(), e.cause());
            }
        }

        // a single chain of successors from every variant: each superseded at most once, no cycles
        QMap<QString, QString> successor;
        for(auto &v: manifest.variants)
        {
            for(auto &other: v.supersedes)
            {
                if(successor.contains(other))
                {
                    fail(QString("variants: %1 is superseded by both %2 and %3").arg(other, successor.value(other), v.id));
                }
                successor.insert(other, v.id);
            }
        }
        for(auto &start: successor.keys())
        {
            QSet<QString> seen{start};
            auto current = start;
            while(successor.contains(current))
            {
                current = successor.value(current);
                if(seen.contains(current))
                {
                    fail(QString("variants: the supersedes chain from %1 loops").arg(start));
                }
                seen.insert(current);
            }
        }
        out = manifest;
    });
}

bool checkSignatureFile(const QByteArray &document, const QByteArray &signatureFile, const QStringList &trustedKeys, QString &error)
{
    return guarded(error, [&]()
    {
        if(signatureFile.size() > MaxSignatureFileSize)
        {
            fail("signature file: too large");
        }
        if(signatureFile.isEmpty() || !signatureFile.endsWith('\n'))
        {
            fail("signature file: empty, or doesn't end with a newline");
        }
        auto lines = signatureFile.left(signatureFile.size() - 1).split('\n');
        if(lines.size() > MaxSignatureLines)
        {
            fail(QString("signature file: %1 lines, allowed 1 to %2").arg(lines.size()).arg(MaxSignatureLines));
        }
        QRegularExpression lineFormat("\\A([0-9a-f]{64}) ([0-9a-f]{128})\\z");
        QStringList seen;
        int trusted = 0;
        for(int i = 0; i < lines.size(); i++)
        {
            auto m = lineFormat.match(QString::fromLatin1(lines[i]));
            if(!m.hasMatch())
            {
                fail(QString("signature file: line %1 isn't <64 hex digits> <128 hex digits>").arg(i + 1));
            }
            auto key = m.captured(1);
            if(seen.contains(key))
            {
                fail("signature file: a key is listed twice");
            }
            seen.append(key);
            if(!trustedKeys.contains(key))
            {
                continue;
            }
            trusted++;
            auto signature = QByteArray::fromHex(m.captured(2).toLatin1());
            if(!Signature::checkEd25519(document, signature, QByteArray::fromHex(key.toLatin1())))
            {
                fail(QString("the signature by %1 doesn't verify").arg(key));
            }
        }
        if(!trusted)
        {
            fail("no signature by a trusted key");
        }
    });
}

}
