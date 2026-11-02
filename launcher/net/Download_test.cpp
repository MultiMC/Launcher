#include <QTest>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "TestUtil.h"
#include "net/Download.h"
#include "net/SizeValidator.h"

/*
 * A minimal HTTP server on localhost: it answers each request by its path, from a fixed script.
 */
class TestServer : public QObject
{
    Q_OBJECT
public:
    TestServer()
    {
        m_server.listen(QHostAddress::LocalHost);
        connect(&m_server, &QTcpServer::newConnection, this, &TestServer::accept);
    }
    QUrl url(const QString &path)
    {
        return QUrl(QString("http://127.0.0.1:%1%2").arg(m_server.serverPort()).arg(path));
    }

private slots:
    void accept()
    {
        while(auto socket = m_server.nextPendingConnection())
        {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]()
            {
                m_requests[socket] += socket->readAll();
                if(m_requests[socket].contains("\r\n\r\n"))
                {
                    respond(socket, QString::fromLatin1(m_requests[socket].split(' ').value(1)));
                }
            });
        }
    }

private:
    void respond(QTcpSocket *socket, const QString &path)
    {
        if(path == "/hello")
        {
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\n\r\nhello");
            socket->disconnectFromHost();
        }
        else if(path == "/stall")
        {
            // 10 of the 100 bytes it promises, then nothing
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n0123456789");
        }
        else if(path == "/huge")
        {
            // half of what it promises, then nothing: only refusing it ends the download
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2000000\r\n\r\n");
            socket->write(QByteArray(1000000, 'x'));
        }
        else
        {
            socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\nnot found");
            socket->disconnectFromHost();
        }
    }

    QTcpServer m_server;
    QMap<QTcpSocket *, QByteArray> m_requests;
};

class DownloadTest : public QObject
{
    Q_OBJECT

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<TestServer> m_server;

    // runs a download to its end; true when it succeeded
    bool run(Net::Download::Ptr download, int timeout = 10000)
    {
        bool done = false, succeeded = false;
        QList<QMetaObject::Connection> connections;
        connections << connect(download.get(), &Net::Download::succeeded, [&]() { done = succeeded = true; });
        connections << connect(download.get(), &Net::Download::failed, [&]() { done = true; });
        connections << connect(download.get(), &Net::Download::aborted, [&]() { done = true; });
        download->start(m_network);
        QElapsedTimer timer;
        timer.start();
        while(!done && timer.elapsed() < timeout)
        {
            QTest::qWait(10);
        }
        if(!done)
        {
            qWarning() << "The download didn't end";
        }
        for(auto &connection: connections)
        {
            disconnect(connection);
        }
        return succeeded;
    }

private
slots:
    void init()
    {
        m_network.reset(new QNetworkAccessManager());
        m_server.reset(new TestServer());
    }

    void test_succeeds()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/hello"), &data);
        QVERIFY(run(download));
        QCOMPARE(data, QByteArray("hello"));
        QCOMPARE(download->httpStatus(), 200);
        QCOMPARE(download->networkError(), QNetworkReply::NoError);
        QVERIFY(!download->wasNotFound());
    }

    void test_notFound()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/missing"), &data);
        QVERIFY(!run(download));
        QVERIFY(download->wasNotFound());
        QCOMPARE(download->httpStatus(), 404);
    }

    void test_localFileNotFound()
    {
        QTemporaryDir dir;
        QByteArray data;
        auto download = Net::Download::makeByteArray(QUrl::fromLocalFile(dir.path() + "/missing"), &data);
        QVERIFY(!run(download));
        QVERIFY(download->wasNotFound());
        QCOMPARE(download->httpStatus(), 0);
    }

    void test_otherFailureIsNotNotFound()
    {
        QByteArray data;
        // nothing listens on port 1
        auto download = Net::Download::makeByteArray(QUrl("http://127.0.0.1:1/hello"), &data);
        QVERIFY(!run(download));
        QVERIFY(!download->wasNotFound());
        QVERIFY(download->networkError() != QNetworkReply::NoError);
    }

    void test_sizeLimit()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/hello"), &data);
        download->addValidator(new Net::SizeValidator(5));
        QVERIFY(run(download));

        download = Net::Download::makeByteArray(m_server->url("/hello"), &data);
        download->addValidator(new Net::SizeValidator(4));
        QVERIFY(!run(download));
        QVERIFY(data.isEmpty());
    }

    // the response never ends; refusing the data has to end the download
    void test_sizeLimitStopsTheTransfer()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/huge"), &data);
        download->addValidator(new Net::SizeValidator(1000));
        QElapsedTimer timer;
        timer.start();
        QVERIFY(!run(download));
        QVERIFY(timer.elapsed() < 5000);
        QVERIFY(data.isEmpty());
        QVERIFY(!download->wasNotFound());
    }

    void test_sizeLimitToFile()
    {
        QTemporaryDir dir;
        auto path = dir.path() + "/huge";
        auto download = Net::Download::makeFile(m_server->url("/huge"), path);
        download->addValidator(new Net::SizeValidator(1000));
        QVERIFY(!run(download));
        QVERIFY(!QFile::exists(path));
    }

    void test_stallTimeout()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/stall"), &data);
        download->setStallTimeout(300);
        QElapsedTimer timer;
        timer.start();
        QVERIFY(!run(download));
        QVERIFY(timer.elapsed() < 5000);
        QCOMPARE(download->networkError(), QNetworkReply::TimeoutError);
        QVERIFY(data.isEmpty());
    }

    // without a stall timeout, a stalled transfer is still waited for, as before
    void test_noStallTimeoutByDefault()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/stall"), &data);
        bool done = false;
        connect(download.get(), &Net::Download::failed, [&]() { done = true; });
        connect(download.get(), &Net::Download::succeeded, [&]() { done = true; });
        download->start(m_network);
        QTest::qWait(1000);
        download->disconnect();
        QVERIFY(!done);
        download->abort();
    }
};

QTEST_GUILESS_MAIN(DownloadTest)

#include "Download_test.moc"
