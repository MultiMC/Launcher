#include "LiveCheck.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

#include <LocalPeer.h>

bool LiveCheck::isRunning(const QString &key)
{
    LocalPeer peer(nullptr, ApplicationId::fromRawString(key));
    return peer.isClient();
}

bool LiveCheck::waitFor(const QString &file, int timeoutMs, const IsRunning &running)
{
    QElapsedTimer elapsed;
    elapsed.start();
    QEventLoop loop;
    QTimer poll;
    poll.setInterval(250);
    bool started = false;
    QObject::connect(&poll, &QTimer::timeout, [&]() {
        QFile check(file);
        if (check.open(QIODevice::ReadOnly))
        {
            const QString key = QString::fromUtf8(check.readAll()).trimmed();
            // empty: the new build created the file but hasn't written its key yet
            if (!key.isEmpty())
            {
                started = running(key);
                if (started)
                    qDebug() << "Found process started with key" << key << "after" << elapsed.elapsed() << "ms";
                else
                    qWarning() << "Process started with key" << key << "apparently died or is not responding";
                loop.quit();
                return;
            }
        }
        if (elapsed.elapsed() >= timeoutMs)
        {
            qWarning() << "No key in" << file << "after" << timeoutMs << "ms: the new build didn't start";
            loop.quit();
        }
    });
    poll.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    return started;
}
