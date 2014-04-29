#include "QuickModChooseModDialog.h"
#include "ui_QuickModChooseModDialog.h"

#include <QSortFilterProxyModel>
#include <QIdentityProxyModel>
#include <QListView>
#include <QMessageBox>

#include "logic/quickmod/QuickModsList.h"
#include "gui/dialogs/quickmod/QuickModInstallDialog.h"
#include "QuickModAddFileDialog.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/OneSixInstance.h"

#include "MultiMC.h"

template <typename T> bool intersectLists(const QList<T> &l1, const QList<T> &l2)
{
	foreach(const T & item, l1)
	{
		if (!l2.contains(item))
		{
			return false;
		}
	}

	return true;
}
bool listContainsSubstring(const QStringList &list, const QString &str)
{
	foreach(const QString & item, list)
	{
		if (item.contains(str))
		{
			return true;
		}
	}
	return false;
}

class ModFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	ModFilterProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
	{
		setSortRole(Qt::DisplayRole);
		setSortCaseSensitivity(Qt::CaseInsensitive);
	}

	void setSourceModel(QAbstractItemModel *model)
	{
		QSortFilterProxyModel::setSourceModel(model);
		connect(model, &QAbstractItemModel::modelReset, this, &ModFilterProxyModel::resort);
		connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(resort()));
		resort();
	}

	void setTags(const QStringList &tags)
	{
		m_tags = tags;
		invalidateFilter();
	}
	void setCategory(const QString &category)
	{
		m_category = category;
		invalidateFilter();
	}
	void setMCVersion(const QString &version)
	{
		m_mcVersion = version;
		invalidateFilter();
	}
	void setFulltext(const QString &query)
	{
		m_fulltext = query;
		invalidateFilter();
	}

private
slots:
	void resort()
	{
		sort(0);
	}

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

		if (!m_tags.isEmpty())
		{
			if (!intersectLists(m_tags, index.data(QuickModsList::TagsRole).toStringList()))
			{
				return false;
			}
		}
		if (!m_category.isEmpty())
		{
			if (!listContainsSubstring(index.data(QuickModsList::CategoriesRole).toStringList(),
									   m_category))
			{
				return false;
			}
		}
		if (!m_mcVersion.isEmpty())
		{
			if (!listContainsSubstring(index.data(QuickModsList::MCVersionsRole).toStringList(),
									   m_mcVersion))
			{
				return false;
			}
		}
		if (!m_fulltext.isEmpty())
		{
			bool inName = index.data(QuickModsList::NameRole).toString().contains(
				m_fulltext, Qt::CaseInsensitive);
			bool inDesc = index.data(QuickModsList::DescriptionRole).toString().contains(
				m_fulltext, Qt::CaseInsensitive);
			if (!inName && !inDesc)
			{
				return false;
			}
		}

		return true;
	}

private:
	QStringList m_tags;
	QString m_category;
	QString m_mcVersion;
	QString m_fulltext;
};

class TagsValidator : public QValidator
{
	Q_OBJECT
public:
	TagsValidator(QObject *parent = 0) : QValidator(parent)
	{
	}

protected:
	State validate(QString &input, int &pos) const
	{
		// TODO write a good validator
		return Acceptable;
	}
};

class CheckboxProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	CheckboxProxyModel(QObject *parent)
		: QIdentityProxyModel(parent)
	{
	}

	void setSourceModel(QAbstractItemModel *model)
	{
		QIdentityProxyModel::setSourceModel(model);
		connect(model, &QAbstractItemModel::modelReset, [this](){m_items.clear();});
	}

	QStringList getCheckedItems() const
	{
		QStringList items;
		for (auto item : m_items)
		{
			items.append(item);
		}
		return items;
	}

	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		return QIdentityProxyModel::flags(index) | Qt::ItemIsUserCheckable;
	}
	bool setData(const QModelIndex &index, const QVariant &value, int role)
	{
		if (index.isValid() && role == Qt::CheckStateRole)
		{
			if (value == Qt::Checked)
			{
				m_items.insert(index.data(QuickModsList::UidRole).toString());
				emit dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
				return true;
			}
			else if (value == Qt::Unchecked)
			{
				m_items.remove(index.data(QuickModsList::UidRole).toString());
				emit dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
				return true;
			}
		}
		return QIdentityProxyModel::setData(index, value, role);
	}
	QVariant data(const QModelIndex &proxyIndex, int role) const
	{
		if (proxyIndex.isValid() && role == Qt::CheckStateRole)
		{
			return m_items.contains(proxyIndex.data(QuickModsList::UidRole).toString()) ? Qt::Checked : Qt::Unchecked;
		}
		return QIdentityProxyModel::data(proxyIndex, role);
	}

private:
	QSet<QString> m_items;
};

QuickModChooseModDialog::QuickModChooseModDialog(InstancePtr instance, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModChooseModDialog), m_currentMod(0),
	  m_instance(instance), m_view(new QListView(this)),
	  m_filterModel(new ModFilterProxyModel(this)), m_checkModel(new CheckboxProxyModel(this))
{
	ui->setupUi(this);

	ui->viewLayout->addWidget(m_view);

	ui->tagsEdit->setValidator(new TagsValidator(this));

	m_view->setSelectionBehavior(QListView::SelectRows);
	m_view->setSelectionMode(QListView::SingleSelection);

	m_filterModel->setSourceModel(MMC->quickmodslist().get());
	m_checkModel->setSourceModel(m_filterModel);
	m_view->setModel(m_checkModel);

	connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
			&QuickModChooseModDialog::modSelectionChanged);
	connect(MMC->quickmodslist().get(), &QuickModsList::modsListChanged, this,
			&QuickModChooseModDialog::setupComboBoxes);
	connect(ui->closeButton, &QPushButton::clicked, this, &QuickModChooseModDialog::reject);

	setupComboBoxes();
}

