#include "QuickModDownloadAction.h"

#include <functional>

#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"

#include "MultiMC.h"
#include "logger/QsLog.h"
#include "logic/MMCJson.h"

QuickModDownloadAction::QuickModDownloadAction(const QUrl &url, const QString &expectedUid)
	: QuickModBaseDownloadAction(url), m_expectedUid(expectedUid)
{
}

void QuickModDownloadAction::add()
{
	MMC->quickmodslist()->addMod(m_resultMetadata);
	for (const auto version : m_resultVersions)
	{
		MMC->quickmodslist()->addVersion(version);
	}
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
		m_resultMetadata = std::make_shared<QuickModMetadata>();
		m_resultMetadata->parse(root);

		const QJsonArray versions = MMCJson::ensureArray(root.value("versions"));
		for (const auto versionVal : versions)
		{
			QuickModVersionPtr ptr = std::make_shared<QuickModVersion>(m_resultMetadata);
			ptr->parse(MMCJson::ensureObject(versionVal));
			m_resultVersions.append(ptr);
		}

		if (m_autoAdd)
		{
			add();
		}
	}
	catch (MMCError &e)
	{
		m_errorString = e.cause();
		return false;
	}
	return true;
}
