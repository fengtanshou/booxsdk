// Author: John

#include "onyx/cms/download_db.h"
#include "onyx/cms/cms_utils.h"

namespace cms
{
static const QString TAG_URL = "url";
static const QString TAG_PATH = "path";
static const QString TAG_SIZE = "size";
static const QString TAG_STATE = "state";
static const QString TAG_TIMESTAMP = "timestamp";

DownloadItemInfo::DownloadItemInfo(const QVariantMap & vm)
    : OData(vm)
{
    setState(STATE_INVALID);
    setTimeStamp(QDateTime::currentDateTime().toString(dateFormat()));
}

DownloadItemInfo::~DownloadItemInfo()
{
}

bool DownloadItemInfo::operator == (const DownloadItemInfo &right)
{
    return url() == right.url();
}

QString DownloadItemInfo::url() const
{
    return value(TAG_URL).toString();
}

void DownloadItemInfo::setUrl(const QString & url)
{
    insert(TAG_URL, url);
}
QString DownloadItemInfo::path() const
{
    return value(TAG_PATH).toString();
}

void DownloadItemInfo::setPath(const QString & path)
{
    insert(TAG_PATH, path);
}

int DownloadItemInfo::size() const
{
    return value(TAG_SIZE).toInt();
}

void DownloadItemInfo::setSize(int size)
{
    insert(TAG_SIZE, size);
}

DownloadState DownloadItemInfo::state() const
{
    return static_cast<DownloadState>(value(TAG_STATE).toInt());
}

void DownloadItemInfo::setState(DownloadState state)
{
    insert(TAG_STATE, state);
}

QString DownloadItemInfo::timeStamp() const
{
    return value(TAG_TIMESTAMP).toString();
}

void DownloadItemInfo::setTimeStamp(const QString & timeStamp)
{
    insert(TAG_TIMESTAMP, timeStamp);
}

DownloadDB::DownloadDB(const QString & db_name)
    : database_name_(db_name) 
{
    open();
}

DownloadDB::~DownloadDB()
{
    close();
}

bool DownloadDB::open()
{
    if (!database_)
    {
        database_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", database_name_)));
    }

    if (!database_->isOpen())
    {
        QDir home = QDir::home();
        database_->setDatabaseName(home.filePath(database_name_));
        if (!database_->open())
        {
            qDebug() << database_->lastError().text();
            return false;
        }
        makeSureTableExist(*database_);
    }
    return true;
}

bool DownloadDB::close()
{
    if (database_)
    {
        database_->close();
        database_.reset(0);
        QSqlDatabase::removeDatabase(database_name_);
        return true;
    }
    return false;
}

/// Return all download item list including pending list, finished list and the others.
DownloadInfoList DownloadDB::list()
{
    return pendingList(DownloadInfoList(), true, true);
}

DownloadInfoList DownloadDB::pendingList(const DownloadInfoList & input,
                                     bool force_all,
                                     bool sort)
{
    DownloadInfoList list;

    // Fetch all download items.
    QSqlQuery query(db());
    query.prepare( "select url, value from download ");
    if (!query.exec())
    {
        return list;
    }

    while (query.next())
    {
        DownloadItemInfo item(query.value(1).toMap());

        // Ignore items finished.
        if (item.state() != FINISHED || force_all)
        {
            if (!list.contains(item))
            {
                list.push_back(item);
            }
        }
    }

    // check input now.
    foreach(DownloadItemInfo i, input)
    {
        if (!list.contains(i))
        {
            list.push_back(i);
        }
    }

    if (sort)
    {
        qSort(list.begin(), list.end(), GreaterByTimestamp());
    }
    return list;
}

bool DownloadDB::update(const DownloadItemInfo & item)
{
    QSqlQuery query(db());
    query.prepare( "INSERT OR REPLACE into download (url, value) values(?, ?)");
    query.addBindValue(item.url());
    query.addBindValue(item);
    return query.exec();
}

bool DownloadDB::updateState(const QString & myUrl, DownloadState state)
{
    // find the download item info first.

    QSqlQuery query(db());
    query.prepare( "select url, value from download ");
    if (!query.exec())
    {
        return false;
    }

    while (query.next())
    {
        if (query.value(0).toString() == myUrl)
        {
            DownloadItemInfo item(query.value(1).toMap());
            item.setUrl(myUrl);
            item.setState(state);
            return update(item);
        }
    }

    return false;
}

bool DownloadDB::makeSureTableExist(QSqlDatabase &db)
{
    QSqlQuery query(db);
    query.exec("create table if not exists download ("
               "url text primary key,"
               "value blob) ");
    query.exec("create index if not exists url_index on info (url)");
    return true;
}

QSqlDatabase & DownloadDB::db()
{
    return *database_;
}


}  // namespace cms


