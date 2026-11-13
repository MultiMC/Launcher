#include "Checker.h"

#include <QDebug>

#include <memory>

#include "net/Download.h"
#include "net/Mirrors.h"
#include "net/SizeValidator.h"

namespace UpdaterV2
{

namespace {

// a transfer that gets nothing for this long fails, and the next mirror is tried
const int StallTimeout = 30 * 1000;

/*
 * Refuses a document whose detached signature doesn't verify over its exact bytes, or that
 * isn't accepted once it does, so the download moves on to the next mirror.
 */
class SignatureValidator : public Net::Validator
{
public:
    SignatureValidator(const QByteArray &signature, const QStringList &trusted,
                       std::function<bool(const QByteArray &, QString &)> accept, std::shared_ptr<QString> error)
        : m_signature(signature), m_trusted(trusted), m_accept(accept), m_error(error)
    {
    }

    bool init(QNetworkRequest &) override
    {
        // a retry or the next mirror starts over
        m_data.clear();
        return true;
    }
    bool write(QByteArray &data) override
    {
        m_data.append(data);
        return true;
    }
    bool abort() override
    {
        return true;
    }
    bool validate(QNetworkReply &) override
    {
        // the signature over the raw bytes first; nothing is parsed before it verifies
        if(!checkSignatureFile(m_data, m_signature, m_trusted, *m_error) || !m_accept(m_data, *m_error))
        {
            qWarning() << "Updater:" << *m_error;
            return false;
        }
        return true;
    }

private:
    QByteArray m_data;
    QByteArray m_signature;
    QStringList m_trusted;
    std::function<bool(const QByteArray &, QString &)> m_accept;
    std::shared_ptr<QString> m_error;
};

// why a download failed, for the user
QString failure(Net::Download *download)
{
    if(download->wasNotFound())
    {
        return QObject::tr("not found");
    }
    if(download->httpStatus())
    {
        return QObject::tr("HTTP status %1").arg(download->httpStatus());
    }
    return QObject::tr("network error %1").arg(download->networkError());
}

}

Checker::Checker(shared_qobject_ptr<QNetworkAccessManager> network, const Config &config)
    : m_network(network), m_config(config), m_state(config.floorKeys, config.rootKeys, config.serialFloor)
{
    if(!m_state.error().isEmpty())
    {
        qCritical() << "Updater:" << m_state.error();
        return;
    }
    if(!m_config.statePath.isEmpty())
    {
        m_state.load(m_config.statePath);
    }
    useMirrors();
    qDebug() << "Updater: keys version" << m_state.keys().version << "tags serial" << m_state.serial() << "variant" << m_config.variant;
}

void Checker::useMirrors()
{
    if(m_state.hasMirrors())
    {
        Net::Mirrors::setMirrorList(m_config.site, m_state.mirrors().groups);
    }
}

QUrl Checker::url(const QString &name) const
{
    return treeUrl(m_config.site, name);
}

void Checker::check(Tier tier, bool tierSwitched, bool notify)
{
    if(m_checking)
    {
        qDebug() << "Updater: a check is running already, this one starts after it";
        m_pending = true;
        m_pendingTier = tier;
        m_pendingTierSwitched = tierSwitched;
        m_pendingNotify = notify;
        return;
    }
    m_checking = true;
    m_stateChanged = false;
    m_tier = tier;
    m_tierSwitched = tierSwitched;
    m_notify = notify;
    m_result = CheckResult();
    if(!configError().isEmpty())
    {
        done(CheckResult::Failed, {configError()});
        return;
    }
    qDebug() << "Updater: checking" << tierName(tier) << "on" << url("");
    m_keys = m_state.keys();
    m_keysData.clear();
    nextKeys();
}

void Checker::fail(const QString &error)
{
    qWarning() << "Updater:" << error;
    done(CheckResult::Failed, {error});
}

void Checker::fetchSigned(const QString &name, int limit, const QStringList &trusted, bool optional,
                          std::function<bool(const QByteArray &, QString &)> accept, std::function<void(bool)> then)
{
    Fetch fetch;
    fetch.name = name;
    fetch.limit = limit;
    fetch.trusted = trusted;
    fetch.optional = optional;
    fetch.accept = accept;
    fetch.then = then;
    // the files site's mirrors in the order Net::Mirrors gives, the unreachable ones last
    for(auto &candidate: Net::Mirrors::candidates(url(name)))
    {
        auto text = candidate.toString(QUrl::FullyEncoded);
        fetch.mirrors.append(text.left(text.size() - name.size()));
    }
    fetchFrom(fetch, 0);
}

void Checker::fetchFrom(const Fetch &fetch, int index)
{
    if(index >= fetch.mirrors.size())
    {
        if(fetch.optional && fetch.notFound)
        {
            fetch.then(false);
            return;
        }
        fail(QString("%1: %2").arg(fetch.name, fetch.errors.join("; ")));
        return;
    }
    // a document and its signature from the same mirror, as they were written together: a
    // mirror that lags behind can't mix its signature with another mirror's document
    auto base = fetch.mirrors[index];
    auto next = [this, fetch, index, base](Net::Download *download, const QString &error)
    {
        auto tried = fetch;
        if(download->wasNotFound())
        {
            tried.notFound = true;
        }
        tried.errors.append(QString("%1: %2").arg(base, error.isEmpty() ? failure(download) : error));
        fetchFrom(tried, index + 1);
    };
    auto signature = std::make_shared<QByteArray>();
    auto sigDownload = Net::Download::makeByteArray(QUrl(base + fetch.name + ".sig"), signature.get(), Net::Download::Option::NoMirrors);
    sigDownload->addValidator(new Net::SizeValidator(MaxSignatureFileSize));
    sigDownload->setStallTimeout(StallTimeout);
    // the downloads are kept in m_download for as long as they run
    auto sig = sigDownload.get();
    connect(sig, &Net::Download::failed, this, [sig, next]()
    {
        next(sig, QString());
    });
    connect(sig, &Net::Download::succeeded, this, [this, signature, fetch, base, next]()
    {
        auto data = std::make_shared<QByteArray>();
        auto error = std::make_shared<QString>();
        auto download = Net::Download::makeByteArray(QUrl(base + fetch.name), data.get(), Net::Download::Option::NoMirrors);
        download->addValidator(new Net::SizeValidator(fetch.limit));
        download->addValidator(new SignatureValidator(*signature, fetch.trusted, fetch.accept, error));
        download->setStallTimeout(StallTimeout);
        auto doc = download.get();
        connect(doc, &Net::Download::failed, this, [doc, error, next]()
        {
            next(doc, *error);
        });
        // data is held here for as long as the download writes into it
        auto then = fetch.then;
        connect(doc, &Net::Download::succeeded, this, [data, then]()
        {
            then(true);
        });
        m_download = download;
        download->start(m_network);
    });
    m_download = sigDownload;
    sigDownload->start(m_network);
}

void Checker::nextKeys()
{
    auto version = m_keys.version + 1;
    auto name = QString("keys/%1.json").arg(version);
    auto keys = std::make_shared<KeysDocument>();
    auto keysData = std::make_shared<QByteArray>();
    // keys/N+1.json is checked against the root keys of keys/N.json
    auto accept = [version, keys, keysData](const QByteArray &data, QString &error)
    {
        *keysData = data;
        return parseKeys(data, version, *keys, error);
    };
    fetchSigned(name, MaxDocumentSize, m_keys.rootKeys, true, accept, [this, keys, keysData](bool found)
    {
        if(!found)
        {
            afterKeys();
            return;
        }
        m_keys = *keys;
        m_keysData = *keysData;
        nextKeys();
    });
}

void Checker::afterKeys()
{
    // the documents on the way may have expired, the newest one must not have
    auto now = m_config.now.isValid() ? m_config.now : QDateTime::currentDateTimeUtc();
    if(m_keys.expires <= now)
    {
        fail(tr("The newest keys document (version %1) expired at %2, so updates can't be trusted.")
                 .arg(m_keys.version).arg(m_keys.expires.toString(Qt::ISODate)));
        return;
    }
    if(!m_keysData.isEmpty() && m_state.acceptKeys(m_keys, m_keysData))
    {
        qDebug() << "Updater: now on keys version" << m_keys.version;
        m_stateChanged = true;
    }

    auto mirrors = std::make_shared<MirrorsDocument>();
    auto mirrorsData = std::make_shared<QByteArray>();
    auto acceptMirrors = [mirrors, mirrorsData](const QByteArray &data, QString &error)
    {
        *mirrorsData = data;
        return parseMirrors(data, *mirrors, error);
    };
    fetchSigned("mirrors.json", MaxDocumentSize, m_keys.onlineKeys, false, acceptMirrors, [this, mirrors, mirrorsData](bool)
    {
        // an older list is ignored, not refused: mirrors may lag behind
        if(m_state.acceptMirrors(*mirrors, *mirrorsData))
        {
            m_stateChanged = true;
            useMirrors();
        }

        // a mirror that lags behind what was seen already is skipped
        auto acceptTags = [this](const QByteArray &data, QString &error)
        {
            if(!parseTags(data, m_tags, error))
            {
                return false;
            }
            if(m_tags.serial < m_state.serial())
            {
                error = QString("serial %1 is lower than %2, seen already").arg(m_tags.serial).arg(m_state.serial());
                return false;
            }
            return true;
        };
        fetchSigned("tags.json", MaxDocumentSize, m_keys.onlineKeys, false, acceptTags, [this](bool)
        {
            if(m_state.acceptSerial(m_tags.serial))
            {
                m_stateChanged = true;
            }
            m_verifiedTags = m_tags;
            m_haveVerifiedTags = true;
            emit tagsVerified();
            afterTags();
        });
    });
}

void Checker::setPreferredVariant(const QString &variant)
{
    m_preferred = variant;
}

bool Checker::lastVariants(QList<Variant> &runnable, QString &recommended) const
{
    if(m_haveVariants)
    {
        runnable = m_lastRunnable;
        recommended = m_lastRecommended;
    }
    return m_haveVariants;
}

bool Checker::lastTags(TagsDocument &tags) const
{
    if(m_haveVerifiedTags)
    {
        tags = m_verifiedTags;
    }
    return m_haveVerifiedTags;
}

void Checker::afterTags()
{
    m_result.currentWithdrawn = m_tags.withdrawn.contains(m_config.currentBuild);
    if(m_result.currentWithdrawn)
    {
        m_result.messages.append(tr("This build (%1) was withdrawn from the servers.").arg(m_config.currentBuild));
    }
    m_decision = decideTier(m_tags, m_tier, m_config.currentBuild, m_tierSwitched);
    m_result.build = m_decision.build;

    // the tier's build even when it's the installed one: which variants it has decides on a switch
    auto acceptBuild = [this](const QByteArray &data, QString &error)
    {
        return parseBuild(data, m_decision.build, m_build, error);
    };
    fetchSigned(QString("builds/%1.json").arg(m_decision.build), MaxManifestSize, m_keys.onlineKeys, false, acceptBuild, [this](bool)
    {
        for(auto it = m_build.unreadable.begin(); it != m_build.unreadable.end(); it++)
        {
            qDebug() << "Updater: build" << m_build.build << "has a variant this version can't read, left out:" << it.key() << it.value();
        }
        // a variant this version can't read is still in the build: not dropped
        m_result.ownVariantDropped = !m_build.variants.contains(m_config.variant) && !m_build.unreadable.contains(m_config.variant);
        m_result.ownVariantDeprecated = m_build.variants.value(m_config.variant).deprecated;
        m_result.runnable = runnableVariants(m_build, m_config.system);
        auto recommended = chooseVariant(m_build, m_config.variant, QString(), m_config.system);
        if(recommended.found)
        {
            m_result.recommended = recommended.variant.id;
        }
        m_lastRunnable = m_result.runnable;
        m_lastRecommended = m_result.recommended;
        m_haveVariants = true;
        emit variantsKnown();

        auto choice = chooseVariant(m_build, m_config.variant, m_preferred, m_config.system);
        m_result.commit = m_build.commit;
        if(m_decision.kind == TierDecision::BehindInstalled)
        {
            done(CheckResult::BehindInstalled);
            return;
        }
        if(m_decision.kind == TierDecision::NoUpdate)
        {
            // the installed build is the tier's, so it has this variant
            if(!choice.found || choice.variant.id == m_config.variant)
            {
                done(CheckResult::NoUpdate);
                return;
            }
            m_result.variant = choice.variant;
            m_result.haveCurrent = true;
            m_result.current = m_build.variants.value(m_config.variant);
            done(CheckResult::Switch);
            return;
        }
        if(!choice.found)
        {
            done(CheckResult::NotForThisMachine, choice.unmet);
            return;
        }
        m_result.variant = choice.variant;

        // the installed build's files and links, to know what to remove; it may be gone (withdrawn or purged)
        auto current = std::make_shared<BuildManifest>();
        auto acceptCurrent = [this, current](const QByteArray &data, QString &error)
        {
            return parseBuild(data, m_config.currentBuild, *current, error);
        };
        fetchSigned(QString("builds/%1.json").arg(m_config.currentBuild), MaxManifestSize, m_keys.onlineKeys, true, acceptCurrent,
                    [this, current](bool found)
        {
            if(found && current->variants.contains(m_config.variant))
            {
                m_result.haveCurrent = true;
                m_result.current = current->variants.value(m_config.variant);
            }
            done(m_decision.kind == TierDecision::Downgrade ? CheckResult::Downgrade : CheckResult::Update);
        });
    });
}

void Checker::done(CheckResult::Kind kind, const QStringList &messages)
{
    m_download.reset();
    if(kind == CheckResult::Failed)
    {
        // nothing from a check that failed part way counts
        auto build = m_result.build;
        m_result = CheckResult();
        m_result.build = build;
    }
    m_result.kind = kind;
    m_result.tier = m_tier;
    m_result.tierSwitched = m_tierSwitched;
    m_result.notify = m_notify;
    m_result.currentBuild = m_config.currentBuild;
    m_result.ownVariant = m_config.variant;
    m_result.messages.append(messages);
    if(m_stateChanged && !m_config.statePath.isEmpty() && !m_state.save(m_config.statePath))
    {
        qWarning() << "Updater: couldn't save the state to" << m_config.statePath;
    }
    m_checking = false;
    qDebug() << "Updater: check finished:" << kind << "build" << m_result.build << m_result.messages;
    emit finished(m_result);
    if(m_pending && !m_checking)
    {
        m_pending = false;
        check(m_pendingTier, m_pendingTierSwitched, m_pendingNotify);
    }
}

}
