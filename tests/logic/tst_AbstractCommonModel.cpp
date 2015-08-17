#include <QTest>
#include <QColor>
#include <QSignalSpy>

#include "logic/AbstractCommonModel.h"
#include "tests/ModelTester.h"

struct TestStruct
{
	explicit TestStruct(const QString &text1, const QString &text2, const int value)
		: text1(text1), m_text2(text2), m_value(value) {}

	// public member
	QString text1;

	// private member, readonly accessor
	QString text2() const { return m_text2; }
	// private member, read/write accessors
	int value() const { return m_value; }
	void setValue(const int val) { m_value = val; }

	// public member, accessed through a lambda
	QColor color = QColor(Qt::black);

private:
	QString m_text2;
	int m_value;
};
class TestObject : public QObject, public TestStruct
{
	Q_OBJECT
	Q_PROPERTY(QString text1 MEMBER text1)
	Q_PROPERTY(QString text2 READ text2)
	Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
public:
	explicit TestObject(const QString &text1, const QString &text2, const int value, QObject *parent = nullptr)
		: TestStruct(text1, text2, value) {}

	void setValue(const int val)
	{
		TestStruct::setValue(val);
		emit valueChanged();
	}

signals:
	void valueChanged();
};

class StructModel : public AbstractCommonModel<TestStruct>
{
	Q_OBJECT
public:
	explicit StructModel() : AbstractCommonModel<TestStruct>()
	{
		setEntryTitle(0, "Title One");
		setEntryTitle(1, "Title Two");
		setEntryTitle(2, "Additional column for extra tests"); // column 2 does not have a Qt::EditRole
		addEntry<QString>(0, Qt::DisplayRole, &TestStruct::text1);
		addEntry<QString>(1, Qt::DisplayRole, &TestStruct::text2);
		addEntry<QString>(0, Qt::EditRole, &TestStruct::text1); // member, can set
		addEntry<QString>(1, Qt::EditRole, &TestStruct::text2); // only getter, can't set
		addEntry<int>(0, Qt::UserRole, &TestStruct::value, &TestStruct::setValue);
		addEntry<int>(1, Qt::UserRole, &TestStruct::value, &TestStruct::setValue);
		addEntry<QColor>(2, Qt::DecorationRole, [](TestStruct ts) { return ts.color; });
	}
	~StructModel() {}

	void populate()
	{
		setAll({TestObject("one one", "one two", 1),
				TestObject("two one", "two two", 2),
				TestObject("three one", "three two", 3)});
	}
};
class ObjectModel : public AbstractCommonModel<TestObject *>
{
	Q_OBJECT
public:
	explicit ObjectModel() : AbstractCommonModel<TestObject *>()
	{
		addEntry<QString>(0, Qt::DisplayRole, &TestStruct::text1);
		addEntry<QString>(1, Qt::DisplayRole, &TestObject::text2);
		addEntry<QString>(0, Qt::EditRole, &TestStruct::text1); // member, can set
		addEntry<QString>(1, Qt::EditRole, &TestObject::text2); // only getter, can't set
		addEntry<int>(0, Qt::UserRole, &TestStruct::value, &TestStruct::setValue);
		addEntry<int>(1, Qt::UserRole, &TestStruct::value, &TestObject::setValue);
		addEntry<QColor>(0, Qt::DecorationRole, [](TestObject *ts) { return ts->color; });
		setEntryTitle(1, "Title Two"); // we can also set headers after the entires
		setEntryTitle(0, "Title One");
	}

	void populate()
	{
		setAll({new TestObject("one one", "one two", 1),
				new TestObject("two one", "two two", 2),
				new TestObject("three one", "three two", 3)});
	}
};

class AbstractCommonModelTest : public ModelTester
{
	Q_OBJECT

	enum Stage
	{
		StructStage,
		ObjectStage
	};

	int stages() const override { return 2; }
	std::shared_ptr<QAbstractItemModel> createModel(const int stage) const override
	{
		switch (stage)
		{
		case StructStage: return std::make_shared<StructModel>();
		case ObjectStage: return std::make_shared<ObjectModel>();
		default: return nullptr;
		}
	}
	void populate(std::shared_ptr<QAbstractItemModel> model, const int stage) const override
	{
		if (stage == StructStage)
		{
			std::dynamic_pointer_cast<StructModel>(model)->populate();
		}
		else
		{
			std::dynamic_pointer_cast<ObjectModel>(model)->populate();
		}
	}

