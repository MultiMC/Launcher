#include "AuthServer.h"

#include <QDebug>
#include <QTcpSocket>

AuthServer::AuthServer(QObject *parent) : QObject(parent)
{
    m_tcpServer.reset(new QTcpServer(this));

    connect(m_tcpServer.get(), &QTcpServer::newConnection, this, &AuthServer::newConnection);

    if (!m_tcpServer->listen(QHostAddress::LocalHost))
    {
        // TODO: think about stop launching when server start fails
        qCritical() << "Auth server start failed";
    }
}

quint16 AuthServer::port()
{
    return m_tcpServer->serverPort();
}

void AuthServer::newConnection()
{
    QTcpSocket *tcpSocket = m_tcpServer->nextPendingConnection();

    connect(tcpSocket, &QTcpSocket::readyRead, this, [tcpSocket]()
            {
            // Not the best way to process queries, but it just works
            QString rawRequest = tcpSocket->readAll().data();
            QStringList requestLines = rawRequest.split("\r\n");
            QString requestPath = requestLines[0].split(" ")[1];

            int responseStatusCode = 500;
            QString responseBody = "";
            QStringList responseHeaders;

            responseHeaders << "Connection: keep-alive";

            if (requestPath == "/")
            {
            responseBody = "{\"Status\":\"OK\",\"Runtime-Mode\":\"productionMode\",\"Application-Author\":\"Mojang Web Force\",\"Application-Description\":\"Mojang Public API.\",\"Specification-Version\":\"3.58.0\",\"Application-Name\":\"yggdrasil.accounts.restlet.server.public\",\"Implementation-Version\":\"3.58.0_build194\",\"Application-Owner\":\"Mojang\"}";
            responseStatusCode = 200;
            responseHeaders << "Content-Type: application/json; charset=utf-8";
            }
            else if (requestPath == "/sessionserver/session/minecraft/join" || requestPath == "/sessionserver/session/minecraft/hasJoined")
            {
                responseStatusCode = 204;
            }
            else
            {
                responseBody = "Not found";
                responseStatusCode = 404;
            }

            QString responseStatusText = "Internal Server Error";
            if (responseStatusCode == 200)
                responseStatusText = "OK";
            else if (responseStatusCode == 204)
                responseStatusText = "No Content";
            else if (responseStatusCode == 404)
                responseStatusText = "Not Found";

            if (responseBody.length() != 0)
            {
                responseHeaders << ((QString) "Content-Length: %1").arg(responseBody.length());
            }

            tcpSocket->write(((QString) "HTTP/1.1 %1 %2\r\nConnection: keep-alive\r\n").arg(responseStatusCode).arg(responseStatusText).toUtf8());
            tcpSocket->write(responseHeaders.join("\r\n").toUtf8());
            tcpSocket->write("\r\n\r\n");
            tcpSocket->write(responseBody.toUtf8());
            });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [tcpSocket]()
            {
            tcpSocket->close();
            });
}
