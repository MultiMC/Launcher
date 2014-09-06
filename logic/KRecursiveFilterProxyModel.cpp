/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "KRecursiveFilterProxyModel.h"

// Maintainability note:
// This class invokes some Q_PRIVATE_SLOTs in QSortFilterProxyModel which are
// private API and could be renamed or removed at any time.
// If they are renamed, the invocations can be updated with an #if (QT_VERSION(...))
// If they are removed, then layout{AboutToBe}Changed Q_SIGNALS should be used when the source model
// gets new rows or has rowsremoved or moved. The Q_PRIVATE_SLOT invocation is an optimization
// because layout{AboutToBe}Changed is expensive and causes the entire mapping of the tree in QSFPM
// to be cleared, even if only a part of it is dirty.
// Stephen Kelly, 30 April 2010.

class KRecursiveFilterProxyModelPrivate
{
    Q_DECLARE_PUBLIC(KRecursiveFilterProxyModel)
    KRecursiveFilterProxyModel *q_ptr;
public:
    KRecursiveFilterProxyModelPrivate(KRecursiveFilterProxyModel *model)
        : q_ptr(model),
          ignoreRemove(false),
          completeInsert(false),
          completeRemove(false)
    {
        qRegisterMetaType<QModelIndex>("QModelIndex");
    }

    // Convenience methods for invoking the QSFPM Q_SLOTS. Those slots must be invoked with invokeMethod
    // because they are Q_PRIVATE_SLOTs
    inline void invokeDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
    {
        Q_Q(KRecursiveFilterProxyModel);
        bool success = QMetaObject::invokeMethod(q, "_q_sourceDataChanged", Qt::DirectConnection,
                       Q_ARG(QModelIndex, topLeft),
                       Q_ARG(QModelIndex, bottomRight));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsInserted(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection,
                       Q_ARG(QModelIndex, source_parent),
                       Q_ARG(int, start),
                       Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsAboutToBeInserted", Qt::DirectConnection,
                       Q_ARG(QModelIndex, source_parent),
                       Q_ARG(int, start),
                       Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsRemoved(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsRemoved", Qt::DirectConnection,
                       Q_ARG(QModelIndex, source_parent),
                       Q_ARG(int, start),
                       Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsAboutToBeRemoved", Qt::DirectConnection,
                       Q_ARG(QModelIndex, source_parent),
                       Q_ARG(int, start),
                       Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
    void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);

    /**
      Given that @p index does not match the filter, clear mappings in the QSortFilterProxyModel up to and excluding the
      first ascendant that does match, and remake the mappings.

      If @p refreshAll is true, this method also refreshes intermediate mappings. This is significant when removing rows.
    */
    void refreshAscendantMapping(const QModelIndex &index, bool refreshAll = false);

    bool ignoreRemove;
    bool completeInsert;
    bool completeRemove;
};

void KRecursiveFilterProxyModelPrivate::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right)
{
    Q_Q(KRecursiveFilterProxyModel);

    QModelIndex source_parent = source_top_left.parent();

    if (!source_parent.isValid() || q->filterAcceptsRow(source_parent.row(), source_parent.parent())) {
        invokeDataChanged(source_top_left, source_bottom_right);
        return;
    }

    bool requireRow = false;
    for (int row = source_top_left.row(); row <= source_bottom_right.row(); ++row)
        if (q->filterAcceptsRow(row, source_parent)) {
            requireRow = true;
            break;
        }

    if (!requireRow) { // None of the changed rows are now required in the model.
        return;
    }

    refreshAscendantMapping(source_parent);
}

void KRecursiveFilterProxyModelPrivate::refreshAscendantMapping(const QModelIndex &index, bool refreshAll)
{
    Q_Q(KRecursiveFilterProxyModel);

    Q_ASSERT(index.isValid());
    QModelIndex lastAscendant = index;
    QModelIndex sourceAscendant = index.parent();
    // We got a matching descendant, so find the right place to insert the row.
    // We need to tell the QSortFilterProxyModel that the first child between an existing row in the model
    // has changed data so that it will get a mapping.
    while (sourceAscendant.isValid() && !q->acceptRow(sourceAscendant.row(), sourceAscendant.parent())) {
        if (refreshAll) {
            invokeDataChanged(sourceAscendant, sourceAscendant);
        }

        lastAscendant = sourceAscendant;
        sourceAscendant = sourceAscendant.parent();
    }

    // Inform the model that its data changed so that it creates new mappings and finds the rows which now match the filter.
    invokeDataChanged(lastAscendant, lastAscendant);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    if (!source_parent.isValid() || q->filterAcceptsRow(source_parent.row(), source_parent.parent())) {
        invokeRowsAboutToBeInserted(source_parent, start, end);
        completeInsert = true;
    }
}

void KRecursiveFilterProxyModelPrivate::sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    if (completeInsert) {
        completeInsert = false;
        invokeRowsInserted(source_parent, start, end);
        // If the parent is already in the model, we can just pass on the signal.
        return;
    }

    bool requireRow = false;
    for (int row = start; row <= end; ++row) {
        if (q->filterAcceptsRow(row, source_parent)) {
            requireRow = true;
            break;
        }
    }

