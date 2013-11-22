/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Vishesh Handa <me@vhanda.in>
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

#include "commitqueue.h"
#include "database.h"

#include <KDebug>

Baloo::CommitQueue::CommitQueue(Database* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
    m_smallTimer.setSingleShot(true);
    m_smallTimer.setInterval(200);
    connect(&m_smallTimer, SIGNAL(timeout()), this, SLOT(commit()));

    m_largeTimer.setSingleShot(true);
    m_largeTimer.setInterval(10000);
    connect(&m_largeTimer, SIGNAL(timeout()), this, SLOT(commit()));
}

Baloo::CommitQueue::~CommitQueue()
{
    commit();
}

void Baloo::CommitQueue::add(unsigned id, Xapian::Document doc)
{
    m_docsToAdd << qMakePair(id, doc);
    startTimers();
}

void Baloo::CommitQueue::remove(unsigned int docid)
{
    kDebug() << docid;
    m_docsToRemove << docid;
    startTimers();
}

void Baloo::CommitQueue::startTimers()
{
    m_smallTimer.start();
    if (!m_largeTimer.isActive()) {
        m_largeTimer.start();
    }
}


void Baloo::CommitQueue::commit()
{
    kDebug() << "COMMITTING";
    m_db->sqlDatabase().commit();
    m_db->sqlDatabase().transaction();

    const std::string path = m_db->path().toStdString();
    Xapian::WritableDatabase db(path, Xapian::DB_CREATE_OR_OPEN);

    Q_FOREACH (const DocIdPair& doc, m_docsToAdd) {
        db.replace_document(doc.first, doc.second);
    }
    m_docsToAdd.clear();

    Q_FOREACH (Xapian::docid id, m_docsToRemove) {
        db.delete_document(id);
    }
    m_docsToRemove.clear();

    db.commit();
    m_db->xapainDatabase()->reopen();

    Q_EMIT committed();
}