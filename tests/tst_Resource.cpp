#include <QTest>
#include <QAction>
#include "TestUtil.h"

#include "resources/Resource.h"
#include "resources/ResourceHandler.h"
#include "resources/ResourceObserver.h"

class DummyStringResourceHandler : public ResourceHandler
{
public:
	explicit DummyStringResourceHandler(const QString &key)
		: m_key(key) {}

	void init(std::shared_ptr<ResourceHandler> &) override
	{
		setResult(m_key);
	}

	QString m_key;
};
class DummyObserver : public ResourceObserver
{
public:
	void resourceUpdated() override
	{
		values += get<QString>();
	}

	QStringList values;
};
class DummyObserverObject : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString property MEMBER property)

public:
	explicit DummyObserverObject(QObject *parent = nullptr) : QObject(parent) {}

	QString property;
};

class ResourceTest : public QObject
{
	Q_OBJECT
private
slots:
	void initTestCase()
	{
		Resource::registerHandler<DummyStringResourceHandler>("dummy");
	}
	void cleanupTestCase()
	{
	}

	void test_Then()
	{
		QString val;
		Resource::create("dummy:test_Then")
				->then([&val](const QString &key) { val = key; });
		QCOMPARE(val, QStringLiteral("test_Then"));
	}
	void test_Object()
	{
		DummyObserver *observer = new DummyObserver;
		Resource::create("dummy:test_Object")->applyTo(observer);
		QCOMPARE(observer->values, QStringList() << "test_Object");
	}
	void test_QObjectProperty()
	{
		DummyObserverObject *object = new DummyObserverObject;
		Resource::create("dummy:test_QObjectProperty")->applyTo(object);
		QCOMPARE(object->property, QStringLiteral("test_QObjectProperty"));
	}

	void test_DontRequestPlaceholder()
	{
		auto resource = Resource::create("dummy:asdf")
				->then([](const QString &key) { QCOMPARE(key, QStringLiteral("asdf")); });
		// the following call should not notify the observer. if it does the above QCOMPARE would fail.
		resource->placeholder(Resource::create("dummy:fdsa"));
	}

	void test_MergedResources()
	{
		auto r1 = Resource::create("dummy:asdf");
		auto r2 = Resource::create("dummy:asdf");
		auto r3 = Resource::create("dummy:fdsa");
		auto r4 = Resource::create("dummy:asdf");

		QCOMPARE(r1, r2);
		QCOMPARE(r1, r4);
		QVERIFY(r1 != r3);
		QVERIFY(r2 != r3);
		QVERIFY(r4 != r3);
	}
};

QTEST_GUILESS_MAIN(ResourceTest)

#include "tst_Resource.moc"
