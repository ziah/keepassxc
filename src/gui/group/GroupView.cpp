/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GroupView.h"

#include <QDragMoveEvent>
#include <QMetaObject>
#include <QMimeData>
#include <QShortcut>

#include "core/Database.h"
#include "core/Group.h"
#include "gui/group/GroupModel.h"

GroupView::GroupView(Database* db, QWidget* parent)
    : QTreeView(parent)
    , m_model(new GroupModel(db, this))
    , m_updatingExpanded(false)
{
    QTreeView::setModel(m_model);
    setHeaderHidden(true);
    setUniformRowHeights(true);

    // clang-format off
    connect(this, SIGNAL(expanded(QModelIndex)), SLOT(expandedChanged(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(expandedChanged(QModelIndex)));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(syncExpandedState(QModelIndex,int,int)));
    connect(m_model, SIGNAL(modelReset()), SLOT(modelReset()));
    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(emitGroupChanged()));
    // clang-format on

    new QShortcut(Qt::CTRL + Qt::Key_F10, this, SLOT(contextMenuShortcutPressed()), nullptr, Qt::WidgetShortcut);

    modelReset();

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
}

void GroupView::contextMenuShortcutPressed()
{
    auto index = currentIndex();
    if (hasFocus() && index.isValid()) {
        emit customContextMenuRequested(visualRect(index).bottomLeft());
    }
}

void GroupView::changeDatabase(const QSharedPointer<Database>& newDb)
{
    m_model->changeDatabase(newDb.data());
}

void GroupView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->keyboardModifiers() & Qt::ControlModifier) {
        event->setDropAction(Qt::CopyAction);
    } else {
        event->setDropAction(Qt::MoveAction);
    }

    QTreeView::dragMoveEvent(event);

    // entries may only be dropped on groups
    if (event->isAccepted() && event->mimeData()->hasFormat("application/x-keepassx-entry")
        && (dropIndicatorPosition() == AboveItem || dropIndicatorPosition() == BelowItem)) {
        event->ignore();
    }
}

void GroupView::focusInEvent(QFocusEvent* event)
{
    emitGroupChanged();
    QTreeView::focusInEvent(event);
}

Group* GroupView::currentGroup()
{
    if (currentIndex() == QModelIndex()) {
        return nullptr;
    } else {
        return m_model->groupFromIndex(currentIndex());
    }
}

void GroupView::expandedChanged(const QModelIndex& index)
{
    if (m_updatingExpanded) {
        return;
    }

    Group* group = m_model->groupFromIndex(index);
    group->setExpanded(isExpanded(index));
}

void GroupView::recInitExpanded(Group* group)
{
    m_updatingExpanded = true;
    expandGroup(group, group->isExpanded());
    m_updatingExpanded = false;

    const QList<Group*> children = group->children();
    for (Group* child : children) {
        recInitExpanded(child);
    }
}

void GroupView::expandGroup(Group* group, bool expand)
{
    QModelIndex index = m_model->index(group);
    setExpanded(index, expand);
}

void GroupView::sortGroups(bool reverse)
{
    Group* group = currentGroup();
    if (group) {
        m_model->sortChildren(group, reverse);
    }
}

void GroupView::setModel(QAbstractItemModel* model)
{
    Q_UNUSED(model);
    Q_ASSERT(false);
}

void GroupView::emitGroupChanged()
{
    emit groupSelectionChanged(currentGroup());
}

void GroupView::syncExpandedState(const QModelIndex& parent, int start, int end)
{
    for (int row = start; row <= end; row++) {
        Group* group = m_model->groupFromIndex(m_model->index(row, 0, parent));
        recInitExpanded(group);
    }
}

void GroupView::setCurrentGroup(Group* group)
{
    if (group == nullptr)
        setCurrentIndex(QModelIndex());
    else
        setCurrentIndex(m_model->index(group));
}

void GroupView::modelReset()
{
    recInitExpanded(m_model->groupFromIndex(m_model->index(0, 0)));
    setCurrentIndex(m_model->index(0, 0));
}
