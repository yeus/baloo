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

#include "filesearchstore.h"
#include "item.h"
#include "term.h"
#include "query.h"
#include "filemapping.h"

#include <xapian.h>
#include <QVector>

#include <KStandardDirs>
#include <KDebug>

using namespace Baloo;

FileSearchStore::FileSearchStore(QObject* parent, const QVariantList&)
    : SearchStore(parent)
    , m_sqlDb(0)
    , m_mutex(QMutex::Recursive)
{
    m_dbPath = KStandardDirs::locateLocal("data", "baloo/file/");
    m_nextId = 1;

    setDbPath(m_dbPath);
}

FileSearchStore::~FileSearchStore()
{
    const QString conName = m_sqlDb->connectionName();
    delete m_sqlDb;
    QSqlDatabase::removeDatabase(conName);
}

void FileSearchStore::setDbPath(const QString& path)
{
    m_dbPath = path;

    const QString conName = "filesearchstore" + QString::number(qrand());

    delete m_sqlDb;
    m_sqlDb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE3", conName));
    m_sqlDb->setDatabaseName(m_dbPath + "/fileMap.sqlite3");
    m_sqlDb->open();
}

QStringList FileSearchStore::types()
{
    return QStringList() << "File";
}

Xapian::Query FileSearchStore::toXapianQuery(Xapian::Query::op op, const QList<Term>& terms)
{
    Q_ASSERT_X(op == Xapian::Query::OP_AND || op == Xapian::Query::OP_OR,
               "FileSearchStore::toXapianQuery", "The op must be AND / OR");

    QVector<Xapian::Query> queries;
    queries.reserve(terms.size());

    Q_FOREACH (const Term& term, terms) {
        Xapian::Query q = toXapianQuery(term);
        queries << q;
    }

    return Xapian::Query(op, queries.begin(), queries.end());
}

Xapian::Query FileSearchStore::toXapianQuery(const Term& term)
{
    if (term.operation() == Term::And) {
        return toXapianQuery(Xapian::Query::OP_AND, term.subTerms());
    }
    if (term.operation() == Term::Or) {
        return toXapianQuery(Xapian::Query::OP_OR, term.subTerms());
    }

    if (term.property().isEmpty())
        return Xapian::Query();

    // FIXME: Need some way to check if only a property exists!
    if (!term.value().isNull()) {
        return Xapian::Query();
    }

    // Both property and value are non empty
    // FIXME: How to convert the property to the appropriate prefix?
    if (term.comparator() == Term::Contains) {
        Xapian::QueryParser parser;

        std::string prefix = term.property().toUpper().toStdString();
        std::string str = term.value().toString().toStdString();
        return parser.parse_query(str, Xapian::QueryParser::FLAG_DEFAULT, prefix);
    }

    // FIXME: We use equals in all other conditions
    //if (term.comparator() == Term::Equal) {
        return Xapian::Query(term.value().toString().toStdString());
    //}
}

namespace {
    Xapian::Query andQuery(const Xapian::Query& a, const Xapian::Query& b)
    {
        if (a.empty() && !b.empty())
            return b;
        if (!a.empty() && b.empty())
            return a;
        if (a.empty() && b.empty())
            return Xapian::Query();
        else
            return Xapian::Query(Xapian::Query::OP_AND, a, b);
    }
}

int FileSearchStore::exec(const Query& query)
{
    QMutexLocker lock(&m_mutex);
    Xapian::Database db(m_dbPath.toStdString());

    Xapian::Query xapQ = toXapianQuery(query.term());
    if (query.searchString().size()) {
        std::string str = query.searchString().toStdString();

        Xapian::QueryParser parser;
        parser.set_database(db);

        int flags = Xapian::QueryParser::FLAG_DEFAULT | Xapian::QueryParser::FLAG_PARTIAL;
        Xapian::Query q = parser.parse_query(str, flags);

        xapQ = andQuery(xapQ, q);
    }

    Q_FOREACH (const QString& type, query.types()) {
        QString t = 'T' + type.toLower();
        if (t == "Tfile")
            continue;

        xapQ = andQuery(xapQ, Xapian::Query(t.toStdString()));
    }

    Xapian::Enquire enquire(db);
    enquire.set_query(xapQ);

    Result& res = m_queryMap[m_nextId++];
    res.mset = enquire.get_mset(0, query.limit());
    res.it = res.mset.begin();

    return m_nextId-1;
}

void FileSearchStore::close(int queryId)
{
    QMutexLocker lock(&m_mutex);
    m_queryMap.remove(queryId);
}

Item::Id FileSearchStore::id(int queryId)
{
    QMutexLocker lock(&m_mutex);
    Q_ASSERT_X(m_queryMap.contains(queryId), "FileSearchStore::id",
               "Passed a queryId which does not exist");

    Result& res = m_queryMap[queryId];
    if (!res.lastId)
        return QByteArray();

    return serialize("file", res.lastId);
}

QUrl FileSearchStore::url(int queryId)
{
    QMutexLocker lock(&m_mutex);
    Result& res = m_queryMap[queryId];

    if (!res.lastId)
        return QUrl();

    if (!res.lastUrl.isEmpty())
        return res.lastUrl;

    FileMapping file(res.lastId);
    file.fetch(*m_sqlDb);

    res.lastUrl = QUrl::fromLocalFile(file.url());
    return res.lastUrl;
}

bool FileSearchStore::next(int queryId)
{
    QMutexLocker lock(&m_mutex);
    Result& res = m_queryMap[queryId];

    bool atEnd = (res.it == res.mset.end());
    if (atEnd) {
        res.lastId = 0;
        res.lastUrl.clear();
    }
    else {
        res.lastId = *res.it;
        res.lastUrl.clear();
        res.it++;
    }

    return !atEnd;
}

BALOO_EXPORT_SEARCHSTORE(Baloo::FileSearchStore, "baloo_filesearchstore")
