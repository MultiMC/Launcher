#include "ChooseInstallModDialog.h"
#include "ui_ChooseInstallModDialog.h"

#include <QSortFilterProxyModel>

#include "depends/groupview/include/categorizedview.h"
#include "depends/groupview/include/categorizedsortfilterproxymodel.h"
#include "depends/groupview/include/categorydrawer.h"

#include "logic/lists/QuickModsList.h"
#include "gui/widgets/WebDownloadNavigator.h"
#include "ChooseQuickModVersionDialog.h"
#include "DownloadProgressDialog.h"
#include "AddQuickModFileDialog.h"

#include "MultiMC.h"

template<typename T>
bool intersectLists(const QList<T> &l1, const QList<T> &l2)
{
	foreach (const T& item, l1)
	{
		if (!l2.contains(item))
		{
			return false;
		}
	}

	return true;
}
bool listContainsSubstring(const QStringList& list, const QString& str)
{
	foreach (const QString& item, list)
	{
		if (item.contains(str))
		{
			return true;
		}
	}
	return false;
}

class ModFilterProxyModel : public KCategorizedSortFilterProxyModel
{
	Q_OBJECT
public:
	ModFilterProxyModel(QObject *parent = 0) : KCategorizedSortFilterProxyModel(parent)
	{
		setCategorizedModel(true);
	}

	void setTags(const QStringList& tags)
	{
		m_tags = tags;
		invalidateFilter();
	}
	void setCategory(const QString &category)
	{
		m_category = category;
		invalidateFilter();
	}
	void setFulltext(const QString &query)
	{
		m_fulltext = query;
		invalidateFilter();
	}

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

		if (index.data(QuickModsList::IsStubRole).toBool())
		{
			return false;
		}

