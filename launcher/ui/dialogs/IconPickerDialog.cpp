/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QKeyEvent>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include "Application.h"

#include "IconPickerDialog.h"
#include "ui_IconPickerDialog.h"

#include "ui/instanceview/InstanceDelegate.h"
#include "IconPickerModel.h"

#include "icons/IconList.h"
#include "icons/IconUtils.h"
#include <DesktopServices.h>

IconPickerDialog::IconPickerDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::IconPickerDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    auto contentsWidget = ui->iconView;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setFlow(QListView::LeftToRight);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setResizeMode(QListView::Adjust);
    contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    contentsWidget->setSpacing(5);
    contentsWidget->setWordWrap(false);
    contentsWidget->setWrapping(true);
    contentsWidget->setUniformItemSizes(true);
    contentsWidget->setTextElideMode(Qt::ElideRight);
    contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    contentsWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setItemDelegate(new ListViewDelegate());

    // contentsWidget->setAcceptDrops(true);
    contentsWidget->setDropIndicatorShown(true);
    contentsWidget->viewport()->setAcceptDrops(true);
    contentsWidget->setDragDropMode(QAbstractItemView::DropOnly);
    contentsWidget->setDefaultDropAction(Qt::CopyAction);

    contentsWidget->installEventFilter(this);

    // renamed in place with F2 or Rename Icon; a double click picks the icon
    contentsWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);
    contentsWidget->setModel(new IconPickerModel(APPLICATION->icons().get(), [this](const QString &key, const QString &newKey) {
        const QString error = APPLICATION->icons()->renameIcon(key, newKey);
        if (!error.isEmpty())
        {
            QMessageBox::warning(this, tr("Can't rename the icon"), error);
            // back to the icons, for another try (F2)
            ui->iconView->setFocus();
            return false;
        }
        // the renamed icon, where it is in the list now, once the editor is gone
        QTimer::singleShot(0, this, [this, newKey]() { selectIcon(newKey); });
        return true;
    }, this));

    // NOTE: ResetRole forces the button to be on the left, while the OK/Cancel ones are on the right. We win.
    auto buttonAdd = ui->buttonBox->addButton(tr("Add Icon"), QDialogButtonBox::ResetRole);
    buttonRemove = ui->buttonBox->addButton(tr("Remove Icon"), QDialogButtonBox::ResetRole);
    buttonRename = ui->buttonBox->addButton(tr("Rename Icon"), QDialogButtonBox::ResetRole);
    buttonDefault = ui->buttonBox->addButton(tr("Set as Default"), QDialogButtonBox::ResetRole);
    buttonDefault->setToolTip(tr("Show this icon for new instances, and for instances without an icon of their own."));

    connect(buttonAdd, SIGNAL(clicked(bool)), SLOT(addNewIcon()));
    connect(buttonRemove, SIGNAL(clicked(bool)), SLOT(removeSelectedIcon()));
    connect(buttonRename, &QPushButton::clicked, this, &IconPickerDialog::renameSelectedIcon);
    connect(buttonDefault, &QPushButton::clicked, this, &IconPickerDialog::setSelectedAsDefault);

    connect(contentsWidget, SIGNAL(doubleClicked(QModelIndex)), SLOT(activated(QModelIndex)));

    connect(contentsWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SLOT(selectionChanged(QItemSelection, QItemSelection)));
    // a file-based icon can turn into a built-in one when its file goes away; the default can
    // change
    connect(APPLICATION->icons().get(), &QAbstractItemModel::dataChanged, this, &IconPickerDialog::updateButtons);

    auto buttonFolder = ui->buttonBox->addButton(tr("Open Folder"), QDialogButtonBox::ResetRole);
    connect(buttonFolder, &QPushButton::clicked, this, &IconPickerDialog::openFolder);
}

bool IconPickerDialog::eventFilter(QObject *obj, QEvent *evt)
{
    if (obj != ui->iconView)
        return QDialog::eventFilter(obj, evt);
    if (evt->type() != QEvent::KeyPress)
    {
        return QDialog::eventFilter(obj, evt);
    }
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(evt);
    switch (keyEvent->key())
    {
    case Qt::Key_Delete:
        removeSelectedIcon();
        return true;
    case Qt::Key_Plus:
        addNewIcon();
        return true;
    default:
        break;
    }
    return QDialog::eventFilter(obj, evt);
}

void IconPickerDialog::addNewIcon()
{
    //: The title of the select icons open file dialog
    QString selectIcons = tr("Select Icons");
    //: The type of icon files
    auto filter = IconUtils::getIconFilter();
    QStringList fileNames = QFileDialog::getOpenFileNames(this, selectIcons, QString(), tr("Icons %1").arg(filter));
    APPLICATION->icons()->installIcons(fileNames);
}

void IconPickerDialog::removeSelectedIcon()
{
    APPLICATION->icons()->deleteIcon(selectedIconKey);
}

void IconPickerDialog::renameSelectedIcon()
{
    ui->iconView->edit(ui->iconView->currentIndex());
}

void IconPickerDialog::setSelectedAsDefault()
{
    // the logo is the default when none is set
    APPLICATION->settings()->set("DefaultInstanceIcon", selectedIconKey == "logo" ? QString() : selectedIconKey);
    updateButtons();
}

void IconPickerDialog::accept()
{
    if (initialKey == "default" && selectedIconKey == APPLICATION->icons()->defaultIconKey())
        selectedIconKey = initialKey;
    QDialog::accept();
}

void IconPickerDialog::activated(QModelIndex index)
{
    selectedIconKey = index.data(Qt::UserRole).toString();
    accept();
}

void IconPickerDialog::selectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if (selected.empty())
    {
        updateButtons();
        return;
    }

    QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
    if (!key.isEmpty()) {
        selectedIconKey = key;
    }
    updateButtons();
}

void IconPickerDialog::updateButtons()
{
    // only icons from the icons folder can be removed and renamed, built-in ones can't
    auto list = APPLICATION->icons();
    auto icon = list->icon(selectedIconKey);
    const bool fileBased = icon && icon->has(IconType::FileBased);
    buttonRemove->setEnabled(fileBased);
    buttonRename->setEnabled(fileBased);
    buttonDefault->setEnabled(icon && list->getIconIndex(selectedIconKey) != list->getIconIndex("default"));
}

void IconPickerDialog::selectIcon(const QString &key)
{
    auto contentsWidget = ui->iconView;
    auto model_index = contentsWidget->model()->index(APPLICATION->icons()->getIconIndex(key), 0);
    selectedIconKey = model_index.isValid() ? model_index.data(Qt::UserRole).toString() : key;
    contentsWidget->selectionModel()->setCurrentIndex(model_index, QItemSelectionModel::ClearAndSelect);
    updateButtons();
    QMetaObject::invokeMethod(this, "delayed_scroll", Qt::QueuedConnection, Q_ARG(QModelIndex, model_index));
}

int IconPickerDialog::execWithSelection(QString selection)
{
    initialKey = selection;
    selectIcon(selection);
    return QDialog::exec();
}

void IconPickerDialog::delayed_scroll(QModelIndex model_index)
{
    auto contentsWidget = ui->iconView;
    contentsWidget->scrollTo(model_index);
}

IconPickerDialog::~IconPickerDialog()
{
    delete ui;
}

void IconPickerDialog::openFolder()
{
    DesktopServices::openDirectory(APPLICATION->icons()->getDirectory());
}
