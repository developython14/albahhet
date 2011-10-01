#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <qstring.h>
#include <tchar.h>
#include <qdebug.h>

class IndexInfo;

class BookInfo
{
public:
    BookInfo();
    BookInfo(int bid, QString bname, QString bpath, int barchive);
    ~BookInfo();

    int id() { return m_id; }
    int archive() { return m_archive; }
    int cat() { return m_cat; }
    int authorID() { return m_authorID; }
    QString name() { return m_name; }
    QString path() { return m_path; }
    QString mainTable() { return m_mainTable; }
    QString tocTable() { return m_tocTable; }

    void genInfo();
    void genInfo(IndexInfo *info);

    TCHAR *idT() { return m_idT; }
    TCHAR *authorDeathT() { return m_authorDeathT; }

    void setId(int id);
    void setArchive(int archive);
    void setCat(int cat);
    void setAuthorID(int id);
    void setName(const QString &name);
    void setPath(const QString &path);
    void setAuthorDeath(int dYear);

protected:
    void init();

protected:
    int m_id;
    QString m_name;
    QString m_path;
    QString m_mainTable;
    QString m_tocTable;
    int m_archive;
    int m_cat;
    int m_authorID;
    int m_authorDeath;
    TCHAR *m_idT;
    TCHAR *m_authorDeathT;
};

#endif // BOOKINFO_H