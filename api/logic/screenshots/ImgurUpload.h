#pragma once
#include "net/NetAction.h"
#include "Screenshot.h"

#include "launcher_logic_export.h"

typedef std::shared_ptr<class ImgurUpload> ImgurUploadPtr;
class LAUNCHER_LOGIC_EXPORT ImgurUpload : public NetAction
{
public:
    explicit ImgurUpload(ScreenshotPtr shot);
    static ImgurUploadPtr make(ScreenshotPtr shot)
    {
        return ImgurUploadPtr(new ImgurUpload(shot));
    }

protected
slots:
    virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    virtual void downloadError(QNetworkReply::NetworkError error);
    virtual void downloadFinished();
    virtual void downloadReadyRead()
    {
    }

public
slots:
    virtual void start();

private:
    ScreenshotPtr m_shot;
    bool finished = true;
};
