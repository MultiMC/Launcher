/* Copyright 2013-2015 MultiMC Contributors
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

#include "PageContainer.h"

#include <QStackedLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QListView>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QDesktopServices>

#include "MultiMC.h"
#include "logic/settings/SettingsObject.h"
#include "gui/widgets/IconLabel.h"
#include "gui/Platform.h"
#include "PageContainer_p.h"

class PageEntryFilterModel : public QSortFilterProxyModel
{
public:
	explicit PageEntryFilterModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
	{
	}

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
	{
		const QString pattern = filterRegExp().pattern();
		const auto model = static_cast<PageModel *>(sourceModel());
		const auto page = model->pages().at(sourceRow);
		if (!page->shouldDisplay())
			return false;
		// Regular contents check, then check page-filter.
		return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
	}
};

PageContainer::PageContainer(BasePageProviderPtr pageProvider, QString defaultId,
							 QWidget *parent)
	: QWidget(parent)
{
	createUI();
	m_model = new PageModel(this);
	m_proxyModel = new PageEntryFilterModel(this);
	int firstIndex = -1;
	int counter = 0;
	auto pages = pageProvider->getPages();
	for (auto page : pages)
	{
		page->stackIndex = m_pageStack->addWidget(dynamic_cast<QWidget *>(page));
		page->listIndex = counter;
		counter++;
		if (firstIndex == -1)
		{
			firstIndex = page->stackIndex;
		}
	}
	m_model->setPages(pages);

	m_proxyModel->setSourceModel(m_model);
	m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

	m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
	m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_pageList->setModel(m_proxyModel);
	connect(m_pageList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
			this, SLOT(currentChanged(QModelIndex)));
	m_pageStack->setStackingMode(QStackedLayout::StackOne);
	m_pageList->setFocus();
	// now find what we want to have selected...
	auto page = m_model->findPageEntryById(defaultId);
	QModelIndex index;
	if (page)
	{
		index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
	}
	else
	{
		index = m_proxyModel->index(0, 0);
	}
	if (index.isValid())
		m_pageList->setCurrentIndex(index);
}

void PageContainer::createUI()
{
	m_pageStack = new QStackedLayout;
	m_filter = new QLineEdit;
	m_pageList = new PageView;
	m_header = new QLabel();
	m_iconHeader = new IconLabel(this, QIcon(), QSize(24, 24));

	QFont headerLabelFont = m_header->font();
	headerLabelFont.setBold(true);
	const int pointSize = headerLabelFont.pointSize();
	if (pointSize > 0)
		headerLabelFont.setPointSize(pointSize + 2);
	m_header->setFont(headerLabelFont);

	QHBoxLayout *headerHLayout = new QHBoxLayout;
	const int leftMargin = MMC->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
	headerHLayout->addSpacerItem(
		new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
	headerHLayout->addWidget(m_header);
	headerHLayout->addSpacerItem(
		new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
	headerHLayout->addWidget(m_iconHeader);
	const int rightMargin = MMC->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
	headerHLayout->addSpacerItem(
		new QSpacerItem(rightMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));

	m_pageStack->setMargin(0);
	m_pageStack->addWidget(new QWidget(this));

	m_layout = new QGridLayout;
	m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
	m_layout->addWidget(m_pageList, 0, 0, 2, 1);
	m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
	m_layout->setColumnStretch(1, 4);
	setLayout(m_layout);
}

void PageContainer::addButtons(QWidget *buttons)
{
	m_layout->addWidget(buttons, 2, 0, 1, 2);
}

void PageContainer::addButtons(QLayout *buttons)
{
	m_layout->addLayout(buttons, 2, 0, 1, 2);
}

void PageContainer::showPage(int row)
{
	if (m_currentPage)
	{
		m_currentPage->closed();
	}
	if (row != -1)
	{
		m_currentPage = m_model->pages().at(row);
	}
	else
	{
		m_currentPage = nullptr;
	}
	if (m_currentPage)
	{
		m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
		m_header->setText(m_currentPage->displayName());
		m_iconHeader->setIcon(m_currentPage->icon());
		m_currentPage->opened();
	}
	else
	{
		m_pageStack->setCurrentIndex(0);
		m_header->setText(QString());
		m_iconHeader->setIcon(QIcon::fromTheme("bug"));
	}
}

void PageContainer::help()
{
	if (m_currentPage)
	{
		QString pageId = m_currentPage->helpPage();
		if (pageId.isEmpty())
			return;
		QDesktopServices::openUrl(QUrl("https://github.com/MultiMC/MultiMC5/wiki/" + pageId));
	}
}

void PageContainer::currentChanged(const QModelIndex &current)
{
	showPage(current.isValid() ? m_proxyModel->mapToSource(current).row() : -1);
}

bool PageContainer::requestClose(QCloseEvent *event)
{
	for (auto page : m_model->pages())
	{
		if (!page->apply())
			return false;
	}
	if (m_currentPage)
	{
		m_currentPage->closed();
	}
	return true;
}
