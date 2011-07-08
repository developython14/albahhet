#ifndef SHAMELAINDEXERWIDGET_H
#define SHAMELAINDEXERWIDGET_H

#include "abstractindexingwidget.h"

namespace Ui {
    class ShamelaIndexerWidget;
}
class IndexInfo;

class ShamelaIndexerWidget : public AbstractIndexingWidget
{
    Q_OBJECT

public:
    ShamelaIndexerWidget(QWidget *parent = 0);
    ~ShamelaIndexerWidget();
    QString indexTypeName();

protected:
    void readPaths();
    void checkIndex();
    void showBooks();
    void startIndexing();
    void stopIndexing();
    void saveIndexInfo(int indexingTime=0, int opimizingTime=0);

public slots:
    void nextStep();
    void addBook(const QString &);
    void doneIndexing();
    void indexingError();

private slots:
    void on_buttonSelectIndexPath_clicked();
    void on_buttonSelectShamela_clicked();
    void on_label_2_linkActivated(QString);

private:
    IndexInfo *m_indexInfo;
    Ui::ShamelaIndexerWidget *ui;
    int m_booksCount;
    int m_indexedBooks;
    int m_threadCount;
};

#endif // SHAMELAINDEXERWIDGET_H
