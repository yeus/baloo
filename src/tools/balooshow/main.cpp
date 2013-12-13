/*
   Copyright (c) 2010 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2012 Vishesh Handa <me@vhanda.in>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QCoreApplication>
#include <QHash>
#include <QUrl>

#include <KCmdLineArgs>
#include <KAboutData>
#include <KLocale>
#include <KComponentData>
#include <KDebug>
#include <KUrl>

#include "file.h"
#include "filefetchjob.h"

int main(int argc, char* argv[])
{
    KAboutData aboutData("balooshow",
                         "balooshow",
                         ki18n("Baloo Show"),
                         "0.1",
                         ki18n("The Baloo data Viewer - A debugging tool"),
                         KAboutData::License_GPL,
                         ki18n("(c) 2012, Vishesh Handa"),
                         KLocalizedString(),
                         "http://kde.org");
    aboutData.addAuthor(ki18n("Vishesh Handa"), ki18n("Maintainer"), "me@vhanda.in");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+resource", ki18n("The file URL"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    QCoreApplication app(argc, argv);
    KComponentData comp(aboutData);

    if (args->count() == 0)
        KCmdLineArgs::usage();

    QTextStream err(stdout);

    //
    // The Resource Uri
    //
    const QUrl url = args->url(0);
    if (!url.isLocalFile()) {
        err << "Not a local file" << args->url(0).url() << endl;
        return 1;
    }

    QTextStream stream(stdout);
    Baloo::FileFetchJob* job = new Baloo::FileFetchJob(url.toLocalFile());
    job->exec();

    QVariantMap properties = job->file().properties();
    QMapIterator<QString, QVariant> it(properties);
    while (it.hasNext()) {
        it.next();
        stream << it.key() << ": " << it.value().toString() << endl;
    }

    return 0;
}