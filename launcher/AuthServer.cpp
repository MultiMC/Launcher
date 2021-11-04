#include "AuthServer.h"

#include <QThread>
#include <QDebug>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

struct Request
{
    QString method;
    QString url;
    QMap<QString, QString> headers;
    QByteArray body;

    QJsonDocument json() {
        return QJsonDocument::fromJson(body.data());
    }
};

struct Response
{
    int statusCode;
    QMap<QString, QString> headers;
    QString body;
};

enum ConnectionState
{
    CREATING,
    READING_HEAD,
    READING_BODY,
    PROCESS_REQUEST
};

struct Connection
{
    ConnectionState state;
    Request *request;
    Response *response;
    int leftToRead;
    QByteArray buffer;
};

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

void processRequest(Request *request, Response *response)
{
    qDebug() << "Processing request";
    if (request->url == "/")
    {
        response->body = "{\"Status\":\"OK\",\"Runtime-Mode\":\"productionMode\",\"Application-Author\":\"Mojang Web Force\",\"Application-Description\":\"Mojang Public API.\",\"Specification-Version\":\"3.58.0\",\"Application-Name\":\"yggdrasil.accounts.restlet.server.public\",\"Implementation-Version\":\"3.58.0_build194\",\"Application-Owner\":\"Mojang\"}";
        response->statusCode = 200;
        response->headers["Content-Type"] = "application/json; charset=utf-8";
        return;
    }

    if (request->url == "/sessionserver/session/minecraft/join" || request->url == "/sessionserver/session/minecraft/hasJoined")
    {
        response->statusCode = 204;
        return;
    }

    if (request->url == "/auth/authenticate" || request->url == "/auth/refresh")
    {
        auto json = request->json().object();
        QString clientToken = json.value("clientToken").toString();
        QString username = json.value(request->url == "/auth/authenticate" ? "username" : "accessToken").toString();

        QString profile = ((QString) "{\"id\":\"%1\",\"name\":\"%2\"}").arg(clientToken, username);

        response->statusCode = 200;
        response->body = ((QString) "{\"accessToken\":\"%1\",\"clientToken\":\"%2\",\"availableProfiles\":[%3], \"selectedProfile\": %3}").arg(username, clientToken, profile);
        return;
    }

    response->body = "Not found";
    response->statusCode = 404;
}

void AuthServer::newConnection()
{

    QTcpSocket *tcpSocket = m_tcpServer->nextPendingConnection();
    Connection *connection = new Connection();

    connect(tcpSocket, &QTcpSocket::readyRead, this, [tcpSocket, connection]()
            {
                // Not the best way to process queries, but it just works
                QByteArray curBuf = tcpSocket->readAll().data();
                qDebug() << "Read " << curBuf.size() << " bytes";

                if (connection->state == CREATING)
                {
                    connection->response = new Response();
                    connection->request = new Request();
                    connection->buffer = ((QString)"").toUtf8();
                    connection->state = READING_HEAD;
                }

                if (connection->state == READING_HEAD)
                {
                    connection->buffer.append(curBuf);
                    if (connection->buffer.contains("\r\n\r\n"))
                    {
                        QByteArray head = connection->buffer.left(connection->buffer.indexOf("\r\n\r\n"));
                        QString headStr = head.data();
                        QStringList headList = headStr.split("\r\n");
                        QStringList firstLine = headList.at(0).split(" ");
                        connection->request->method = firstLine.at(0);
                        connection->request->url = firstLine.at(1);

                        for (int i = 1; i < headList.size(); i++)
                        {
                            QStringList header = headList.at(i).split(":");
                            connection->request->headers.insert(header.at(0), header.at(1));
                        }

                        if (connection->request->headers.contains("Content-Length"))
                        {
                            connection->leftToRead = connection->request->headers["Content-Length"].toInt();
                        }
                        else
                        {
                            connection->leftToRead = 0;
                        }

                        curBuf = connection->buffer.mid(connection->buffer.indexOf("\r\n\r\n") + 4);
                        connection->state = READING_BODY;
                    }
                }

                if (connection->state == READING_BODY)
                {
                    connection->request->body.append(curBuf);
                    if (connection->request->body.size() >= connection->leftToRead)
                    {
                        connection->state = PROCESS_REQUEST;
                    }
                }

                if(connection->state == PROCESS_REQUEST){
                    processRequest(connection->request, connection->response);

                    if(connection->response->body.size() > 0){
                        connection->response->headers["Content-Length"] = QString::number(connection->response->body.size());
                    }
                    connection->response->headers["Connection"] = "Keep-Alive";

                    QString responseStatusText = "Internal Server Error";
                    if (connection->response->statusCode == 200)
                        responseStatusText = "OK";
                    else if (connection->response->statusCode == 204)
                        responseStatusText = "No Content";
                    else if (connection->response->statusCode == 404)
                        responseStatusText = "Not Found";

                    
                    QString responseHead = ((QString)"HTTP/1.1 %1 %2\r\n").arg(connection->response->statusCode).arg(responseStatusText);
                    for (auto h: connection->response->headers.keys())
                    {
                        responseHead += ((QString)"%1: %2\r\n").arg(h, connection->response->headers[h]);
                    }
                    responseHead += "\r\n";
                    tcpSocket->write(responseHead.toUtf8());
                    tcpSocket->write(connection->response->body.toUtf8());
                    connection->state = CREATING;
                }
            });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [tcpSocket]()
            { tcpSocket->close(); });
}
