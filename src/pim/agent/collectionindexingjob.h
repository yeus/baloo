/*
 * Copyright 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef COLLECTIONINDEXINGJOB_H
#define COLLECTIONINDEXINGJOB_H

#include <KJob>
#include <Akonadi/Item>
#include <Akonadi/Collection>
#include <QTime>
#include "index.h"

/**
 * Indexing Job that ensure a collections is fully indexed.
 * The following steps are required to bring the index up-to date:
 * 1. Index pending items
 * 2. Check if indexed item == local items (optimization)
 * 3. Make a full diff if necessary
 */
class CollectionIndexingJob : public KJob
{
    Q_OBJECT
public:
    explicit CollectionIndexingJob(Index& index, const Akonadi::Collection& col, const QList<Akonadi::Item::Id>& pending, QObject* parent = 0);
    void setFullSync(bool);
    virtual void start();

Q_SIGNALS:
    void status(int, QString);
    void percent(int);

private Q_SLOTS:
    void slotOnCollectionFetched(KJob*);
    void slotPendingItemsReceived(const Akonadi::Item::List& items);
    void slotPendingIndexed(KJob*);
    void slotUnindexedItemsReceived(const Akonadi::Item::List& items);
    void slotFoundUnindexed(KJob*);

private:
    void findUnindexed();
    void indexItems(const QList<Akonadi::Item::Id>&itemIds);

    Akonadi::Collection m_collection;
    QList<Akonadi::Item::Id> m_pending;
    QSet<Akonadi::Item::Id> m_indexedItems;
    QList<Akonadi::Item::Id> m_needsIndexing;
    Index& m_index;
    QTime m_time;
    bool m_reindexingLock;
    bool m_fullSync;
    int m_progressCounter;
    int m_progressTotal;
};

#endif