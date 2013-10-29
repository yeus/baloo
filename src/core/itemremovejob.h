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

#ifndef ITEMREMOVEJOB_H
#define ITEMREMOVEJOB_H

#include "core_export.h"

#include <KJob>

namespace Baloo {

class Item;

class BALOO_CORE_EXPORT ItemRemoveJob : public KJob
{
    Q_OBJECT
public:
    explicit ItemRemoveJob(QObject* parent = 0);
    virtual ~ItemRemoveJob();

signals:
    void itemRemoved(const Baloo::Item& item);
};

}

#endif // ITEMREMOVEJOB_H