    if (!requireRow) {
        // The row doesn't have descendants that match the filter. Filter it out.
        return;
    }

    refreshAscendantMapping(source_parent);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    if (source_parent.isValid() && q->filterAcceptsRow(source_parent.row(), source_parent.parent())) {
        invokeRowsAboutToBeRemoved(source_parent, start, end);
        completeRemove = true;
        return;
    }

    bool accepted = false;
    for (int row = start; row <= end; ++row) {
        if (q->filterAcceptsRow(row, source_parent)) {
            accepted = true;
            break;
        }
    }
    if (!accepted) {
        // All removed rows are already filtered out. We don't care about the signal.
        ignoreRemove = true;
        return;
    }
    completeRemove = true;
    invokeRowsAboutToBeRemoved(source_parent, start, end);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
    if (completeRemove) {
        completeRemove = false;
        // Source parent is already in the model.
        invokeRowsRemoved(source_parent, start, end);
        // fall through. After removing rows, we need to refresh things so that intermediates will be removed too if necessary.
    }

    if (ignoreRemove) {
        ignoreRemove = false;
        return;
    }

    // Refresh intermediate rows too.
    // This is needed because QSFPM only invalidates the mapping for the
    // index range given to dataChanged, not its children.
    if (source_parent.isValid()) {
        refreshAscendantMapping(source_parent, true);
    }
}

KRecursiveFilterProxyModel::KRecursiveFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent), d_ptr(new KRecursiveFilterProxyModelPrivate(this))
{
    setDynamicSortFilter(true);
}

KRecursiveFilterProxyModel::~KRecursiveFilterProxyModel()
{
    delete d_ptr;
}

bool KRecursiveFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // TODO: Implement some caching so that if one match is found on the first pass, we can return early results
    // when the subtrees are checked by QSFPM.
    if (acceptRow(sourceRow, sourceParent)) {
        return true;
    }

    QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
    Q_ASSERT(source_index.isValid());
    bool accepted = false;

    for (int row = 0; row < sourceModel()->rowCount(source_index); ++row)
        if (filterAcceptsRow(row, source_index)) {
            accepted = true;    // Need to do this in a loop so that all siblings in a parent get processed, not just the first.
        }

    return accepted;
}

QModelIndexList KRecursiveFilterProxyModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole) {
        return QSortFilterProxyModel::match(start, role, value, hits, flags);
    }

    QModelIndexList list;
    QModelIndex proxyIndex;
    Q_FOREACH (const QModelIndex &idx, sourceModel()->match(mapToSource(start), role, value, hits, flags)) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list << proxyIndex;
        }
    }

    return list;
}

bool KRecursiveFilterProxyModel::acceptRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void KRecursiveFilterProxyModel::setSourceModel(QAbstractItemModel *model)
{
    // Standard disconnect.
    disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

    disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
               this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
               this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

    QSortFilterProxyModel::setSourceModel(model);

    // Disconnect in the QSortFilterProxyModel. These methods will be invoked manually
    // in invokeDataChanged, invokeRowsInserted etc.
    //
    // The reason for that is that when the source model adds new rows for example, the new rows
    // May not match the filter, but maybe their child items do match.
    //
    // Source model before insert:
    //
    // - A
    // - B
    // - - C
    // - - D
    // - - - E
    // - - - F
    // - - - G
    // - H
    // - I
    //
    // If the A F and L (which doesn't exist in the source model yet) match the filter
    // the proxy will be:
    //
    // - A
    // - B
    // - - D
    // - - - F
    //
    // New rows are inserted in the source model below H:
    //
    // - A
    // - B
    // - - C
    // - - D
    // - - - E
    // - - - F
    // - - - G
    // - H
    // - - J
    // - - K
    // - - - L
    // - I
    //
    // As L matches the filter, it should be part of the KRecursiveFilterProxyModel.
    //
    // - A
    // - B
    // - - D
    // - - - F
    // - H
    // - - K
    // - - - L
    //
    // when the QSortFilterProxyModel gets a notification about new rows in H, it only checks
    // J and K to see if they match, ignoring L, and therefore not adding it to the proxy.
    // To work around that, we make sure that the QSFPM slot which handles that change in
    // the source model (_q_sourceRowsAboutToBeInserted) does not get called directly.
    // Instead we connect the sourceModel signal to our own slot in *this (sourceRowsAboutToBeInserted)
    // Inside that method, the entire new subtree is queried (J, K *and* L) to see if there is a match,
    // then the relevant Q_SLOTS in QSFPM are invoked.
    // In the example above, we need to tell the QSFPM that H should be queried again to see if
    // it matches the filter. It did not before, because L did not exist before. Now it does. That is
    // achieved by telling the QSFPM that the data changed for H, which causes it to requery this class
    // to see if H matches the filter (which it now does as L now exists).
    // That is done in refreshAscendantMapping.

    disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

    disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
               this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
               this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
               this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

    disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

    // Slots for manual invoking of QSortFilterProxyModel methods.
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

    connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

}

#include "moc_KRecursiveFilterProxyModel.cpp"

