#include "shamelasearcher.h"
#include "arabicanalyzer.h"
#include "common.h"
#include "cl_common.h"
#include "indexinfo.h"
#include "shamelaresult.h"
#include <qdatetime.h>
#include <qstringlist.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qdebug.h>
#include <qvariant.h>

ShamelaSearcher::ShamelaSearcher(QObject *parent) : QThread(parent)
{
    m_hits = NULL;
    m_query = NULL;
    m_searcher = NULL;
    m_currentPage = 0;
    m_pageCount = 0;
    m_action = SEARCH;
    m_defautOpIsAnd = false;
    m_resultParPage = 10;
    m_timeSearch = 0;

    m_colors.append("#FFFF63");
    m_colors.append("#A5FFFF");
    m_colors.append("#FF9A9C");
    m_colors.append("#9CFF9C");
    m_colors.append("#EF86FB");
}

ShamelaSearcher::~ShamelaSearcher()
{
    if(m_hits != NULL)
        delete m_hits;

    if(m_query != NULL)
        delete m_query;

    if(m_searcher != NULL) {
        m_searcher->close();
        delete m_searcher;
    }

    qDeleteAll(m_resultsHash);
    m_resultsHash.clear();
}

void ShamelaSearcher::run()
{
    try {
        if(m_action == SEARCH){
            search();
            fetech();
        } else if (m_action == FETECH) {
            fetech();
        }
    } catch(CLuceneError &e) {
        qDebug() << "Error when searching:" << e.what() << "\ncode:" << e.number();
        emit gotException(e.what(), e.number());
    } catch(...) {
        qDebug() << "Error when searching at : " << m_indexInfo->path();
        gotException("UNKNOW", -1);
    }

}

void ShamelaSearcher::search()
{
    emit startSearching();

    qDeleteAll(m_resultsHash);
    m_resultsHash.clear();

    m_searcher = new IndexSearcher(qPrintable(m_indexInfo->path()));

    qDebug() << "Query: " << TCHAR_TO_QSTRING(m_query->toString(_T("text")));

    QTime time;
    time.start();
    m_hits = m_searcher->search(m_query);
    m_timeSearch = time.elapsed();

    m_pageCount = _ceil((resultsCount()/(double)m_resultParPage));
    m_currentPage = 0;

    emit doneSearching();
}

void ShamelaSearcher::fetech()
{
    emit startFeteching();

    ArabicAnalyzer hl_analyzer;
    QueryScorer scorer(m_query->rewrite(IndexReader::open(qPrintable(m_indexInfo->path()))));
    SimpleCssFormatter hl_formatter;
    int maxNumFragmentsRequired = 30;
    const TCHAR* fragmentSeparator = _T("...");

    int start = m_currentPage * m_resultParPage;
    int maxResult  =  (resultsCount() >= start+m_resultParPage)
                      ? (start+m_resultParPage) : resultsCount();
    int entryID;
    bool whiteBG = false;
    for(int i=start; i < maxResult;i++){
        //Document doc = m_hits->doc(i);
        entryID = idAt(i);
        int bookID = bookIdAt(i);
        int archive = ArchiveAt(i);
        int score = (int) (scoreAt(i) * 100.0);

        ShamelaResult *savedResult = m_resultsHash.value(entryID, 0);
        if(savedResult != 0
           && archive == savedResult->archive()
           && bookID == savedResult->bookId()) {
            emit gotResult(savedResult);
            continue;
        }

        QString connName = (archive) ? QString("bid_%1").arg(archive) : QString("bid_%1_%2").arg(archive).arg(bookID);

        {
            QSqlDatabase bookDB;
            if(archive && QSqlDatabase::contains(connName)) {
                bookDB = QSqlDatabase::database(connName);
            } else {
                bookDB = QSqlDatabase::addDatabase("QODBC", connName);
                QString mdbpath = QString("DRIVER={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=%1")
                                  .arg(buildFilePath(QString::number(bookID), archive));
                bookDB.setDatabaseName(mdbpath);
            }

            if (!bookDB.open())
                qDebug() << "Cannot open" << buildFilePath(QString::number(bookID), archive) << "database.";

            QSqlQuery bookQuery(bookDB);

            bookQuery.exec(QString("SELECT nass, page, part FROM %1 WHERE id = %2")
                             .arg((!archive) ? "book" : QString("b%1").arg(bookID))
                             .arg(entryID));
            if(bookQuery.first()){
                ShamelaResult *result = new ShamelaResult;
                result->setBookId(bookID);
                result->setArchive(archive);
                result->setId(entryID);
                result->setPage(bookQuery.value(1).toInt());
                result->setPart(bookQuery.value(2).toInt());
                result->setScore(score);
                result->setText(bookQuery.value(0).toString());
                result->setTitle(getTitleId(bookDB, result));
                result->setBgColor((whiteBG = !whiteBG) ? "whiteBG" : "grayBG");
                //result->setSnippet(hiText(abbreviate(result->text(), 320), m_queryStr));
                /**/
                Highlighter highlighter(&hl_formatter, &scorer);
                SimpleFragmenter frag(20);
                highlighter.setTextFragmenter(&frag);

                const TCHAR* text = QSTRING_TO_TCHAR(bookQuery.value(0).toString());
                StringReader reader(text);
                TokenStream* tokenStream = hl_analyzer.tokenStream(_T("text"), &reader);

                TCHAR* hi_result = highlighter.getBestFragments(
                        tokenStream,
                        text,
                        maxNumFragmentsRequired,
                        fragmentSeparator);

                result->setSnippet(TCHAR_TO_QSTRING(hi_result));

                _CLDELETE_CARRAY(hi_result)
                _CLDELETE(tokenStream)
                _CLDELETE(text)
                /**/

                        emit gotResult(result);
                m_resultsHash.insert(entryID, result);
            }
        }
        if(!archive)
            QSqlDatabase::removeDatabase(connName);
    }

    emit doneFeteching();
}

