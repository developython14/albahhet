#ifndef SHAMELASEARCHWIDGET_H
#define SHAMELASEARCHWIDGET_H

#include "abstractsearchwidget.h"
#include "arabicanalyzer.h"
#include <QModelIndex>
#include <QStandardItem>

namespace Ui {
    class ShamelaSearchWidget;
}

class IndexInfoBase;
class ShamelaIndexInfo;
class BooksDB;
class ShamelaModels;
class SearchFilterHandler;
class TabWidget;
class QNetworkAccessManager;

class ShamelaSearchWidget : public AbstractSearchWidget
{
    Q_OBJECT

public:
    ShamelaSearchWidget(QWidget *parent = 0);
    ~ShamelaSearchWidget();
    void setIndexInfo(IndexInfoBase *info);
    void setTabWidget(TabWidget *tabWidget);
    void saveSettings();
    void indexChanged();
    QList<int> selectedBooks();
    void selectBooks(QList<int> books);

protected:
    void closeEvent(QCloseEvent *e);
    Query *getBooksListQuery();

public slots:
    void search();
    void loadSettings();
    void clearSpecialChar();
    void clearLineText();

    void selectAllBooks();
    void unSelectAllBooks();
    void selectVisibleBooks();
    void unSelectVisibleBooks();
    void expandFilterView();
    void collapseFilterView();

protected slots:
    void on_lineFilter_textChanged(QString text);
    void sortChanged(int logicalIndex, Qt::SortOrder);
    void itemChanged(QStandardItem *item);

protected:
    Ui::ShamelaSearchWidget *ui;
    ShamelaIndexInfo *m_currentIndex;
    BooksDB *m_booksDB;
    TabWidget *m_tabWidget;
    ShamelaModels *m_shaModel;
    SearchFilterHandler *m_filterHandler;
    QString m_searchQuery;
    QNetworkAccessManager *m_nam;
    int m_resultParPage;
    int m_searchCount;
    bool m_useMultiTab;
    bool m_proccessItemChange;
};

#endif // SHAMELASEARCHWIDGET_H
