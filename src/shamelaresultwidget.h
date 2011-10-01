#ifndef SHAMELARESULTWIDGET_H
#define SHAMELARESULTWIDGET_H

#include <qwidget.h>
#include <qsqldatabase.h>
#include <qhash.h>
#include <qurl.h>

namespace Ui {
    class ShamelaResultWidget;
}

class ShamelaSearcher;
class ShamelaBooksReader;
class WebView;
class IndexInfo;
class BooksDB;
class ShamelaResult;

class ShamelaResultWidget : public QWidget
{
    Q_OBJECT

public:
    ShamelaResultWidget(QWidget *parent = 0);
    ~ShamelaResultWidget();
    void setShamelaSearch(ShamelaSearcher *s);
    void setIndexInfo(IndexInfo *info) { m_indexInfo = info; }
    void setBooksDb(BooksDB *db) {m_booksDb = db; }
    void doSearch();
    void clearResults();

public slots:
    void openResult(int bookID, int resultID);
    QString baseUrl();
    void showNavigationButton(bool show);

protected slots:
    void searchStarted();
    void searchFinnished();
    void fetechStarted();
    void fetechFinnished();
    void gotResult(ShamelaResult *result);
    void gotException(QString what, int id);
    void populateJavaScriptWindowObject();

protected:
    void setPageCount(int current, int count);
    void buttonStat(int currentPage, int pageCount);

protected:
    WebView *m_webView;
    ShamelaSearcher *m_searcher;
    ShamelaBooksReader *m_bookReader;
    IndexInfo *m_indexInfo;
    BooksDB *m_booksDb;
    Ui::ShamelaResultWidget *ui;

private slots:
    void writeHtmlResult();
    void on_buttonGoFirst_clicked();
    void on_buttonGoLast_clicked();
    void on_buttonGoPrev_clicked();
    void on_buttonGoNext_clicked();
};

#endif // SHAMELARESULTWIDGET_H