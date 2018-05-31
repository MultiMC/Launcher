#include "FtbPrivatePackManager.h"

#include <QDebug>
#include <QTextStream>

QFile FtbPrivatePackManager::saveFile("private_packs.txt");
QStringList FtbPrivatePackManager::currentPacks;

void FtbPrivatePackManager::refresh()
{
	if(!saveFile.exists()) {
		currentPacks = QStringList();
		return;
	}

	if(saveFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QByteArray data = saveFile.readAll();
		saveFile.close();

		QString text(data);
		currentPacks = text.split("\n");

		QStringList removeBuffer;

		foreach (const QString& current, currentPacks) {
			if(current.isNull() || current.trimmed().isEmpty()) {
				removeBuffer.append(current);
				continue;
			}
			qDebug() << "Got pack code" << current;
		}

		foreach (const QString& toRemove, removeBuffer) {
			currentPacks.removeAll(toRemove);
		}
	} else {
		qWarning() << "Failed to open ThirdPartySaveFile " + saveFile.fileName() + "!";
	}
}

void FtbPrivatePackManager::save()
{
	if(saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		QTextStream stream(&saveFile);
		foreach (const QString& current, currentPacks) {
			stream << current << "\n";
		}
		stream.flush();
		saveFile.close();
	} else {
		qCritical() << "Failed to open ThirdPartySaveFile " + saveFile.fileName() + " for saving!";
	}
}

QStringList FtbPrivatePackManager::getCurrentPackCodes()
{
	return currentPacks;
}

void FtbPrivatePackManager::setCurrentPacks(QStringList newPackList)
{
	currentPacks = newPackList;
}

void FtbPrivatePackManager::addCode(QString code)
{
	currentPacks.append(code);
	currentPacks.removeDuplicates();
}

void FtbPrivatePackManager::removeCode(QString code)
{
	currentPacks.removeAll(code);
}
