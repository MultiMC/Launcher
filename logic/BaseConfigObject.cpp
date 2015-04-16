// Licensed under the Apache-2.0 license. See README.md for details.

#include "BaseConfigObject.h"

#include <QTimer>
#include <QSaveFile>
#include <QFile>
#include <QCoreApplication>

#include "FileSystem.h"

BaseConfigObject::BaseConfigObject(const QString &filename)
	: m_filename(filename)
{
	m_saveTimer = new QTimer;
	m_saveTimer->setSingleShot(true);
	// cppcheck-suppress pureVirtualCall
	QObject::connect(m_saveTimer, &QTimer::timeout, [this](){saveNow();});
	setSaveTimeout(250);

	m_initialReadTimer = new QTimer;
	m_initialReadTimer->setSingleShot(true);
	QObject::connect(m_initialReadTimer, &QTimer::timeout, [this]()
	{
		loadNow();
		m_initialReadTimer->deleteLater();
		m_initialReadTimer = 0;
	});
	m_initialReadTimer->start(0);

	// cppcheck-suppress pureVirtualCall
	m_appQuitConnection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [this](){saveNow();});
}
BaseConfigObject::~BaseConfigObject()
{
	delete m_saveTimer;
	if (m_initialReadTimer)
	{
		delete m_initialReadTimer;
	}
	QObject::disconnect(m_appQuitConnection);
}

void BaseConfigObject::setSaveTimeout(int msec)
{
	m_saveTimer->setInterval(msec);
}

void BaseConfigObject::scheduleSave()
{
	m_saveTimer->stop();
	m_saveTimer->start();
}
void BaseConfigObject::saveNow()
{
	if (m_saveTimer->isActive())
	{
		m_saveTimer->stop();
	}
	if (m_disableSaving)
	{
		return;
	}

	QSaveFile file(m_filename);
	if (!file.open(QFile::WriteOnly))
	{
		qWarning() << "Couldn't open" << m_filename << "for writing:" << file.errorString();
		return;
	}
	// cppcheck-suppress pureVirtualCall
	file.write(doSave());

	if (!file.commit())
	{
		qCritical() << "Unable to commit the file" << file.fileName() << ":" << file.errorString();
		file.cancelWriting();
	}
}
void BaseConfigObject::loadNow()
{
	if (m_saveTimer->isActive())
	{
		saveNow();
	}

	QFile file(m_filename);
	if (!file.exists())
	{
		return;
	}
	if (!file.open(QFile::ReadOnly))
	{
		qWarning() << "Couldn't open" << m_filename << "for reading:" << file.errorString();
		return;
	}
	try
	{
		doLoad(file.readAll());
	}
	catch (Exception &e)
	{
		qWarning() << "Error loading" << m_filename << ":" << e.cause();
	}
}
