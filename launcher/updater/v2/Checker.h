#pragma once
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>

#include <functional>

#include "Formats.h"
#include "Plan.h"
#include "State.h"
#include "QObjectPtr.h"
#include "net/Download.h"

namespace UpdaterV2
{

/**
 * What updater v2 is built with, and where it runs.
 */
struct Config
{
    // the files site's first base URL (its mirror list is the tree's mirrors.json); the tree
    // is at updater/ under it
    QString site;
    // the pinned root keys (hex) and the compiled-in keys document, which must list exactly them
    QStringList rootKeys;
    QByteArray floorKeys;
    // the lowest tags serial accepted when nothing higher is stored
    qint64 serialFloor = 0;
    // this build's variant ID and build number
    QString variant;
    qint64 currentBuild = -1;
    // where the verified state is kept
    QString statePath;
    SystemRequirements::System system;
    // for tests: the time to check expiry against
    QDateTime now;
};

struct CheckResult
{
    enum Kind
    {
        // on the tier's build already
        NoUpdate,
        // a newer build
        Update,
        // the installed build, in another variant
        Switch,
        // an older build, after switching tiers
        Downgrade,
        // the tier is on an older build; nothing offered without a tier switch
        BehindInstalled,
        // no variant of the build runs here (messages say why)
        NotForThisMachine,
        // no verified answer (messages say what went wrong)
        Failed
    } kind = Failed;
    Tier tier = Tier::Stable;
    // as given to check()
    bool tierSwitched = false;
    bool notify = false;
    qint64 currentBuild = -1;
    // the tier's build, and the commit it was made from (when its manifest was read)
    qint64 build = 0;
    QString commit;
    // the installed build was removed from the servers
    bool currentWithdrawn = false;
    // the variant of this build, and whether the tier's build no longer has it
    QString ownVariant;
    bool ownVariantDropped = false;
    // the last day the tier's build says builds of this variant are made (invalid: not deprecated)
    QDate ownVariantDeprecated;
    // the variants of the tier's build this machine can run, and the one it moves to when the
    // user hasn't chosen one (empty when none runs here); known unless the check failed
    QList<Variant> runnable;
    QString recommended;
    // what to install
    Variant variant;
    // the installed build's variant, when its manifest is still there
    bool haveCurrent = false;
    Variant current;
    QStringList messages;
};

/**
 * Checks the tree for an update: the keys documents from the newest known one forward, the
 * mirror list, the tags, and the manifests of the tier's build and the installed build. Each
 * document comes with its signature from one of the files site's mirrors (in Net::Mirrors'
 * order), and from the next one when that fails or serves a document that doesn't verify;
 * every signature is checked over the raw bytes before anything is parsed.
 *
 * The newest verified mirror list becomes the files site's mirror list, for every download
 * from the site.
 */
class Checker : public QObject
{
    Q_OBJECT
public:
    Checker(shared_qobject_ptr<QNetworkAccessManager> network, const Config &config);

    // empty when the compiled-in configuration is usable
    QString configError() const
    {
        return m_state.error();
    }
    bool isChecking() const
    {
        return m_checking;
    }

    /**
     * Start a check for the tier. `tierSwitched`: the user just switched to it (or to another
     * variant), so an older build is offered as a downgrade. `notify` is passed on to the result, for the UI to tell
     * the user about every outcome (a check they asked for). While a check runs, it starts
     * when that one is done (only the last one asked for).
     */
    void check(Tier tier, bool tierSwitched, bool notify);

    /**
     * The tags of the last check that verified them (which build each tier is on). False
     * before a check got that far.
     */
    bool lastTags(TagsDocument &tags) const;

    /**
     * The variant the user chose to stay on or go back to (the UpdateVariant setting), for the
     * checks from now on; empty for none, then Launchers move along `supersedes`.
     */
    void setPreferredVariant(const QString &variant);

    /**
     * The variants of the tier's build in the last check that read it: the ones this machine
     * can run, and the recommended one. False before a check got that far.
     */
    bool lastVariants(QList<Variant> &runnable, QString &recommended) const;

signals:
    void finished(const UpdaterV2::CheckResult &result);
    // a check verified the tags; lastTags() has them
    void tagsVerified();
    // a check read the tier's build; lastVariants() has its variants
    void variantsKnown();

private:
    void nextKeys();
    void afterKeys();
    void afterTags();
    void fail(const QString &error);
    void done(CheckResult::Kind kind, const QStringList &messages = QStringList());
    void useMirrors();

    QUrl url(const QString &name) const;
    /**
     * Get a document and its signature from one mirror. Once the signature verifies with one
     * of `trusted`, `accept` reads the document; when it refuses it (returns false and sets the
     * error), the document is refused like one that doesn't verify, and the next mirror is
     * tried. Then `then` is called with found = true. When nothing works on any mirror and
     * `optional`, a document a mirror said isn't there calls `then` with found = false;
     * otherwise the check fails.
     */
    void fetchSigned(const QString &name, int limit, const QStringList &trusted, bool optional,
                     std::function<bool(const QByteArray &data, QString &error)> accept, std::function<void(bool found)> then);

    // a fetchSigned() on its way through the mirrors
    struct Fetch
    {
        QString name;
        int limit = 0;
        QStringList trusted;
        bool optional = false;
        std::function<bool(const QByteArray &, QString &)> accept;
        std::function<void(bool)> then;
        // the files site's base URLs, in the order to try them
        QStringList mirrors;
        // a mirror said the document isn't there
        bool notFound = false;
        QStringList errors;
    };
    void fetchFrom(const Fetch &fetch, int index);

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    Config m_config;
    State m_state;

    bool m_checking = false;
    bool m_stateChanged = false;
    Tier m_tier = Tier::Stable;
    bool m_tierSwitched = false;
    bool m_notify = false;
    // a check asked for while one ran
    bool m_pending = false;
    Tier m_pendingTier = Tier::Stable;
    bool m_pendingTierSwitched = false;
    bool m_pendingNotify = false;

    Net::Download::Ptr m_download;
    KeysDocument m_keys;
    QByteArray m_keysData;
    TagsDocument m_tags;
    // the tags of the last check that verified them
    TagsDocument m_verifiedTags;
    bool m_haveVerifiedTags = false;
    QString m_preferred;
    QList<Variant> m_lastRunnable;
    QString m_lastRecommended;
    bool m_haveVariants = false;
    TierDecision m_decision;
    BuildManifest m_build;
    CheckResult m_result;
};

}

Q_DECLARE_METATYPE(UpdaterV2::CheckResult)
