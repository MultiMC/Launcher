// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QObject>

class QTimer;

class BaseConfigObject
{
public:
	void setSaveTimeout(int msec);

protected:
	explicit BaseConfigObject(const QString &filename);
	virtual ~BaseConfigObject();

	// cppcheck-suppress pureVirtualCall
	virtual QByteArray doSave() const = 0;
	virtual void doLoad(const QByteArray &data) = 0;

	void setSavingDisabled(bool savingDisabled) { m_disableSaving = savingDisabled; }

public:
	void scheduleSave();
	void saveNow();
	void loadNow();

private:
	QTimer *m_saveTimer;
	QString m_filename;
	QMetaObject::Connection m_appQuitConnection;
	bool m_disableSaving = false;
};
