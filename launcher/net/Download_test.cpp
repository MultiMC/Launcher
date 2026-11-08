#include <QTest>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "TestUtil.h"
#include "net/Download.h"
#include "net/SizeValidator.h"
#include "net/ChecksumValidator.h"
#include "net/Mirrors.h"

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
    QString base()
    {
        return url("/").toString();
    }
    /// answers path with this content, besides the fixed script
    void serve(const QString &path, const QByteArray &content)
    {
        m_files[path] = content;
    }
    /// how many requests it has answered
    int requests() const
    {
        return m_count;
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
        m_count++;
        if(m_files.contains(path))
        {
            const auto content = m_files[path];
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(content.size()) + "\r\nConnection: close\r\n\r\n" + content);
            socket->disconnectFromHost();
        }
        else if(path == "/hello")
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
        else if(path == "/missing-page")
        {
            // a 404 with a large error page, as R2's custom domains send
            const QByteArray page(27000, 'x');
            socket->write("HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(page.size()) + "\r\nConnection: close\r\n\r\n" + page);
            socket->disconnectFromHost();
        }
        else
        {
            socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\nnot found");
            socket->disconnectFromHost();
        }
    }

    QTcpServer m_server;
    QMap<QTcpSocket *, QByteArray> m_requests;
    QMap<QString, QByteArray> m_files;
    int m_count = 0;
};

class DownloadTest : public QObject
{
    Q_OBJECT

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<TestServer> m_server;
    std::unique_ptr<TestServer> m_copy;

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
        m_copy.reset(new TestServer());
        // a site: the server, then its copy
        Net::Mirrors::setSites({{m_server->base(), m_copy->base()}});
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

    // the error page doesn't count against a size limit: the download is not found, not too large
    void test_notFoundWithALargeErrorPage()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/missing-page"), &data, Net::Download::Option::NoMirrors);
        download->addValidator(new Net::SizeValidator(100));
        QVERIFY(!run(download));
        QVERIFY(download->wasNotFound());
        QCOMPARE(download->httpStatus(), 404);
        QVERIFY(data.isEmpty());
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

    void test_mirrorCandidates()
    {
        const auto url = m_server->url("/a%20b/c?d=1");
        QCOMPARE(Net::Mirrors::candidates(url), QList<QUrl>({url, m_copy->url("/a%20b/c?d=1")}));
        // the copy's URL is on the same site: the main server still comes first
        QCOMPARE(Net::Mirrors::candidates(m_copy->url("/x")), QList<QUrl>({m_server->url("/x"), m_copy->url("/x")}));
        QCOMPARE(Net::Mirrors::candidates(QUrl("https://example.org/x")), QList<QUrl>({QUrl("https://example.org/x")}));
    }

    // a signed mirror list: its groups in order, then the site's own base URLs it doesn't have
    void test_mirrorList()
    {
        const auto reverse = [](QStringList &list) { std::reverse(list.begin(), list.end()); };
        Net::Mirrors::setShuffle(reverse);
        Net::Mirrors::setMirrorList(m_server->base(), {{"https://a.example/"}, {"https://b.example/", m_copy->base()}});
        const QList<QUrl> expected{QUrl("https://a.example/x"), m_copy->url("/x"), QUrl("https://b.example/x"), m_server->url("/x")};
        QCOMPARE(Net::Mirrors::candidates(m_server->url("/x")), expected);
        // a URL on a mirror from the list is on the site too
        QCOMPARE(Net::Mirrors::candidates(QUrl("https://b.example/x")), expected);

        // a group keeps its place whatever the order within it
        Net::Mirrors::setShuffle(nullptr);
        for(int i = 0; i < 20; i++)
        {
            auto candidates = Net::Mirrors::candidates(m_server->url("/x"));
            QCOMPARE(candidates.first(), QUrl("https://a.example/x"));
            QCOMPARE(QSet<QUrl>({candidates[1], candidates[2]}), QSet<QUrl>({QUrl("https://b.example/x"), m_copy->url("/x")}));
        }

        // an empty list goes back to the site's own base URLs
        Net::Mirrors::setMirrorList(m_server->base(), {});
        QCOMPARE(Net::Mirrors::candidates(m_server->url("/x")), QList<QUrl>({m_server->url("/x"), m_copy->url("/x")}));
        // a list for a site that doesn't exist does nothing
        Net::Mirrors::setMirrorList("https://nowhere.example/", {{"https://a.example/"}});
        QCOMPARE(Net::Mirrors::candidates(QUrl("https://a.example/x")), QList<QUrl>({QUrl("https://a.example/x")}));
    }

