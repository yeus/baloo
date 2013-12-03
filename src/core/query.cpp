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

#include "query.h"
#include "term.h"
#include "searchstore.h"

#include <QString>
#include <QStringList>
#include <QList>

#include <KDebug>

#include <qjson/serializer.h>
#include <qjson/parser.h>

using namespace Baloo;

class Baloo::Query::Private {
public:
    Private() {
        m_limit = 100000;
    }
    Term m_term;

    QStringList m_types;
    QString m_searchString;
    uint m_limit;
};

Query::Query()
    : d(new Private)
{
}

Query::Query(const Term& t)
    : d(new Private)
{
    d->m_term = t;
}

void Query::setTerm(const Term& t)
{
    d->m_term = t;
}

Term Query::term() const
{
    return d->m_term;
}

void Query::addType(const QString& type)
{
    d->m_types << type.split('/', QString::SkipEmptyParts);
}

void Query::setTypes(const QStringList& types)
{
    d->m_types = types;
}

QStringList Query::types() const
{
    return d->m_types;
}

QString Query::searchString() const
{
    return d->m_searchString;
}

void Query::setSearchString(const QString& str)
{
    d->m_searchString = str;
}

void Query::addRelation(const Relation& rel)
{
    //TODO:
}

void Query::setRelations(const QList<Relation>& rel)
{
    //TODO:
}

QList<Relation> Query::relations() const
{
    //TODO:
    return QList<Relation>();
}

uint Query::limit() const
{
    return d->m_limit;
}

void Query::setLimit(uint limit)
{
    d->m_limit = limit;
}

ResultIterator Query::exec()
{
    // vHanda: Maybe this should default to allow searches on all search stores?
    Q_ASSERT_X(!types().isEmpty(), "Baloo::Query::exec", "A query is being initialized without a type");
    if (types().isEmpty())
        return ResultIterator();

    QList<SearchStore*> stores = SearchStore::searchStores();

    SearchStore* storeMatch = 0;
    Q_FOREACH (SearchStore* store, stores) {
        Q_FOREACH (const QString& type, types()) {
            if (store->types().contains(type)) {
                storeMatch = store;
                break;
            }
        }

        if (storeMatch)
            break;
    }

    if (!storeMatch)
        return ResultIterator();

    int id = storeMatch->exec(*this);
    return ResultIterator(id, storeMatch);
}

QByteArray Query::toJSON()
{
    QVariantMap map;
    map["type"] = d->m_types;
    map["limit"] = d->m_limit;
    map["searchString"] = d->m_searchString;
    map["term"] = QVariant(d->m_term.toVariantMap());

    QJson::Serializer serializer;
    return serializer.serialize(map);
}

// static
Query Query::fromJSON(const QByteArray& arr)
{
    QJson::Parser parser;
    QVariantMap map = parser.parse(arr).toMap();

    Query query;
    query.d->m_types = map["type"].toStringList();
    query.d->m_limit = map["limit"].toInt();
    query.d->m_searchString = map["searchString"].toString();
    query.d->m_term = Term::fromVariantMap(map["term"].toMap());

    return query;
}
