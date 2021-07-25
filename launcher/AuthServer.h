#pragma once

#include <QString>
#include <QTcpServer>
#include "settings/SettingsObject.h"
#include "logic_export.h"

class LOGIC_EXPORT AuthServer: public QObject
{
public:
    explicit AuthServer(QObject *parent = 0);

    quint16 port();

private:
    void newConnection();

private:
    std::shared_ptr<QTcpServer> m_tcpServer;
};
