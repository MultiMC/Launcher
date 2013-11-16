#include "QuickModFile.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>


QuickModVersion::QuickModVersion(QuickModFile *parent) :
	QObject(parent), m_qmFile(parent)
{

}

QString QuickModVersion::modTypeString() const
{
	switch (m_modType) {
	case QuickModVersion::ForgeMod: return tr("Forge Mod");
	case QuickModVersion::ForgeCoreMod: return tr("Forge Coremod");
	case QuickModVersion::ResourcePack: return tr("Resource pack");
	case QuickModVersion::ConfigPack: return tr("Config pack");
	}
	return QString();
}

QuickModFile::QuickModFile(QObject *parent) :
	QObject(parent), m_file(0)
{

}

void QuickModFile::setFileInfo(const QFileInfo &info)
{
	m_info = info;
}
bool QuickModFile::open()
{
	if (m_file == 0) {
		if (m_info.isFile()) {
			m_file = new QFile(m_info.absoluteFilePath(), this);
		} else {
			return false;
		}
	}
	if (!m_file->open(QFile::ReadOnly)) {
		m_errorString = m_file->errorString();
		return false;
	}
	return parse();
}

bool QuickModFile::open(QIODevice *device)
{
	m_file = device;
	setFileInfo(QFileInfo());

	if (!m_file->isOpen() && !m_file->open(QIODevice::ReadOnly)) {
		m_errorString = m_file->errorString();
	}

	return parse();
}

bool QuickModFile::parse()
{
	QJsonParseError parseError;
	QJsonObject mod = QJsonDocument::fromJson(m_file->readAll(), &parseError).object();
	if (parseError.error != QJsonParseError::NoError) {
		m_errorString = tr("At %1: %2").arg(parseError.offset).arg(parseError.errorString());
		return false;
	}
	m_name = mod.value("name").toString();
	m_website = mod.value("website").toString();
	m_icon = mod.value("icon").toString();
	QJsonObject recommends = mod.value("recommends").toObject();
	for (QJsonObject::const_iterator it = recommends.begin(); it != recommends.end(); ++it) {
		m_recommends.insert(it.key(), QUrl(it.value().toString()));
	}
	QJsonObject depends = mod.value("depends").toObject();
	for (QJsonObject::const_iterator it = depends.begin(); it != depends.end(); ++it) {
		m_depends.insert(it.key(), QUrl(it.value().toString()));
	}
	QJsonArray versions = mod.value("versions").toArray();
	foreach (const QJsonValue& versionValue, versions) {
		QJsonObject v = versionValue.toObject();
		QuickModVersion* version = new QuickModVersion(this);
		version->m_name = v.value("name").toString();
		if (v.contains("url")) {
			version->m_urls.append(QUrl(v.value("url").toString()));
		}
		if (v.contains("urls")) {
			foreach (const QVariant& url, v.value("urls").toArray().toVariantList()) {
				version->m_urls.append(QUrl(url.toString()));
			}
		}
		foreach (const QVariant& mcVersion, v.value("mcCompatibility").toArray().toVariantList()) {
			version->m_mcVersions.append(mcVersion.toString());
		}
		const QString modType = v.value("type").toString();
		if (modType == "forgeMod") {
			version->m_modType = QuickModVersion::ForgeMod;
		} else if (modType == "forgeCoreMod") {
			version->m_modType = QuickModVersion::ForgeCoreMod;
		} else if (modType == "resourcePack") {
			version->m_modType = QuickModVersion::ResourcePack;
		} else if (modType == "configPack") {
			version->m_modType = QuickModVersion::ConfigPack;
		}
		m_versions.push_back(version);
	}
	return true;
}
