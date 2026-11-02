#pragma once

#include "Validator.h"

namespace Net {
/*
 * Refuses a download as soon as it gets larger than a maximum, before the rest of it arrives.
 */
class SizeValidator: public Validator
{
public: /* con/des */
    SizeValidator(qint64 maximum)
        :m_maximum(maximum)
    {
    };
    virtual ~SizeValidator() {};

public: /* methods */
    bool init(QNetworkRequest &) override
    {
        m_size = 0;
        return true;
    }
    bool write(QByteArray & data) override
    {
        m_size += data.size();
        if(m_size > m_maximum)
        {
            qWarning() << "Download is larger than" << m_maximum << "bytes, refusing it.";
            return false;
        }
        return true;
    }
    bool abort() override
    {
        return true;
    }
    bool validate(QNetworkReply &) override
    {
        return m_size <= m_maximum;
    }

private: /* data */
    qint64 m_maximum;
    qint64 m_size = 0;
};
}