		if (!m_tags.isEmpty())
		{
			if (!intersectLists(m_tags, index.data(QuickModsList::TagsRole).toStringList()))
			{
				return false;
			}
		}
		if (!m_category.isEmpty())
		{
			if (!listContainsSubstring(index.data(QuickModsList::CategoriesRole).toStringList(), m_category))
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

ChooseInstallModDialog::ChooseInstallModDialog(BaseInstance* instance, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ChooseInstallModDialog),
	m_instance(instance),
	m_view(new KCategorizedView(this)),
	m_model(new ModFilterProxyModel(this))
{
	ui->setupUi(this);

	ui->viewLayout->addWidget(m_view);

	ui->tagsEdit->setValidator(new TagsValidator(this));

	m_view->setSelectionBehavior(KCategorizedView::SelectRows);
	m_view->setSelectionMode(KCategorizedView::SingleSelection);
	m_view->setCategoryDrawer(new KCategoryDrawer(m_view));
	m_model->setSourceModel(MMC->quickmodslist().get());
	m_view->setModel(m_model);

	connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ChooseInstallModDialog::modSelectionChanged);
	connect(MMC->quickmodslist().get(), &QuickModsList::modsListChanged, this, &ChooseInstallModDialog::setupCategoryBox);

	setupCategoryBox();
}

ChooseInstallModDialog::~ChooseInstallModDialog()
{
	delete ui;
}

void ChooseInstallModDialog::on_installButton_clicked()
{
	if (m_view->selectionModel()->selection().isEmpty())
	{
		return;
	}

	auto choosenMod = m_view->selectionModel()->selectedRows().first().data(QuickModsList::QuickModRole).value<QuickMod*>();
	QMap<QuickMod*, QuickMod::Version> modsList;
	/*
	 * At this point we have access to the QuickModsList instance (via MMC->quickmodslist()) and
	 * one mod (choosenMod of type QuickMod*). We here need to fill the modsList with any dependency
	 * mods.
	 */

	ChooseQuickModVersionDialog *versionDialog = new ChooseQuickModVersionDialog;
	versionDialog->setMod(choosenMod, m_instance);
	if (versionDialog->exec() != QDialog::Accepted)
	{
		return;
	}
	modsList.insert(choosenMod, choosenMod->version(versionDialog->version()));

	dialog = new DownloadProgressDialog(choosenMod->dependentUrls().count(), this);
	connect(&*MMC->quickmodslist(), SIGNAL(modAdded(QuickMod *)), dialog,
			SLOT(modAdded(QuickMod *)));
	connect(&*MMC->quickmodslist(), SIGNAL(modAdded(QuickMod *)), this,
			SLOT(resolveSingleMod(QuickMod *)));
	resolveSingleMod(choosenMod);
	dialog->exec();
	delete dialog;
	disconnect(&*MMC->quickmodslist(), SIGNAL(modAdded(QuickMod *)), this,
			   SLOT(resolveSingleMod(QuickMod *)));
	foreach (auto mod, dialog->mods())
	{
		versionDialog = new ChooseQuickModVersionDialog;
		versionDialog->setMod(mod, m_instance);
		if (versionDialog->exec() != QDialog::Accepted)
		{
			return;
		}
		modsList.insert(mod, mod->version(versionDialog->version()));
	}

	// TODO check if we already have the file. redownload or use existing? ask user?
	// TODO download using the QuickModDownloadDialog
}
void ChooseInstallModDialog::resolveSingleMod(QuickMod *mod)
{
	for (auto url : mod->dependentUrls())
	{
		MMC->quickmodslist()->registerMod(url);
	}
}

void ChooseInstallModDialog::on_cancelButton_clicked()
{
	reject();
}

void ChooseInstallModDialog::on_categoriesLabel_linkActivated(const QString &link)
{
	ui->categoryBox->setCurrentText(link);
	ui->tagsEdit->setText(QString());
}
void ChooseInstallModDialog::on_tagsLabel_linkActivated(const QString &link)
{
	ui->tagsEdit->setText(ui->tagsEdit->text() + ", " + link);
	ui->categoryBox->setCurrentText(QString());
	on_tagsEdit_textChanged();
}
void ChooseInstallModDialog::on_fulltextEdit_textChanged()
{
	m_model->setFulltext(ui->fulltextEdit->text());
}
void ChooseInstallModDialog::on_tagsEdit_textChanged()
{
	m_model->setTags(ui->tagsEdit->text().split(QRegularExpression(", {0,1}"), QString::SkipEmptyParts));
}
void ChooseInstallModDialog::on_categoryBox_currentTextChanged()
{
	m_model->setCategory(ui->categoryBox->currentText());
}

void ChooseInstallModDialog::modSelectionChanged(const QItemSelection &selected,
												 const QItemSelection &deselected)
{
	if (selected.isEmpty())
	{
		ui->nameLabel->setText("");
		ui->descriptionLabel->setText("");
		ui->websiteLabel->setText("");
		ui->categoriesLabel->setText("");
		ui->tagsLabel->setText("");
		ui->logoLabel->setPixmap(QPixmap());
	}
	else
	{
		QuickMod *mod = m_model->index(selected.first().top(), 0)
							.data(QuickModsList::QuickModRole)
							.value<QuickMod *>();
		ui->nameLabel->setText(mod->name());
		ui->descriptionLabel->setText(mod->description());
		ui->websiteLabel->setText(QString("<a href=\"%1\">%2</a>")
									  .arg(mod->websiteUrl().toString(QUrl::FullyEncoded),
										   mod->websiteUrl().toString(QUrl::PrettyDecoded)));
		QStringList categories;
		foreach(const QString & category, mod->categories())
		{
			categories.append(QString("<a href=\"%1\">%1</a>").arg(category));
		}
		ui->categoriesLabel->setText(categories.join(", "));
		QStringList tags;
		foreach(const QString & tag, mod->tags())
		{
			tags.append(QString("<a href=\"%1\">%1</a>").arg(tag));
		}
		ui->tagsLabel->setText(tags.join(", "));
		ui->logoLabel->setPixmap(mod->logo());
	}
}

void ChooseInstallModDialog::setupCategoryBox()
{
	QStringList categories;
	categories.append("");

	for (int i = 0; i < MMC->quickmodslist()->numMods(); ++i)
	{
		categories.append(MMC->quickmodslist()->modAt(i)->categories());
	}

	categories.removeDuplicates();

	ui->categoryBox->clear();
	ui->categoryBox->addItems(categories);
}

#include "ChooseInstallModDialog.moc"

void ChooseInstallModDialog::on_addButton_clicked()
{
	AddQuickModFileDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted)
	{
		switch (dialog.type())
		{
		case AddQuickModFileDialog::FileName:
			MMC->quickmodslist()->registerMod(dialog.fileName());
			break;
		case AddQuickModFileDialog::Url:
			MMC->quickmodslist()->registerMod(dialog.url());
			break;
		}
	}
}

void ChooseInstallModDialog::on_updateButton_clicked()
{
	MMC->quickmodslist()->updateFiles();
}
