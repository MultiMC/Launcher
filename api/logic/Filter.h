#pragma once

#include <QString>
#include <QRegularExpression>

#include "launcher_logic_export.h"

class LAUNCHER_LOGIC_EXPORT Filter
{
public:
    virtual ~Filter();
    virtual bool accepts(const QString & value) = 0;
};

class LAUNCHER_LOGIC_EXPORT ContainsFilter: public Filter
{
public:
    ContainsFilter(const QString &pattern);
    virtual ~ContainsFilter();
    bool accepts(const QString & value) override;
private:
    QString pattern;
};

class LAUNCHER_LOGIC_EXPORT ExactFilter: public Filter
{
public:
    ExactFilter(const QString &pattern);
    virtual ~ExactFilter();
    bool accepts(const QString & value) override;
private:
    QString pattern;
};

class LAUNCHER_LOGIC_EXPORT RegexpFilter: public Filter
{
public:
    RegexpFilter(const QString &regexp, bool invert);
    virtual ~RegexpFilter();
    bool accepts(const QString & value) override;
private:
    QRegularExpression pattern;
    bool invert = false;
};
