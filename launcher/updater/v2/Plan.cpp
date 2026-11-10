#include "Plan.h"

#include <QObject>

#include <algorithm>

namespace UpdaterV2
{

QString tierName(Tier tier)
{
    switch(tier)
    {
        case Tier::Latest:
            return "latest";
        case Tier::Testing:
            return "testing";
        case Tier::Stable:
        default:
            return "stable";
    }
}

bool parseTier(const QString &name, Tier &tier)
{
    for(auto t: {Tier::Latest, Tier::Testing, Tier::Stable})
    {
        if(name == tierName(t))
        {
            tier = t;
            return true;
        }
    }
    return false;
}

TierDecision decideTier(const TagsDocument &tags, Tier tier, qint64 currentBuild, bool tierSwitched)
{
    TierDecision out;
    switch(tier)
    {
        case Tier::Latest:
            out.build = tags.latest;
            break;
        case Tier::Testing:
            // the publisher writes it resolved already; the documents can't say otherwise, this is for clarity
            out.build = std::max(tags.testing, tags.stable);
            break;
        case Tier::Stable:
            out.build = tags.stable;
            break;
    }
    if(out.build > currentBuild)
    {
        out.kind = TierDecision::Update;
    }
    else if(out.build == currentBuild)
    {
        out.kind = TierDecision::NoUpdate;
    }
    else
    {
        out.kind = tierSwitched ? TierDecision::Downgrade : TierDecision::BehindInstalled;
    }
    return out;
}

VariantChoice chooseVariant(const BuildManifest &build, const QString &ownVariant, const QString &preferred,
                            const SystemRequirements::System &system)
{
    VariantChoice out;
    if(!preferred.isEmpty() && build.variants.contains(preferred))
    {
        auto chosen = build.variants.value(preferred);
        if(SystemRequirements::unmetRequirements(chosen.requirements, system).isEmpty())
        {
            out.found = true;
            out.variant = chosen;
            return out;
        }
    }
    QMap<QString, QString> successor;
    for(auto &v: build.variants)
    {
        for(auto &other: v.supersedes)
        {
            successor.insert(other, v.id);
        }
    }
    // the manifest has no loops in it (checked when it was parsed)
    QStringList chain;
    auto current = ownVariant;
    while(successor.contains(current))
    {
        current = successor.value(current);
        chain.append(current);
    }

    QStringList firstUnmet;
    for(int i = chain.size() - 1; i >= 0; i--)
    {
        auto candidate = build.variants.value(chain[i]);
        auto unmet = SystemRequirements::unmetRequirements(candidate.requirements, system);
        if(unmet.isEmpty())
        {
            out.found = true;
            out.variant = candidate;
            return out;
        }
        if(i == 0)
        {
            firstUnmet = unmet;
        }
    }
    if(build.variants.contains(ownVariant))
    {
        auto own = build.variants.value(ownVariant);
        auto unmet = SystemRequirements::unmetRequirements(own.requirements, system);
        if(unmet.isEmpty())
        {
            out.found = true;
            out.variant = own;
            return out;
        }
        out.unmet = unmet;
        return out;
    }
    if(!firstUnmet.isEmpty())
    {
        out.unmet = firstUnmet;
        return out;
    }
    if(build.unreadable.contains(ownVariant))
    {
        out.unmet.append(QObject::tr("Build %1 describes the version for this installation (%2) in a way this version can't read.")
                             .arg(build.build).arg(ownVariant));
        return out;
    }
    out.unmet.append(QObject::tr("Build %1 has no version for this installation (%2).").arg(build.build).arg(ownVariant));
    return out;
}

QList<Variant> runnableVariants(const BuildManifest &build, const SystemRequirements::System &system)
{
    QList<Variant> out;
    for(auto &v: build.variants)
    {
        if(SystemRequirements::unmetRequirements(v.requirements, system).isEmpty())
        {
            out.append(v);
        }
    }
    return out;
}

}
