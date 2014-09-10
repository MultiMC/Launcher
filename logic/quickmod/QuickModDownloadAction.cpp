#include "QuickModDownloadAction.h"

#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModsList.h"

#include "MultiMC.h"
#include "logger/QsLog.h"
#include "logic/MMCJson.h"

QuickModDownloadAction::QuickModDownloadAction(const QUrl &url, const QString &expectedUid)
	: QuickModBaseDownloadAction(url), m_expectedUid(expectedUid)
{
}

bool QuickModDownloadAction::handle(const QByteArray &data)
{
	try
	{
		const QJsonObject root = MMCJson::ensureObject(MMCJson::parseDocument(data, "QuickMod from " + m_url.toString()));
		const QString &uid = MMCJson::ensureString(root.value("uid"));
		if (!m_expectedUid.isEmpty() && uid != m_expectedUid)
		{
			throw MMCError("UID of the received QuickMod isn't matching the expectations");
		}
		QuickModMetadataPtr meta = std::make_shared<QuickModMetadata>();
		meta->parse(root);
		MMC->quickmodslist()->addMod(meta);
	}
	catch (MMCError &e)
	{
		m_errorString = e.cause();
		return false;
	}
	return true;
}
