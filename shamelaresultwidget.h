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
class WebView;
class IndexInfo;
class ShamelaResult;

class ShamelaResultWidget : public QWidget
{
    Q_OBJECT

public:
    ShamelaResultWidget(QWidget *parent = 0);
    ~ShamelaResultWidget();
    void setShamelaSearch(ShamelaSearcher *s);
    void setIndexInfo(IndexInfo *info) { m_indexInfo = info; }
    void doSearch();
    void clearResults();

public slots:
    QString getPage(QString href);
    QString currentBookName();
    QString currentPage() { return QString::number(m_currentPage); }
    QString currentPart() { return QString::number(m_currentPart); }
    QString baseUrl();
    void updateNavgitionLinks(QString href);
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
    QString buildFilePath(QString bkid, int archive);
    QString hiText(const QString &text, const QString &strToHi);
    QStringList buildRegExp(const QString &str);
    QString abbreviate(QString str, int size);
    QString cleanString(QString str);
    QString getTitleId(const QSqlDatabase &db, int pageID, int archive, int bookID);
    QString getBookName(int bookID);
    QString formNextUrl(QString href);
    QString formPrevUrl(QString href);
    void setPageCount(int current, int count);
    void buttonStat(int currentPage, int pageCount);

protected:
    WebView *m_webView;
    ShamelaSearcher *m_searcher;
    IndexInfo *m_indexInfo;
    QList<QString> m_colors;
    QHash<int, QString> m_booksName;
    int m_currentShownId;
    int m_currentBookId;
    int m_currentPage;
    int m_currentPart;
    Ui::ShamelaResultWidget *ui;

private slots:
    void writeHtmlResult();
    void on_buttonGoFirst_clicked();
    void on_buttonGoLast_clicked();
    void on_buttonGoPrev_clicked();
    void on_buttonGoNext_clicked();
};

#endif // SHAMELARESULTWIDGET_H
