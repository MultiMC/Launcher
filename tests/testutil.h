#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <QFile>

namespace Tests
{
namespace Internal
{

QByteArray readFile(const QString &fileName)
{
	QFile f(fileName);
	f.open(QFile::ReadOnly);
	return f.readAll();
}

}
}

#endif // TESTUTIL_H
