#include "QuickModDependancyResolver.h"
#include <QMap>

QuickModDependancyResolver::QuickModDependancyResolver(QObject *parent) :
    QObject(parent)
{
}

QList<QuickModFile> QuickModDependancyResolver::Resolve(BaseInstance *instance, QList<QuickModFile> files)
{
    QList<QString> installedMods();
    QMap<QString, QuickModFile> quickMods();
}