void ShamelaSearcher::clear()
{
    if(m_hits != NULL)
        _CLDELETE(m_hits)

    if(m_query != NULL)
        _CLDELETE(m_query)

    if(m_searcher != NULL) {
        m_searcher->close();
        _CLDELETE(m_searcher)
    }

    qDeleteAll(m_resultsHash);
    m_resultsHash.clear();

    m_currentPage = 0;
    m_pageCount = 0;
    m_timeSearch = 0;
}

int ShamelaSearcher::idAt(int index)
{
    return FIELD_TO_INT("id", (&m_hits->doc(index)));
}

int ShamelaSearcher::bookIdAt(int index)
{
    return FIELD_TO_INT("bookid", (&m_hits->doc(index)));
}

int ShamelaSearcher::ArchiveAt(int index)
{
    return FIELD_TO_INT("archive", (&m_hits->doc(index)));
}

float_t ShamelaSearcher::scoreAt(int index)
{
    return m_hits->score(index);
}

int ShamelaSearcher::pageCount()
{
    return m_pageCount;
}

int ShamelaSearcher::currentPage()
{
    return m_currentPage;
}

void ShamelaSearcher::setIndexInfo(IndexInfo *index)
{
    m_indexInfo = index;
}

void ShamelaSearcher::setsetDefaultOperator(bool defautIsAnd)
{
    m_defautOpIsAnd = defautIsAnd;
}

void ShamelaSearcher::setPageCount(int pageCount)
{
    m_pageCount = pageCount;
}

void ShamelaSearcher::setCurrentPage(int page)
{
    m_currentPage = page;
}

void ShamelaSearcher::setHits(Hits *hit)
{
    m_hits = hit;
}

void ShamelaSearcher::setQuery(Query* q)
{
    m_query = q;
}

void ShamelaSearcher::setQueryString(QString q)
{
    m_queryStr = q;
}

void ShamelaSearcher::setSearcher(IndexSearcher *searcher)
{
    m_searcher = searcher;
}

int ShamelaSearcher::resultsCount()
{
    return m_hits->length();
}

QString ShamelaSearcher::buildFilePath(QString bkid, int archive)
{
    if(!archive)
        return QString("%1/Books/%2/%3.mdb").arg(m_indexInfo->shamelaPath()).arg(bkid.right(1)).arg(bkid);
    else
        return QString("%1/Books/Archive/%2.mdb").arg(m_indexInfo->shamelaPath()).arg(archive);
}

