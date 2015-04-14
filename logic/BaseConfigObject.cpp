// Licensed under the Apache-2.0 license. See README.md for details.

#include "BaseConfigObject.h"

#include <QTimer>
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

	// cppcheck-suppress pureVirtualCall
	m_appQuitConnection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [this](){saveNow();});
}
BaseConfigObject::~BaseConfigObject()
{
	delete m_saveTimer;
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

	QFile file(m_filename);
	if (!file.open(QFile::WriteOnly))
	{
		qWarning() << "Couldn't open" << m_filename << "for writing:" << file.errorString();
		return;
	}
	// cppcheck-suppress pureVirtualCall
	file.write(doSave());
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
