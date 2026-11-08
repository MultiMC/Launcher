#pragma once
#include <QString>
#include <QStringList>

#include "Formats.h"

/**
 * Updater v2's decisions, apart from the network: which build a tier gives, and which
 * variant of a build this machine installs. What changes on disk is planned in Install.h.
 */
namespace UpdaterV2
{

enum class Tier
{
    Latest,
    Testing,
    Stable
};

// `latest`, `testing`, `stable`: the names in the tags document and in the settings
QString tierName(Tier tier);
bool parseTier(const QString &name, Tier &tier);

struct TierDecision
{
    enum Kind
    {
        // the tier's build is the installed one
        NoUpdate,
        // the tier's build is newer
        Update,
        // the tier's build is older, and the user switched to this tier: offered with a warning
        Downgrade,
        // the tier's build is older, and the user didn't ask for it: nothing is offered
        BehindInstalled
    } kind = NoUpdate;
    qint64 build = 0;
};

/**
 * What the tags offer an installed build on a tier. A Launcher never goes to an older build
 * unless `tierSwitched` (the user just switched to this tier).
 * Testing resolves to Stable when there is no newer Testing build.
 */
TierDecision decideTier(const TagsDocument &tags, Tier tier, qint64 currentBuild, bool tierSwitched);

struct VariantChoice
{
    bool found = false;
    Variant variant;
    // when nothing was found: why, for the user
    QStringList unmet;
};

/**
 * Which variant of a build this machine installs. `preferred` is the one the user chose to stay
 * on or go back to (empty: none); while the build has it and the machine meets it, it's that one.
 * Otherwise, following the chain of successors from the Launcher's own variant, the furthest one
 * whose requirements the machine meets; otherwise its own variant, if the build has it and the
 * machine meets it.
 */
VariantChoice chooseVariant(const BuildManifest &build, const QString &ownVariant, const QString &preferred,
                            const SystemRequirements::System &system);

// the variants of a build the machine meets the requirements of, in ID order
QList<Variant> runnableVariants(const BuildManifest &build, const SystemRequirements::System &system);

}
