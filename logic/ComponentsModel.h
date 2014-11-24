#pragma once

#include <QAbstractListModel>
#include <memory>

class ComponentsModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit ComponentsModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QStringList mimeTypes() const override;
	QMimeData *mimeData(const QModelIndexList &indexes) const override;
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
						 const QModelIndex &parent) const override;
	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;

	struct Component
	{
		QString name;
		QString version;
		QString path;
		QString mcversion;
		std::shared_ptr<class BaseInstance> origin;
		enum Type
		{
			LoaderMod,
			CoreMod,
			ResourcePack,
			TexturePack
		} type = LoaderMod;
		QString typeAsString() const;
		QJsonObject toJson() const;
		static Component fromJson(const QJsonObject &object);
		static QList<Component> fromJson(const QByteArray &data);
	};

private:
	QList<Component> m_components;

private slots:
	void update();
};
