#include <QTest>
#include <QDataStream>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QThread>
#include <QUuid>

#include <atomic>
#include <cstdio>
#include <cstdlib>

#include "LocalPeer.h"

// ends the test process when the test thread doesn't get done in time: a hang can't be caught otherwise
class Watchdog : public QThread
{
public:
    std::atomic<bool> done{false};

protected:
    void run() override
    {
        for(int i = 0; i < 200 && !done; i++)
        {
            msleep(50);
        }
        if(!done)
        {
            fprintf(stderr, "receiving a connection without a message hangs\n");
            std::_Exit(1);
        }
    }
};

// sends a message like another process would, on a thread of its own: a named pipe on
// Windows only completes a write once the receiving side reads it
class Sender : public QThread
{
public:
    Sender(const QString &name, const QByteArray &message) : m_name(name), m_message(message)
    {
    }
    // the receiving side's answer
    QByteArray ack;

protected:
    void run() override
    {
        QLocalSocket socket;
        socket.connectToServer(m_name);
        if(!socket.waitForConnected(5000))
        {
            return;
        }
        QDataStream ds(&socket);
        ds.writeBytes(m_message.constData(), m_message.size());
        if(socket.waitForBytesWritten(5000) && socket.waitForReadyRead(5000))
        {
            ack = socket.readAll();
        }
    }

private:
    QString m_name;
    QByteArray m_message;
};

class LocalPeerTest : public QObject
{
    Q_OBJECT

    // a peer of its own for each test, listening
    ApplicationId uniqueId()
    {
        return ApplicationId::fromCustomId(QString::fromLatin1(QUuid::createUuid().toRfc4122().toHex()));
    }

private
slots:
    void test_message()
    {
        auto id = uniqueId();
        LocalPeer server(nullptr, id);
        QVERIFY(!server.isClient());
        QSignalSpy received(&server, &LocalPeer::messageReceived);

        QByteArray message("activate");
        Sender sender(id.toString(), message);
        sender.start();
        QVERIFY(received.wait(5000));
        QVERIFY(sender.wait(5000));
        QCOMPARE(received.first().first().toByteArray(), message);
        QCOMPARE(sender.ack, QByteArray("ack"));
    }

    // a sender that gave up and went away before its message arrived (it waits for the ack
    // only so long): the receiving side carries on
    void test_senderGone()
    {
        Watchdog watchdog;
        watchdog.start();

        auto id = uniqueId();
        LocalPeer server(nullptr, id);
        QVERIFY(!server.isClient());
        QSignalSpy received(&server, &LocalPeer::messageReceived);
        {
            QLocalSocket socket;
            socket.connectToServer(id.toString());
            QVERIFY(socket.waitForConnected(1000));
            // part of the length, then gone
            socket.write("\0\0", 2);
            socket.waitForBytesWritten(1000);
            socket.disconnectFromServer();
        }
        QTest::qWait(500);
        watchdog.done = true;
        watchdog.wait();
        QCOMPARE(received.size(), 0);
    }
};

QTEST_GUILESS_MAIN(LocalPeerTest)

#include "LocalPeer_test.moc"