QStringList ShamelaSearcher::buildRegExp(const QString &str)
{
    QString text = str;
    text.remove(QRegExp(trUtf8("[\\x064B-\\x0652\\x0600\\x061B-\\x0620،]")));

    QStringList strWords = text.split(QRegExp(trUtf8("[\\s;,.()\"'{}\\[\\]]")), QString::SkipEmptyParts);
    QStringList regExpList;
    QChar opPar('(');
    QChar clPar(')');
    foreach(QString word, strWords)
    {
        QString regExpStr;
        regExpStr.append("\\b");
        regExpStr.append(opPar);

        for (int i=0; i< word.size();i++) {
            if(word.at(i) == QChar('~'))
                regExpStr.append("[\\S]*");
            else if(word.at(i) == QChar('*'))
                regExpStr.append("[\\S]*");
            else if(word.at(i) == QChar('?'))
                regExpStr.append("\\S");
            else if( word.at(i) == QChar('"') || word.at(i) == opPar || word.at(i) == opPar )
                continue;
            else {
                regExpStr.append(word.at(i));
                regExpStr.append(trUtf8("[ًٌٍَُِّْ]*"));
            }
        }

        regExpStr.append(clPar);
        regExpStr.append("\\b");
        regExpList.append(regExpStr);
    }

    return regExpList;
}

QString ShamelaSearcher::abbreviate(QString str, int size) {
        if (str.length() <= size-3)
                return str;
        str.simplified();
        int index = str.lastIndexOf(' ', size-3);
        if (index <= -1)
                return "";
        return str.left(index).append("...");
}

QString ShamelaSearcher::cleanString(QString str)
{
    str.replace(QRegExp("[\\x0627\\x0622\\x0623\\x0625]"), "[\\x0627\\x0622\\x0623\\x0625]");//ALEFs
    str.replace(QRegExp("[\\x0647\\x0629]"), "[\\x0647\\x0629]"); //TAH_MARBUTA -> HEH

    return str;
}

QString ShamelaSearcher::hiText(const QString &text, const QString &strToHi)
{
    QStringList regExpStr = buildRegExp(strToHi);
    QString finlStr  = text;
    int color = 0;
    bool useColors = (regExpStr.size() <= m_colors.size());

    foreach(QString regExp, regExpStr)
        finlStr.replace(QRegExp(cleanString(regExp)),
                        QString("<b style=\"background-color:%1\">\\1</b>")
                        .arg(m_colors.at(useColors ? color++ : color)));

//    if(!useColors)
//        finlStr.replace(QRegExp("<\\/b>([\\s])<b style=\"background-color:[^\"]+\">"), "\\1");

    return finlStr;
}

QString ShamelaSearcher::getTitleId(const QSqlDatabase &db, ShamelaResult *result)
{
    QSqlQuery m_bookQuery(db);
    m_bookQuery.exec(QString("SELECT TOP 1 tit FROM %1 WHERE id <= %2 ORDER BY id DESC")
                     .arg((!result->archive()) ? "title" : QString("t%1").arg(result->bookId()))
                     .arg(result->id()));

    if(m_bookQuery.first())
        return m_bookQuery.value(0).toString();
    else {
        qDebug() << m_bookQuery.lastError().text();
        return QString();
    }

}

void ShamelaSearcher::nextPage()
{
    if(!isRunning()) {
        if(m_currentPage+1 < pageCount()) {
            m_currentPage++;
            m_action = FETECH;

            start();
        }
    }
}

void ShamelaSearcher::prevPage()
{
    if(!isRunning()) {
        if(m_currentPage-1 >= 0) {
            m_currentPage--;
            m_action = FETECH;

            start();
        }
    }
}

void ShamelaSearcher::firstPage()
{
    if(!isRunning()) {
        m_currentPage=0;
        m_action = FETECH;

        start();
    }
}

void ShamelaSearcher::lastPage()
{
    if(!isRunning()) {
        m_currentPage = pageCount()-1;
        m_action = FETECH;

        start();
    }
}

void ShamelaSearcher::fetechResults(int page)
{
    if(!isRunning()) {
        if(page < pageCount()) {
            m_currentPage = page;
            m_action = FETECH;

            start();
        }
    }
}
