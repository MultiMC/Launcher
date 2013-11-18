#include "ChooseInstallModDialog.h"
#include "ui_ChooseInstallModDialog.h"

#include <QSortFilterProxyModel>

#include "depends/groupview/include/categorizedview.h"
#include "depends/groupview/include/categorizedsortfilterproxymodel.h"
#include "depends/groupview/include/categorydrawer.h"

#include "logic/lists/QuickModsList.h"
#include "gui/widgets/WebDownloadNavigator.h"
#include "DownloadProgressDialog.h"
#include "AddQuickModFileDialog.h"

#include "MultiMC.h"

class ModFilterProxyModel : public KCategorizedSortFilterProxyModel
{
    Q_OBJECT
public:
    ModFilterProxyModel(QObject *parent = 0) : KCategorizedSortFilterProxyModel(parent)
    {
        setCategorizedModel(true);
    }

    void setTag(const QString& tag)
    {
        m_tag = tag;
        m_category = QString();
        invalidateFilter();
    }
    void setCategory(const QString& category)
    {
        m_tag = QString();
        m_category = category;
        invalidateFilter();
    }
    void setFulltext(const QString& query)
    {
        m_fulltext = query;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        if (!m_tag.isNull())
        {
            if (!index.data(QuickModsList::TagsRole).toStringList().contains(m_tag))
            {
                return false;
            }
        }
        if (!m_category.isNull())
        {
            if (!index.data(QuickModsList::CategoriesRole).toStringList().contains(m_category))
            {
                return false;
            }
        }
        if (!m_fulltext.isEmpty())
        {
            bool inName = index.data(QuickModsList::NameRole).toString().contains(m_fulltext, Qt::CaseInsensitive);
            bool inDesc = index.data(QuickModsList::DescriptionRole).toString().contains(m_fulltext, Qt::CaseInsensitive);
            if (!inName && !inDesc)
            {
                return false;
            }
        }

        return true;
    }

private:
    QString m_tag;
    QString m_category;
    QString m_fulltext;
};

ChooseInstallModDialog::ChooseInstallModDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseInstallModDialog),
    m_view(new KCategorizedView(this)),
    m_model(new ModFilterProxyModel(this))
{
    ui->setupUi(this);

    ui->viewLayout->addWidget(m_view);

    m_view->setSelectionBehavior(KCategorizedView::SelectRows);
    m_view->setSelectionMode(KCategorizedView::SingleSelection);
    m_view->setCategoryDrawer(new KCategoryDrawer(m_view));
    m_model->setSourceModel(MMC->quickmodslist().get());
    m_view->setModel(m_model);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ChooseInstallModDialog::modSelectionChanged);
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
    QList<QuickMod*> modsList;

    modsList.append(choosenMod);
    DownloadProgressDialog dialog(choosenMod->dependentUrls().count(), this);
    connect(&*MMC->quickmodslist(), SIGNAL(modAdded(QuickMod*)), &dialog, SLOT(ModAdded(QuickMod*)));
    for(auto url : choosenMod->dependentUrls())
    {
        MMC->quickmodslist()->registerMod(url);
    }
    dialog.exec();
    modsList.append(dialog.mods());
}
void ChooseInstallModDialog::on_cancelButton_clicked()
{
    reject();
}

void ChooseInstallModDialog::on_categoriesLabel_linkActivated(const QString &link)
{
    m_model->setCategory(link);
}
void ChooseInstallModDialog::on_tagsLabel_linkActivated(const QString &link)
{
    m_model->setTag(link);
}
void ChooseInstallModDialog::on_fulltextEdit_textChanged()
{
    m_model->setFulltext(ui->fulltextEdit->text());
}

void ChooseInstallModDialog::modSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
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
        QuickMod* mod = m_model->index(selected.first().top(), 0).data(QuickModsList::QuickModRole).value<QuickMod*>();
        ui->nameLabel->setText(mod->name());
        ui->descriptionLabel->setText(mod->description());
        ui->websiteLabel->setText(QString("<a href=\"%1\">%2</a>").arg(mod->websiteUrl().toString(QUrl::FullyEncoded),
                                                                       mod->websiteUrl().toString(QUrl::PrettyDecoded)));
        QStringList categories;
        foreach (const QString& category, mod->categories())
        {
            categories.append(QString("<a href=\"%1\">%1</a>").arg(category));
        }
        ui->categoriesLabel->setText(categories.join(", "));
        QStringList tags;
        foreach (const QString& tag, mod->tags())
        {
            tags.append(QString("<a href=\"%1\">%1</a>").arg(tag));
        }
        ui->tagsLabel->setText(tags.join(", "));
        ui->logoLabel->setPixmap(mod->logo());
    }
}

#include "ChooseInstallModDialog.moc"

void ChooseInstallModDialog::on_pushButton_clicked()
{
    AddQuickModFileDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        switch (dialog.type()) {
        case AddQuickModFileDialog::FileName:
            MMC->quickmodslist()->registerMod(dialog.fileName());
            break;
        case AddQuickModFileDialog::Url:
            MMC->quickmodslist()->registerMod(dialog.url());
            break;
        }
    }
}

void ChooseInstallModDialog::on_pushButton_2_clicked()
{
    MMC->quickmodslist()->updateFiles();
}
