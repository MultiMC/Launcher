#include "RawLibrary.h"

#include <QDebug>

QStringList RawLibrary::files() const
{
	QStringList retval;
	QString storage = storagePath();
	if (storage.contains("${arch}"))
	{
		QString cooked_storage = storage;
		cooked_storage.replace("${arch}", "32");
		retval.append(cooked_storage);
		cooked_storage = storage;
		cooked_storage.replace("${arch}", "64");
		retval.append(cooked_storage);
	}
	else
		retval.append(storage);
	return retval;
}

bool RawLibrary::filesExist(const QDir &base) const
{
	auto libFiles = files();
	for(auto file: libFiles)
	{
		QFileInfo info(base, file);
		qWarning() << info.absoluteFilePath() << "doesn't exist";
		if (!info.exists())
			return false;
	}
	return true;
}

QString RawLibrary::downloadUrl() const
{
	if (m_absolute_url.size())
		return m_absolute_url;

	if (m_base_url.isEmpty())
	{
		return QString("https://" + URLConstants::LIBRARY_BASE) + storagePath();
	}

	return m_base_url + storagePath();
}

bool RawLibrary::isActive() const
{
	bool result = true;
	if (m_rules.empty())
	{
		result = true;
	}
	else
	{
		RuleAction ruleResult = Disallow;
		for (auto rule : m_rules)
		{
			RuleAction temp = rule->apply(this);
			if (temp != Defer)
				ruleResult = temp;
		}
		result = result && (ruleResult == Allow);
	}
	if (isNative())
	{
		result = result && m_native_classifiers.contains(currentSystem);
	}
	return result;
}

QString RawLibrary::storagePath() const
{
	// non-native? use only the gradle specifier
	if (!isNative())
	{
		return m_name.toPath();
	}

	// otherwise native, override classifiers. Mojang HACK!
	GradleSpecifier nativeSpec = m_name;
	if(m_native_classifiers.contains(currentSystem))
	{
		nativeSpec.setClassifier(m_native_classifiers[currentSystem]);
	}
	else
	{
		nativeSpec.setClassifier("INVALID");
	}
	return nativeSpec.toPath();
}
