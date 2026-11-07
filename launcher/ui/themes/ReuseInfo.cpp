#include "ReuseInfo.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

ReuseInfo ReuseInfo::read(const QString &file)
{
    ReuseInfo info;
    QFile licenseFile(file + ".license");
    if (!licenseFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return info;
    QTextStream in(&licenseFile);
    in.setCodec("UTF-8");
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.startsWith("SPDX-FileCopyrightText:") && info.copyright.isEmpty())
        {
            // without the email address people often add
            info.copyright = line.mid(int(strlen("SPDX-FileCopyrightText:"))).remove(QRegularExpression("<[^>]*>")).trimmed();
        }
        else if (line.startsWith("SPDX-License-Identifier:") && info.license.isEmpty())
        {
            info.license = line.mid(int(strlen("SPDX-License-Identifier:"))).trimmed();
        }
    }
    return info;
}

QString ReuseInfo::summary() const
{
    QStringList parts;
    if (!copyright.isEmpty())
        parts.append(copyright);
    if (!license.isEmpty())
        parts.append(license);
    return parts.join(QString::fromUtf8(" \xC2\xB7 "));
}
