#include "include/modutils.h"

#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Util::Version::Version(const QString &str) : m_string(str)
{
	parse();
}

bool Util::Version::operator<(const Version &other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i)
	{
		const Section sec1 = (i >= m_sections.size()) ? Section("0", 0) : m_sections.at(i);
		const Section sec2 =
			(i >= other.m_sections.size()) ? Section("0", 0) : other.m_sections.at(i);
		if (sec1 != sec2)
		{
			return sec1 < sec2;
		}
	}

	return false;
}
bool Util::Version::operator<=(const Util::Version &other) const
{
	return *this < other || *this == other;
}
bool Util::Version::operator>(const Version &other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i)
	{
		const Section sec1 = (i >= m_sections.size()) ? Section("0", 0) : m_sections.at(i);
		const Section sec2 =
			(i >= other.m_sections.size()) ? Section("0", 0) : other.m_sections.at(i);
		if (sec1 != sec2)
		{
			return sec1 > sec2;
		}
	}

	return false;
}
bool Util::Version::operator>=(const Version &other) const
{
	return *this > other || *this == other;
}
bool Util::Version::operator==(const Version &other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i)
	{
		const Section sec1 = (i >= m_sections.size()) ? Section("0", 0) : m_sections.at(i);
		const Section sec2 =
			(i >= other.m_sections.size()) ? Section("0", 0) : other.m_sections.at(i);
		if (sec1 != sec2)
		{
			return false;
		}
	}

	return true;
}
bool Util::Version::operator!=(const Version &other) const
{
	return !operator==(other);
}

void Util::Version::parse()
{
	m_sections.clear();

	QStringList parts = m_string.split('.');

	for (const auto part : parts)
	{
		bool ok = false;
		int num = part.toInt(&ok);
		if (ok)
		{
			m_sections.append(Section(part, num));
		}
		else
		{
			m_sections.append(Section(part));
		}
	}
}

QUrl Util::expandQMURL(const QString &in)
{
	QUrl inUrl(in);
	if (inUrl.scheme() == "github")
	{
		// needed because QUrl makes the host all lower cases
		const QString repo =
			in.mid(in.indexOf(inUrl.host(), 0, Qt::CaseInsensitive), inUrl.host().size());
		QUrl out;
		out.setScheme("https");
		out.setHost("raw.github.com");
		out.setPath(QString("/%1/%2/%3%4").arg(
			inUrl.userInfo(), repo, inUrl.fragment().isEmpty() ? "master" : inUrl.fragment(),
			inUrl.path()));
		return out;
	}
	else if (inUrl.scheme() == "mcf")
	{
		QUrl out;
		out.setScheme("http");
		out.setHost("www.minecraftforum.net");
		out.setPath(QString("/topic/%1-").arg(inUrl.path()));
		return out;
	}
	else if (inUrl.scheme() == "curse")
	{
		QUrl out;
		out.setScheme("http");
		out.setHost("www.curse.com");
		out.setPath(QString("/mc-mods/minecraft/%1").arg(inUrl.path()));
		return out;
	}
	else
	{
		return in;
	}
}

bool Util::versionIsInInterval(const QString &version, const QString &interval)
{
	return versionIsInInterval(Util::Version(version), interval);
}
bool Util::versionIsInInterval(const Version &version, const QString &interval)
{
	if (interval.isEmpty() || version.toString() == interval)
	{
		return true;
	}

	// Interval notation is used
	QRegularExpression exp(
		"(?<start>[\\[\\]\\(\\)])(?<bottom>.*?)(,(?<top>.*?))?(?<end>[\\[\\]\\(\\)]),?");
	QRegularExpressionMatch match = exp.match(interval);
	if (match.hasMatch())
	{
		const QChar start = match.captured("start").at(0);
		const QChar end = match.captured("end").at(0);
		const QString bottom = match.captured("bottom");
		const QString top = match.captured("top");

		// check if in range (bottom)
		if (!bottom.isEmpty())
		{
			const auto bottomVersion = Util::Version(bottom);
			if ((start == '[') && !(version >= bottomVersion))
			{
				return false;
			}
			else if ((start == '(') && !(version > bottomVersion))
			{
				return false;
			}
		}

		// check if in range (top)
		if (!top.isEmpty())
		{
			const auto topVersion = Util::Version(top);
			if ((end == ']') && !(version <= topVersion))
			{
				return false;
			}
			else if ((end == ')') && !(version < topVersion))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}
