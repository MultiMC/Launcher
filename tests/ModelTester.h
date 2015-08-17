#pragma once

#include <QAbstractItemModel>
#include <QTest>
#include <memory>

#include "TestUtil.h"

// does some basic sanity checking (doesn't crash on invalid indexes etc.)
class ModelTester : public BaseTest
{
	Q_OBJECT
public:
	virtual int stages() const { return 1; }
	virtual std::shared_ptr<QAbstractItemModel> createModel(const int stage = 0) const = 0;
	virtual void populate(std::shared_ptr<QAbstractItemModel> model, const int stage = 0) const = 0;
	
private slots:
	/// What happens if we request some weird indexes?
	void test_CreateIndexes()
	{
		for (int i = 0; i < stages(); ++i)
		{
			variousIndexes(createModel(i));
		}
	}

	/// What happens if we use some weird indexes for stuff?
	void test_UseIndexes()
	{
		for (int i = 0; i < stages(); ++i)
		{
			std::shared_ptr<QAbstractItemModel> model = createModel(i);

			// send various indexes
			for (const QModelIndex &index : variousIndexes(model))
			{
				model->buddy(index);
				model->columnCount(index);
				model->data(index);
				model->fetchMore(index);
				model->flags(index);
				model->hasChildren(index);
				model->itemData(index);
				model->parent(index);
				model->rowCount(index);
				model->span(index);
			}
		}
	}

	/// Can we request data?
	void test_RequestData()
	{
		for (int i = 0; i < stages(); ++i)
		{
			std::shared_ptr<QAbstractItemModel> model = createModel(i);
			populate(model, i);

			for (int row = 0; row < model->rowCount(); ++row)
			{
				for (int col = 0; col < model->columnCount(); ++col)
				{
					model->data(model->index(row, col), Qt::DisplayRole);
					model->data(model->index(row, col), Qt::DecorationRole);
				}
			}
		}
	}
	
private:
	static QModelIndexList variousIndexes(std::shared_ptr<QAbstractItemModel> model)
	{
		return QModelIndexList()
				<< QModelIndex()
				<< model->index(0, 0)
				<< model->index(-1, 0)
				<< model->index(0, -1)
				<< model->index(INT_MAX, INT_MAX)
				<< model->index(0, 0, model->index(0, 0))
				<< model->index(0, 0, model->index(-1, 0))
				<< model->index(0, 0, model->index(0, -1));
	}
};
