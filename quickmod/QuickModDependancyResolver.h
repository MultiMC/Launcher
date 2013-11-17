#pragma once

#include <QObject>
#include <QList>
#include "QuickModFile.h"
#include "logic/BaseInstance.h"

class QuickModDependancyResolver : public QObject
{
    Q_OBJECT
public:
    explicit QuickModDependancyResolver(QObject *parent = 0);
    QList<QuickModFile> Resolve(BaseInstance* instance, QList<QuickModFile> files);

signals:

public slots:

};
