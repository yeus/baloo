/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "queryparser.h"

#include <QTextBoundaryFinder>
#include <QStringList>
#include <QDebug>

using namespace Baloo;

QueryParser::QueryParser()
    : m_db(0)
{
}

void QueryParser::setDatabase(Xapian::Database* db)
{
    m_db = db;
}

namespace {
    struct Term {
        std::string t;
        uint count;

        // pop_heap pops the largest element, we want the smallest to be popped
        bool operator < (const Term& rhs) const {
            return count > rhs.count;
        }
    };

    Xapian::Query makeQuery(const QString& string, int position, Xapian::Database* db)
    {
        if (!db) {
            QByteArray arr = string.toUtf8();
            std::string stdString(arr.constData(), arr.size());
            return Xapian::Query(stdString, 1, position);
        }

        // Lets just keep the top x (+1 for push_heap)
        static const int MaxTerms = 100;
        QList<Term> topTerms;
        topTerms.reserve(MaxTerms + 1);

        const std::string stdString(string.toLower().toUtf8().constData());
        Xapian::TermIterator it = db->allterms_begin(stdString);
        Xapian::TermIterator end = db->allterms_end(stdString);
        for (; it != end; ++it) {
            Term term;
            term.t = *it;
            term.count = db->get_collection_freq(term.t);

            if (topTerms.size() < MaxTerms) {
                topTerms.push_back(term);
                std::push_heap(topTerms.begin(), topTerms.end());
            }
            else {
                // Remove the term with the min count
                topTerms.push_back(term);
                std::push_heap(topTerms.begin(), topTerms.end());

                std::pop_heap(topTerms.begin(), topTerms.end());
                topTerms.pop_back();
            }
        }

        QVector<Xapian::Query> queries;
        queries.reserve(topTerms.size());

        Q_FOREACH (const Term& term, topTerms) {
            queries << Xapian::Query(term.t, 1, position);
        }

        Xapian::Query finalQ(Xapian::Query::OP_SYNONYM, queries.begin(), queries.end());
        return finalQ;
    }

    bool containsSpace(const QString& string) {
        Q_FOREACH (const QChar& ch, string) {
            if (ch.isSpace())
                return true;
        }

        return false;
    }
}

Xapian::Query QueryParser::parseQuery(const QString& text)
{
    /*
    Xapian::QueryParser parser;
    parser.set_default_op(Xapian::Query::OP_AND);

    if (m_db)
        parser.set_database(*m_db);

    int flags = Xapian::QueryParser::FLAG_PHRASE | Xapian::QueryParser::FLAG_PARTIAL;

    std::string stdString(text.toUtf8().constData());
    return parser.parse_query(stdString, flags);
    */

    if (text.isEmpty()) {
        return Xapian::Query();
    }

    QList<Xapian::Query> queries;
    QList<Xapian::Query> phraseQueries;

    int start = 0;
    int end = 0;
    int position = 0;

    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;
    bool inPhrase = false;

    QTextBoundaryFinder bf(QTextBoundaryFinder::Word, text);
    for (; bf.position() != -1; bf.toNextBoundary()) {
        if (bf.boundaryReasons() & QTextBoundaryFinder::StartOfItem) {
            //
            // Check the previous delimiter
            int pos = bf.position();
            if (pos != end) {
                QString delim = text.mid(end, pos-end);
                if (delim.contains(QLatin1Char('"'))) {
                    if (inDoubleQuotes) {
                        queries << Xapian::Query(Xapian::Query::OP_PHRASE, phraseQueries.begin(), phraseQueries.end());
                        phraseQueries.clear();
                        inDoubleQuotes = false;
                    }
                    else {
                        inDoubleQuotes = true;
                    }
                }
                else if (delim.contains(QLatin1Char('\''))) {
                    if (inSingleQuotes) {
                        queries << Xapian::Query(Xapian::Query::OP_PHRASE, phraseQueries.begin(), phraseQueries.end());
                        phraseQueries.clear();
                        inSingleQuotes = false;
                    }
                    else {
                        inSingleQuotes = true;
                    }
                }
                else if (!containsSpace(delim)) {
                    if (!inPhrase) {
                        phraseQueries << queries.takeLast();
                    }
                    inPhrase = true;
                }
                else if (inPhrase && !phraseQueries.isEmpty()) {
                    queries << Xapian::Query(Xapian::Query::OP_PHRASE, phraseQueries.begin(), phraseQueries.end());
                    phraseQueries.clear();
                    inPhrase = false;
                }
            }

            start = bf.position();
            continue;
        }
        else if (bf.boundaryReasons() & QTextBoundaryFinder::EndOfItem) {
            end = bf.position();

            QString str = text.mid(start, end - start);

            // Get the string ready for saving
            str = str.toLower();

            // Remove all accents
            const QString denormalized = str.normalized(QString::NormalizationForm_KD);
            QString cleanString;
            Q_FOREACH (const QChar& ch, denormalized) {
                auto cat = ch.category();
                if (cat != QChar::Mark_NonSpacing && cat != QChar::Mark_SpacingCombining && cat != QChar::Mark_Enclosing) {
                    cleanString.append(ch);
                }
            }

            str = cleanString.normalized(QString::NormalizationForm_KC);
            Q_FOREACH (const QString& term, str.split(QLatin1Char('_'), QString::SkipEmptyParts)) {
                position++;
                if (inDoubleQuotes || inSingleQuotes || inPhrase) {
                    phraseQueries << makeQuery(term, position, m_db);
                }
                else {
                    queries << makeQuery(term, position, m_db);
                }
            }
        }
    }

    if (inPhrase) {
        queries << Xapian::Query(Xapian::Query::OP_PHRASE, phraseQueries.begin(), phraseQueries.end());
        phraseQueries.clear();
        inPhrase = false;
    }

    if (!phraseQueries.isEmpty()) {
        queries << phraseQueries;
        phraseQueries.clear();
    }

    return Xapian::Query(Xapian::Query::OP_AND, queries.begin(), queries.end());
}
