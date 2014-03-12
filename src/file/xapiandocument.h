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

#ifndef BALOO_XAPIANDOCUMENT_H
#define BALOO_XAPIANDOCUMENT_H

#include <xapian.h>
#include <QString>

namespace Baloo {

/**
 * This class is just a light wrapper over Xapian::Document
 * which provides nice Qt apis.
 */
class XapianDocument
{
public:
    XapianDocument();

    void addTerm(const QString& term, const QString& prefix);
    void addBoolTerm(const QString& term, const QString& prefix = QString());
    void addBoolTerm(int term, const QString& prefix);

    void indexText(const QString& text, int wdfInc = 1);
    void indexText(const QString& text, const QString& prefix, int wdfInc = 1);

    Xapian::Document doc();

private:
    Xapian::Document m_doc;
    Xapian::TermGenerator m_termGen;
};
}

#endif // BALOO_XAPIANDOCUMENT_H