	template<typename Type>
	std::shared_ptr<Type> createAndPopulateModel(const int stage)
	{
		auto model = createModel(stage);
		populate(model, stage);
		return std::dynamic_pointer_cast<Type>(model);
	}

private
slots:
	void test_Dimensions()
	{
		auto model0 = createAndPopulateModel<StructModel>(StructStage);

		QCOMPARE(model0->size(), 3);
		QCOMPARE(model0->entryCount(), 3);
		QCOMPARE(model0->rowCount(), 3);
		QCOMPARE(model0->columnCount(), 3);

		auto model1 = createAndPopulateModel<ObjectModel>(ObjectStage);

		QCOMPARE(model1->size(), 3);
		QCOMPARE(model1->entryCount(), 2);
		QCOMPARE(model1->rowCount(), 3);
		QCOMPARE(model1->columnCount(), 2);
	}

	void test_Flags()
	{
		auto model0 = createAndPopulateModel<StructModel>(StructStage);
		QCOMPARE(model0->flags(model0->index(15, 1)), Qt::NoItemFlags);
		QCOMPARE(model0->flags(model0->index(0, 0)), Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
		QCOMPARE(model0->flags(model0->index(0, 1)), Qt::ItemIsEnabled | Qt::ItemIsSelectable); // no setter
		QCOMPARE(model0->flags(model0->index(0, 2)), Qt::ItemIsEnabled | Qt::ItemIsSelectable); // no editrole
	}

	void test_Header()
	{
		auto model0 = createAndPopulateModel<StructModel>(StructStage);
		QCOMPARE(model0->headerData(0, Qt::Horizontal, Qt::DisplayRole), QVariant(QString("Title One")));
		QCOMPARE(model0->headerData(1, Qt::Horizontal, Qt::DisplayRole), QVariant(QString("Title Two")));
		QCOMPARE(model0->headerData(0, Qt::Vertical, Qt::DisplayRole), QVariant());
		QCOMPARE(model0->headerData(1, Qt::Vertical, Qt::DisplayRole), QVariant());
		QCOMPARE(model0->headerData(0, Qt::Horizontal, Qt::EditRole), QVariant());
	}

	void test_Add()
	{
		auto model = createAndPopulateModel<StructModel>(StructStage);

		QSignalSpy aboutToAddSpy(model.get(), &StructModel::rowsAboutToBeInserted);
		QSignalSpy addedSpy(model.get(), &StructModel::rowsInserted);
		QVERIFY(aboutToAddSpy.isValid() && addedSpy.isValid());

		model->append(TestStruct("alpha", "beta", 42));
		QCOMPARE(model->size(), 4);
		QCOMPARE(model->rowCount(), 4);
		QCOMPARE(model->entryCount(), 3); // only added a row, no extra columns should've been added
		QCOMPARE(model->get(3).text1, QString("alpha"));
		QCOMPARE(aboutToAddSpy.size(), 1);
		QCOMPARE(addedSpy.size(), 1);
		QCOMPARE(aboutToAddSpy.first(), QVariantList() << QModelIndex() << 3 << 3);
		QCOMPARE(addedSpy.first(), QVariantList() << QModelIndex() << 3 << 3);

		model->prepend(TestStruct("x", "y", 42 * 42));
		QCOMPARE(model->size(), 5);
		QCOMPARE(model->rowCount(), 5);
		QCOMPARE(model->get(0).text1, QString("x"));
		QCOMPARE(model->get(4).text1, QString("alpha")); // ensure alpha is still last
		QCOMPARE(aboutToAddSpy.size(), 2);
		QCOMPARE(addedSpy.size(), 2);
		QCOMPARE(aboutToAddSpy.at(1), QVariantList() << QModelIndex() << 0 << 0);
		QCOMPARE(addedSpy.at(1), QVariantList() << QModelIndex() << 0 << 0);
	}

	void test_Remove()
	{
		auto model = createAndPopulateModel<StructModel>(StructStage);

		QSignalSpy aboutToRemoveSpy(model.get(), &StructModel::rowsAboutToBeRemoved);
		QSignalSpy removedSpy(model.get(), &StructModel::rowsRemoved);
		QVERIFY(aboutToRemoveSpy.isValid() && removedSpy.isValid());

		model->remove(2);
		QCOMPARE(model->size(), 2);
		QCOMPARE(model->rowCount(), 2);
		QCOMPARE(model->entryCount(), 3); // only removed a row, no columns should've been removed
		QCOMPARE(aboutToRemoveSpy.size(), 1);
		QCOMPARE(removedSpy.size(), 1);
		QCOMPARE(aboutToRemoveSpy.first(), QVariantList() << QModelIndex() << 2 << 2);
		QCOMPARE(removedSpy.first(), QVariantList() << QModelIndex() << 2 << 2);

		model->remove(0);
		QCOMPARE(model->size(), 1);
		QCOMPARE(model->rowCount(), 1);
		QCOMPARE(model->get(0).text1, QString("two one"));
		QCOMPARE(aboutToRemoveSpy.size(), 2);
		QCOMPARE(removedSpy.size(), 2);
		QCOMPARE(aboutToRemoveSpy.at(1), QVariantList() << QModelIndex() << 0 << 0);
		QCOMPARE(removedSpy.at(1), QVariantList() << QModelIndex() << 0 << 0);
	}

	void test_setData()
	{
		auto model0 = createAndPopulateModel<StructModel>(StructStage);

		// valid role, setter provided
		QCOMPARE(model0->index(1, 0).data().toString(), QString("two one"));
		QVERIFY(model0->setData(model0->index(1, 0), QString("hello"), Qt::EditRole));
		QCOMPARE(model0->index(1, 0).data().toString(), QString("hello"));

		// different valid role, setter provided
		QCOMPARE(model0->index(1, 0).data().toString(), QString("hello"));
		QVERIFY(model0->setData(model0->index(1, 0), QString("forty two"), Qt::DisplayRole));
		QCOMPARE(model0->index(1, 0).data().toString(), QString("forty two"));

		// function entry
		QCOMPARE(model0->index(1, 0).data(Qt::UserRole).toInt(), 2);
		QVERIFY(model0->setData(model0->index(1, 0), 4422, Qt::UserRole));
		QCOMPARE(model0->index(1, 0).data(Qt::UserRole).toInt(), 4422);

		// no setter provided
		QCOMPARE(model0->index(1, 1).data().toString(), QString("two two"));
		QVERIFY(! model0->setData(model0->index(1, 1), QString("hello"), Qt::EditRole));
		QCOMPARE(model0->index(1, 1).data().toString(), QString("two two"));

		// lambda entry == no setter provided
		QCOMPARE(model0->index(1, 2).data(Qt::DecorationRole).value<QColor>(), QColor(Qt::black));
		QVERIFY(! model0->setData(model0->index(1, 2), QColor(Qt::black), Qt::DecorationRole));
		QCOMPARE(model0->index(1, 2).data(Qt::DecorationRole).value<QColor>(), QColor(Qt::black));

		// invalid index
		QVERIFY(! model0->setData(model0->index(1, 3232), QString("asdf"), Qt::EditRole));

		// role that doesn't exist
		QVERIFY(! model0->setData(model0->index(1, 0), QString("asdf"), Qt::DecorationRole));

		// object model
		auto model1 = createAndPopulateModel<ObjectModel>(ObjectStage);

		// valid role, setter provided
		QCOMPARE(model1->index(1, 0).data().toString(), QString("two one"));
		QVERIFY(model1->setData(model1->index(1, 0), QString("hello"), Qt::EditRole));
		QCOMPARE(model1->index(1, 0).data().toString(), QString("hello"));

		// different valid role, setter provided
		QCOMPARE(model1->index(1, 0).data().toString(), QString("hello"));
		QVERIFY(model1->setData(model1->index(1, 0), QString("forty two"), Qt::DisplayRole));
		QCOMPARE(model1->index(1, 0).data().toString(), QString("forty two"));

		// function entry
		QCOMPARE(model1->index(1, 0).data(Qt::UserRole).toInt(), 2);
		QVERIFY(model1->setData(model1->index(1, 0), 4422, Qt::UserRole));
		QCOMPARE(model1->index(1, 0).data(Qt::UserRole).toInt(), 4422);

		// no setter provided
		QCOMPARE(model1->index(1, 1).data().toString(), QString("two two"));
		QVERIFY(! model1->setData(model1->index(1, 1), QString("hello"), Qt::EditRole));
		QCOMPARE(model1->index(1, 1).data().toString(), QString("two two"));

		// lambda entry == no setter provided
		QCOMPARE(model1->index(1, 0).data(Qt::DecorationRole).value<QColor>(), QColor(Qt::black));
		QVERIFY(! model1->setData(model1->index(1, 0), QColor(Qt::black), Qt::DecorationRole));
		QCOMPARE(model1->index(1, 0).data(Qt::DecorationRole).value<QColor>(), QColor(Qt::black));
	}

	void test_Find()
	{
		auto model = createAndPopulateModel<ObjectModel>(ObjectStage);

		TestObject *obj = new TestObject("a", "b", 424242);
		model->append(obj);
		QCOMPARE(model->find(obj), 3);
	}

	void test_Insert()
	{
		auto model = createAndPopulateModel<StructModel>(StructStage);

		TestStruct s1("1a", "1b", 0);
		TestStruct s2("2a", "2b", 0);
		TestStruct s3("3a", "3b", 0);

		// appending
		model->insert(s1, -1);
		QCOMPARE(model->get(0).text1, QString("1a"));

		// prepending
		model->insert(s2, 12345);
		QCOMPARE(model->get(4).text1, QString("2a"));

		// middle
		model->insert(s3, 2);
		QCOMPARE(model->get(2).text1, QString("3a"));
	}
};

MMCTEST_GUILESS_MAIN(AbstractCommonModelTest)

#include "tst_AbstractCommonModel.moc"