    void test_mirrorListUsed()
    {
        m_copy->serve("/file", "copy");
        Net::Mirrors::setMirrorList(m_server->base(), {{m_copy->base()}});
        QByteArray data;
        QVERIFY(run(Net::Download::makeByteArray(m_server->url("/file"), &data)));
        QCOMPARE(data, QByteArray("copy"));
        QCOMPARE(m_server->requests(), 0);
    }

    void test_mainServerFirst()
    {
        m_server->serve("/file", "main");
        m_copy->serve("/file", "copy");
        QByteArray data;
        QVERIFY(run(Net::Download::makeByteArray(m_server->url("/file"), &data)));
        QCOMPARE(data, QByteArray("main"));
        QCOMPARE(m_copy->requests(), 0);
    }

    // a copy behind on a sync may not have a file yet; a main server may lose one
    void test_mirrorWhenNotFound()
    {
        m_copy->serve("/file", "copy");
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/file"), &data);
        QVERIFY(run(download));
        QCOMPARE(data, QByteArray("copy"));
        QCOMPARE(download->url(), m_copy->url("/file"));
        // a 404 doesn't make the main server unreachable: the next request goes there first
        QCOMPARE(Net::Mirrors::candidates(m_server->url("/x")).first(), m_server->url("/x"));
    }

    void test_mirrorWhenRefused()
    {
        m_server->serve("/file", "bad");
        m_copy->serve("/file", "good");
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/file"), &data);
        download->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, QCryptographicHash::hash("good", QCryptographicHash::Sha1)));
        QVERIFY(run(download));
        QCOMPARE(data, QByteArray("good"));
    }

    void test_mirrorWhenTooLarge()
    {
        m_copy->serve("/huge", "small");
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/huge"), &data);
        download->addValidator(new Net::SizeValidator(1000));
        QVERIFY(run(download));
        QCOMPARE(data, QByteArray("small"));
    }

    void test_mirrorToFile()
    {
        m_copy->serve("/file", "copy");
        QTemporaryDir dir;
        auto path = dir.path() + "/file";
        QVERIFY(run(Net::Download::makeFile(m_server->url("/file"), path)));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("copy"));
    }

    void test_allMirrorsFail()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/missing"), &data);
        QVERIFY(!run(download));
        QVERIFY(download->wasNotFound());
        QCOMPARE(m_server->requests(), 1);
        QCOMPARE(m_copy->requests(), 1);
    }

    // a main server that can't be reached is tried last for a while, so it doesn't cost every request a failure
    void test_unreachableTriedLast()
    {
        // nothing listens on port 1
        const QString down = "http://127.0.0.1:1/";
        Net::Mirrors::setSites({{down, m_copy->base()}});
        m_copy->serve("/file", "copy");
        QByteArray data;
        QVERIFY(run(Net::Download::makeByteArray(QUrl(down + "file"), &data)));
        QCOMPARE(data, QByteArray("copy"));
        QCOMPARE(Net::Mirrors::candidates(QUrl(down + "x")), QList<QUrl>({m_copy->url("/x"), QUrl(down + "x")}));

        // when the copy fails too, it is still tried
        QVERIFY(!run(Net::Download::makeByteArray(QUrl(down + "missing"), &data)));
        QCOMPARE(m_copy->requests(), 2);
    }

    // a mirror that answers knows better than one that can't be reached
    void test_notFoundWhenOthersUnreachable()
    {
        Net::Mirrors::setSites({{m_server->base(), "http://127.0.0.1:1/"}});
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/missing"), &data);
        QVERIFY(!run(download));
        QCOMPARE(download->url(), QUrl("http://127.0.0.1:1/missing"));
        QVERIFY(download->wasNotFound());
    }

    // NetJob retries a failed download by starting it again: from the first mirror again
    void test_retryStartsOver()
    {
        QByteArray data;
        auto download = Net::Download::makeByteArray(m_server->url("/file"), &data);
        QVERIFY(!run(download));
        m_server->serve("/file", "main");
        QVERIFY(run(download));
        QCOMPARE(data, QByteArray("main"));
        QCOMPARE(download->url(), m_server->url("/file"));
    }
};

QTEST_GUILESS_MAIN(DownloadTest)

#include "Download_test.moc"
