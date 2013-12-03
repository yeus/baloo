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

#include "filesearchstoretest.h"
#include "filesearchstore.h"
#include "../../database.h"
#include "filemapping.h"
#include "query.h"
#include "term.h"

#include "qtest_kde.h"
#include <KDebug>

#include <xapian.h>

#include <QTest>

using namespace Baloo;

FileSearchStoreTest::FileSearchStoreTest(QObject* parent)
    : QObject(parent)
    , m_tempDir(0)
    , m_db(0)
    , m_store(0)
{
}

void FileSearchStoreTest::init()
{
    cleanupTestCase();

    m_db = new Database(this);
    m_tempDir = new KTempDir();
    m_db->setPath(m_tempDir->name());
    m_db->init();

    m_store = new FileSearchStore(this, QVariantList());
    m_store->setDbPath(m_tempDir->name());
}

void FileSearchStoreTest::initTestCase()
{
}

void FileSearchStoreTest::cleanupTestCase()
{
    delete m_store;
    m_store = 0;

    delete m_db;
    m_db = 0;

    delete m_tempDir;
    m_tempDir = 0;
}

uint FileSearchStoreTest::insertUrl(const QString& url)
{
    FileMapping file(url);
    file.create(m_db->sqlDatabase());

    return file.id();
}

void FileSearchStoreTest::insertText(int id, const QString& text)
{
    Xapian::Document doc;

    Xapian::TermGenerator termGen;
    termGen.set_document(doc);
    termGen.index_text(text.toStdString());

    QScopedPointer<Xapian::WritableDatabase> wdb(new Xapian::WritableDatabase(m_tempDir->name().toStdString(),
                                                                              Xapian::DB_CREATE_OR_OPEN));
    wdb->replace_document(id, doc);
    wdb->commit();

    m_db->xapainDatabase()->reopen();
}

void FileSearchStoreTest::testSimpleSearchString()
{
    QString url1("/home/t/a");
    uint id1 = insertUrl(url1);
    insertText(id1, "This is sample text");

    QString url2("/home/t/b");
    uint id2 = insertUrl(url2);
    insertText(id2, "sample sample more sample text");

    Query q;
    q.addType("File");
    q.setSearchString("Sample");

    int qid = m_store->exec(q);
    QCOMPARE(qid, 1);
    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id2));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url2));

    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id1));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url1));

    QVERIFY(!m_store->next(qid));
    QVERIFY(m_store->id(qid).isEmpty());
    QVERIFY(m_store->url(qid).isEmpty());

    m_store->close(qid);
}

QTEST_KDEMAIN_CORE(Baloo::FileSearchStoreTest)
