#pragma once

#include "QuickModBaseDownloadAction.h"

class QuickModDownloadAction : public QuickModBaseDownloadAction
{
	Q_OBJECT
public:
	explicit QuickModDownloadAction(const QUrl &url, const QString &expectedUid);

public:
	QString m_expectedUid;

private:
	bool handle(const QByteArray &data) override;
};