QuickModChooseModDialog::~QuickModChooseModDialog()
{
	delete ui;
}

void QuickModChooseModDialog::on_installButton_clicked()
{
	auto items = m_checkModel->getCheckedItems();
	auto alreadySelected = std::dynamic_pointer_cast<OneSixInstance>(m_instance)->getFullVersion()->quickmods;
	for (auto mod : alreadySelected.keys())
	{
		items.removeAll(mod);
	}
	if (items.isEmpty())
	{
		return;
	}

	QMap<QString, QString> mods;
	for (auto item : items)
	{
		mods[item] = QString();
	}

	try
	{
		std::dynamic_pointer_cast<OneSixInstance>(m_instance)->setQuickModVersions(mods);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		reject();
		return;
	}
	accept();
}
void QuickModChooseModDialog::on_categoriesLabel_linkActivated(const QString &link)
{
	ui->categoryBox->setCurrentText(link);
	ui->tagsEdit->setText(QString());
}
void QuickModChooseModDialog::on_tagsLabel_linkActivated(const QString &link)
{
	ui->tagsEdit->setText(ui->tagsEdit->text() + ", " + link);
	ui->categoryBox->setCurrentText(QString());
	on_tagsEdit_textChanged();
}
void QuickModChooseModDialog::on_fulltextEdit_textChanged()
{
	m_filterModel->setFulltext(ui->fulltextEdit->text());
}
void QuickModChooseModDialog::on_tagsEdit_textChanged()
{
	m_filterModel->setTags(
		ui->tagsEdit->text().split(QRegularExpression(", {0,1}"), QString::SkipEmptyParts));
}
void QuickModChooseModDialog::on_categoryBox_currentTextChanged()
{
	m_filterModel->setCategory(ui->categoryBox->currentText());
}
void QuickModChooseModDialog::on_mcVersionBox_currentTextChanged()
{
	m_filterModel->setMCVersion(ui->mcVersionBox->currentText());
}


void QuickModChooseModDialog::modSelectionChanged(const QItemSelection &selected,
												 const QItemSelection &deselected)
{
	if (m_currentMod)
	{
		disconnect(m_currentMod.get(), &QuickMod::logoUpdated, this,
				   &QuickModChooseModDialog::modLogoUpdated);
	}

	if (selected.isEmpty())
	{
		m_currentMod = 0;
		ui->nameLabel->setText("");
		ui->descriptionLabel->setText("");
		ui->websiteLabel->setText("");
		ui->categoriesLabel->setText("");
		ui->tagsLabel->setText("");
		ui->logoLabel->setPixmap(QPixmap());
	}
	else
	{
		m_currentMod = m_filterModel->index(selected.first().top(), 0)
							.data(QuickModsList::QuickModRole)
							.value<QuickModPtr>();
		ui->nameLabel->setText(m_currentMod->name());
		ui->descriptionLabel->setText(m_currentMod->description());
		ui->websiteLabel->setText(QString("<a href=\"%1\">%2</a>").arg(
			m_currentMod->websiteUrl().toString(QUrl::FullyEncoded),
			m_currentMod->websiteUrl().toString(QUrl::PrettyDecoded)));
		QStringList categories;
		foreach(const QString & category, m_currentMod->categories())
		{
			categories.append(QString("<a href=\"%1\">%1</a>").arg(category));
		}
		ui->categoriesLabel->setText(categories.join(", "));
		QStringList tags;
		foreach(const QString & tag, m_currentMod->tags())
		{
			tags.append(QString("<a href=\"%1\">%1</a>").arg(tag));
		}
		ui->tagsLabel->setText(tags.join(", "));
		ui->logoLabel->setPixmap(m_currentMod->logo());

		connect(m_currentMod.get(), &QuickMod::logoUpdated, this,
				&QuickModChooseModDialog::modLogoUpdated);
	}
}

void QuickModChooseModDialog::modLogoUpdated()
{
	ui->logoLabel->setPixmap(m_currentMod->logo());
}

void QuickModChooseModDialog::setupComboBoxes()
{
	QStringList categories;
	categories.append("");
	QStringList versions;
	versions.append("");
	versions.append(m_instance->intendedVersionId());

	for (int i = 0; i < MMC->quickmodslist()->numMods(); ++i)
	{
		categories.append(MMC->quickmodslist()->modAt(i)->categories());
		versions.append(MMC->quickmodslist()->modAt(i)->mcVersions());
	}

	categories.removeDuplicates();
	versions.removeDuplicates();

	ui->categoryBox->clear();
	ui->categoryBox->addItems(categories);
	ui->mcVersionBox->clear();
	ui->mcVersionBox->addItems(versions);
	ui->mcVersionBox->setCurrentIndex(1);
}

void QuickModChooseModDialog::on_addButton_clicked()
{
	QuickModAddFileDialog::run(this);
}
void QuickModChooseModDialog::on_updateButton_clicked()
{
	MMC->quickmodslist()->updateFiles();
}

#include "QuickModChooseModDialog.moc"
