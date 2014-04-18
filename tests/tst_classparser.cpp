#include <QTest>
#include "TestUtil.h"

#include "depends/classparser/src/ClassFile.h"

using namespace Java;

Q_DECLARE_METATYPE(AccessFlags)
Q_DECLARE_METATYPE(AbstractAttribute)

namespace QTest
{
template<>
char *toString(const AccessFlags &flags)
{
	return qstrdup(QString::number((uint16_t)flags, 16).toLocal8Bit().data());
}
}

class ClassParserTest : public QObject
{
	Q_OBJECT
private:
	ClassFile *file;

private
slots:
	void initTestCase()
	{
		QDataStream stream(MULTIMC_GET_TEST_FILE("tests/data/ClassParserTestClass.class"));
		try
		{
			file = new ClassFile(stream);
		}
		catch (std::exception e)
		{
			QFAIL(e.what());
		}
	}
	void cleanupTestCase()
	{
		delete file;
	}

	void clazz()
	{
		QCOMPARE(file->accessFlags(), AccessFlag::Public | AccessFlag::Abstract | AccessFlag::Super);
		QCOMPARE((int)file->interfaces().size(), 2);
		QCOMPARE(file->interfaces().at(0)->name(), std::string("java/lang/Comparable"));
		QCOMPARE(file->interfaces().at(1)->name(), std::string("java/io/Serializable"));
		// make sure the jar is for java 6
		QCOMPARE(file->majorVersion(), 50);
		QCOMPARE(QString::fromStdString(file->superClass()->name()), QString("java/util/AbstractList"));
		QCOMPARE(QString::fromStdString(file->thisClass()->name()), QString("ClassParserTestClass"));
		for (auto attribute : file->attributes())
		{
			AbstractAttribute *attr = attribute.resolved();
			switch (attr->type)
			{
			case AbstractAttribute::Type::ConstantValue:
			case AbstractAttribute::Type::Code:
			case AbstractAttribute::Type::StackMapTable:
			case AbstractAttribute::Type::Exceptions:
			case AbstractAttribute::Type::InnerClasses:
			case AbstractAttribute::Type::EnclosingMethod:
			case AbstractAttribute::Type::Synthetic:
			case AbstractAttribute::Type::SourceDebugExtension:
			case AbstractAttribute::Type::LineNumberTable:
			case AbstractAttribute::Type::LocalVariableTable:
			case AbstractAttribute::Type::LocalVariableTypeTable:
			case AbstractAttribute::Type::Deprecated:
			case AbstractAttribute::Type::RuntimeVisibleAnnotations:
			case AbstractAttribute::Type::RuntimeInvisibleAnnotations:
			case AbstractAttribute::Type::RuntimeVisibleParameterAnnotations:
			case AbstractAttribute::Type::RuntimeInvisibleParameterAnnotations:
			case AbstractAttribute::Type::AnnotationDefault:
			case AbstractAttribute::Type::BootstrapMethods:
				QVERIFY2(false, "The only attributes we allow are Signature and SourceFile");
				break;
			case AbstractAttribute::Type::Signature:
				QCOMPARE(QString::fromStdString(attr->getAs<SignatureAttribute>()->signature()), QString("Ljava/util/AbstractList<Ljava/lang/String;>;Ljava/lang/Comparable;Ljava/io/Serializable;"));
				break;
			case AbstractAttribute::Type::SourceFile:
				QCOMPARE(QString::fromStdString(attr->getAs<SourceFileAttribute>()->sourceFile()), QString("ClassParserTestClass.java"));
				break;
			}
		}
	}

	void fields_data()
	{
		QTest::addColumn<QString>("name");
		QTest::addColumn<QString>("descriptor");
		QTest::addColumn<AccessFlags>("flags");
		QTest::addColumn<QVariant>("default_");

		QTest::newRow("publicStaticFinalStringDefault") << "publicStaticFinalStringDefault"
											   << "Ljava/lang/String;" << AccessFlags(AccessFlag::Public | AccessFlag::Static | AccessFlag::Final)
											   << QVariant(QStringLiteral("Ì'm å ÛTF-8 ŝtrïñg"));
		QTest::newRow("privateInteger") << "privateInteger"
											   << "Ljava/lang/Integer;" << AccessFlags(AccessFlag::Private)
											   << QVariant();
		QTest::newRow("protectedStaticVolatileComparable") << "protectedStaticVolatileComparable"
											   << "Ljava/lang/Comparable;" << AccessFlags(AccessFlag::Protected | AccessFlag::Static | AccessFlag::Volatile)
											   << QVariant();
		QTest::newRow("publicTransientFloat") << "publicTransientFloat"
											   << "Ljava/lang/Float;" << AccessFlags(AccessFlag::Public | AccessFlag::Transient)
											   << QVariant();
	}
	void fields()
	{
		QFETCH(QString, name);
		QFETCH(QString, descriptor);
		QFETCH(AccessFlags, flags);
		QFETCH(QVariant, default_);

		try
		{
			FieldInfo field = file->field(name.toStdString());
			QCOMPARE(QString::fromStdString(field.name()), name);
			QCOMPARE(QString::fromStdString(field.descriptor()), descriptor);
			QCOMPARE(field.flags(), flags);
			if (!default_.isNull())
			{
				ConstantValueAttribute *attribute = field.attributes().at(0).resolved()->getAs<ConstantValueAttribute>();
				QVERIFY(attribute);
				AbstractConstant *constant = attribute->value();
				QVERIFY(constant);
				auto string = QString::fromStdString(constant->getAs<StringConstant>()->string());
				switch (default_.type())
				{
				case QVariant::String:
					QVERIFY(constant->getAs<StringConstant>());
					QCOMPARE(string, default_.toString());
					break;
				case QVariant::Int:
					QFAIL("Not implemented");
					break;
				}
			}
		}
		catch (FieldNotFoundException e)
		{
			QFAIL(e.what());
		}
	}
};

QTEST_GUILESS_MAIN(ClassParserTest)

#include "tst_classparser.moc"
