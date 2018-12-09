#pragma once

#include <QObject>
#include <QByteArray>
#include <QTranslator>

#include <memory>
#include "multimc_logic_export.h"

class QLocale;
class MMCTranslatorPrivate;

class MULTIMC_LOGIC_EXPORT MMCTranslator : public QTranslator
{
    Q_OBJECT
public:
    explicit MMCTranslator(QObject *parent = nullptr);
    virtual ~MMCTranslator();

    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;

    bool isEmpty() const override;

    // bool fromQM(QString path);
    // bool fromLinguistQM(QString path);
    // bool fromPO(QString path);
    bool load(const QString & filename, const QString & directory = QString(), const QString & search_delimiters = QString(), const QString & suffix = QString());
    bool load(const QLocale & locale, const QString & filename, const QString & prefix = QString(), const QString & directory = QString(), const QString & suffix = QString());
    bool load(const uchar *data, int len, const QString &directory = QString());

private:
    std::unique_ptr<MMCTranslatorPrivate> d;
};
