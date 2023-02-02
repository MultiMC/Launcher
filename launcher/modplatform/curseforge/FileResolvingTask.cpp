#include "FileResolvingTask.h"
#include "Application.h"

#include "Json.h"

CurseForge::FileResolvingTask::FileResolvingTask(shared_qobject_ptr<QNetworkAccessManager> network, CurseForge::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{
}

void CurseForge::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 1);
    m_dljob = new NetJob("Mod id resolver", m_network);
    QJsonObject object;
    QJsonArray fileIds;
    for(auto & file: m_toProcess.files)
    {
        fileIds.append(file.fileId);
    }
    object["fileIds"] = fileIds;
    QByteArray data = Json::toText(object);
    auto dl = Net::Download::makePost(QUrl("https://api.curseforge.com/v1/mods/files"), data, "application/json", &result);
    dl->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
    m_dljob->addNetAction(dl);
    connect(m_dljob.get(), &NetJob::finished, this, &CurseForge::FileResolvingTask::netJobFinished);
    m_dljob->start();
}

void CurseForge::FileResolvingTask::netJobFinished()
{
    bool failed = false;
    auto doc = Json::requireDocument(result);
    auto obj = Json::requireObject(doc);
    auto dataArray = Json::requireArray(obj, "data");

    for (QJsonValueRef file : dataArray) {
        auto fileObj = Json::requireObject(file);
        auto fileId = Json::requireInteger(fileObj, "id");
        auto& current = m_toProcess.files[fileId];
        if(!current.parse(fileObj)) {
            failed = true;
            qCritical() << "Resolving of" << current.projectId << current.fileId << "failed because of an error!";
            qCritical() << "JSON:";
            qCritical() << Json::toText(fileObj);
        }
        if(!current.resolved) {
            qWarning() << "Failed to resolve " << current.projectId << current.fileId << ":" << current.fileName << current.sha1;
        }
    }
    if(!failed) {
        emitSucceeded();
    }
    else {
        emitFailed(tr("Some mod ID resolving tasks failed."));
    }
}
