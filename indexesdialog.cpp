#include "indexesdialog.h"
#include "ui_indexesdialog.h"
#include "common.h"
#include "cl_common.h"
#include "indexinfo.h"
#include "booksdb.h"
#include "indexingdialg.h"
#include <qsettings.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qprogressdialog.h>
#include <qdir.h>
#include <assert.h>

#define PROGRESS_STEP(text)     progress.setValue(progress.value()+1); \
                                progress.setLabelText(trUtf8("جاري " text "..."));

IndexesDialog::IndexesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IndexesDialog)
{
    ui->setupUi(this);

    loadIndexesList();
}

IndexesDialog::~IndexesDialog()
{
    delete ui;
}

bool IndexesDialog::changeIndexName(IndexInfo *index, QString newName)
{
    QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
    QStringList list =  settings.value("indexes_list").toStringList();

    QString oldHash = INDEX_HASH(index->name());
    QString newHash = INDEX_HASH(newName);

    int oldIndex = list.indexOf(oldHash);
    if(oldIndex != -1) {

        settings.beginGroup(newHash);
        settings.setValue("name", newName);
        settings.setValue("shamela_path", index->shamelaPath());
        settings.setValue("index_path", index->path());
        settings.setValue("ram_size", index->ramSize());
        settings.setValue("optimizeIndex", index->optimize());
        settings.endGroup();

        list.replace(oldIndex, newHash);
        settings.setValue("indexes_list", list);

        settings.remove(oldHash);

        if(oldHash == settings.value("current_index").toString())
            settings.setValue("current_index", newHash);

        return true;
    }

    return false;
}

bool IndexesDialog::deleteIndex(IndexInfo *index)
{
    QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
    QStringList list =  settings.value("indexes_list").toStringList();
    QString indexHash = INDEX_HASH(index->name());

    int indexIndex = list.indexOf(indexHash);

    if(indexIndex != -1) {
        settings.remove(indexHash);
        list.removeAt(indexIndex);

        settings.setValue("indexes_list", list);

        if(indexHash == settings.value("current_index").toString())
            settings.setValue("current_index", list.first());

        return true;
    }

    return false;
}

void IndexesDialog::removeSameIds(QList<int> &big, QList<int> &small)
{
    int i=0;

    qDebug() << "small.count():" << small.count();
    qDebug() << "big.count():" << big.count();

    for(i=0; i < small.count(); i++) {
        int val = small.at(i);
        int index = big.indexOf(val);

        if(index != -1) {
            small.removeAt(i);
            big.removeAt(index);
            i--;
        }
    }

}

void IndexesDialog::loadIndexesList()
{
    ui->treeWidget->clear();

    QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
    QStringList list =  settings.value("indexes_list").toStringList();
    QList<QTreeWidgetItem*> itemList;
    bool haveIndexes = !list.isEmpty();

    if(haveIndexes) {
//        QString current = settings.value("current_index", list.first()).toString();

        for(int i=0; i<list.count(); i++) {
            IndexInfo *info = new IndexInfo();
            QString indexHash(list.at(i));

            settings.beginGroup(indexHash);
            info->setName(settings.value("name").toString());
            info->setShamelaPath(settings.value("shamela_path").toString());
            info->setPath(settings.value("index_path").toString());
            info->setRamSize(settings.value("ram_size").toInt());
            info->setOptimizeIndex(settings.value("optimizeIndex").toBool());
            settings.endGroup();

            m_indexInfoMap.insert(indexHash, info);

            QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
            item->setData(0, Qt::DisplayRole, info->name());
            item->setData(0, Qt::UserRole, indexHash);
/*
            if(current == indexHash) {
                item->setData(0,
                              Qt::ForegroundRole,
                              QBrush(Qt::green));
            }
*/
            itemList.append(item);
        }

        ui->treeWidget->addTopLevelItems(itemList);
    }
}

void IndexesDialog::on_pushEdit_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    if(items.count() > 0) {
        bool ok;
        IndexInfo *indexInfo = m_indexInfoMap[items.at(0)->data(0, Qt::UserRole).toString()];

        QString text = QInputDialog::getText(this,
                                             trUtf8("تغيير اسم الفهرس"),
                                             trUtf8("الاسم الجديد:"),
                                             QLineEdit::Normal,
                                             indexInfo->name(),
                                             &ok);
        if (ok && !text.isEmpty() && text != indexInfo->name()) {
            if(!m_indexInfoMap.keys().contains(INDEX_HASH(text))) {
                if(changeIndexName(indexInfo, text)) {
                    loadIndexesList();
                    emit indexesChanged();
                }
            } else {
                qDebug("Aleardy exist");
            }
        }

    } else {
        QMessageBox::warning(this,
                             trUtf8("تعديل فهرس"),
                             trUtf8("لم تقم باختيار اي فهرس!"));
    }
}

void IndexesDialog::on_pushDelete_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    if(items.count() > 0) {
        IndexInfo *indexInfo = m_indexInfoMap[items.at(0)->data(0, Qt::UserRole).toString()];
        int rep = QMessageBox::question(this,
                             trUtf8("حذف فهرس"),
                             trUtf8("هل تريد خذف فهرس <strong>%1</strong>؟").arg(indexInfo->name()),
                             QMessageBox::Yes|QMessageBox::No, QMessageBox::No);

        if(rep == QMessageBox::Yes) {
            if(deleteIndex(indexInfo)) {
                loadIndexesList();
                emit indexesChanged();
            }
        }
    } else {
        QMessageBox::warning(this,
                             trUtf8("حذف فهرس"),
                             trUtf8("لم تقم باختيار اي فهرس!"));
    }
}

void IndexesDialog::on_pushUpDate_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    int added = 0;
    int removed = 0;
    int progressMax = 7;

    if(items.count() > 0) {
        IndexInfo *indexInfo = m_indexInfoMap[items.at(0)->data(0, Qt::UserRole).toString()];

        BooksDB *bookDb = new BooksDB();
        bookDb->setIndexInfo(indexInfo);

        // Progress dialog
        QProgressDialog progress(trUtf8("جاري تحديث فهرس %1...").arg(indexInfo->name()),
                                 trUtf8("الغاء"),
                                 0,
                                 progressMax,
                                 this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setCancelButton(0);

        PROGRESS_STEP("التعرف على كتب الشاملة");

        QList<int> shaIds = bookDb->getShamelaIds();
        QList<int> savedIds = bookDb->getSavedIds();

        PROGRESS_STEP("مقارنة الكتب");

        if(shaIds.count() > savedIds.count())
            removeSameIds(shaIds, savedIds);
        else
            removeSameIds(savedIds, shaIds);

//        qDebug() << "SHAMELA:" << shaIds.count() << ":" << shaIds;
//        qDebug() << "SAVED:" << savedIds.count() << ":" << savedIds;

        if(shaIds.count() > 0) {
            PROGRESS_STEP("اضافة الكتب الجديدة الى قاعدة البيانات");
            added = bookDb->addBooks(shaIds);
        }

        if(savedIds.count() > 0) {
            PROGRESS_STEP("حذف الكتب من قاعدة البيانات");
            removed = bookDb->removeBooks(savedIds);
        }

        QString msg(trUtf8("ثم تحديث الفهرس بنجاح." "<br>"));
        if(added > 0) {
            PROGRESS_STEP("فهرسة الكتب الجديدة");

            msg.append(trUtf8("ثم اضافة %1." "<br>").arg(IndexingDialg::arPlural(added, 4)));
            indexBooks(shaIds, bookDb, indexInfo);
        }

        if(removed > 0) {
            PROGRESS_STEP("حذف الكتب من الفهرس");

            msg.append(trUtf8("ثم حذف %1." "<br>").arg(IndexingDialg::arPlural(removed, 4)));
            deletBooksFromIndex(savedIds, indexInfo);
        }

        if(added <= 0 && removed <= 0)
            msg.append(trUtf8("لم يتم اضافة او حذف اي كتاب."));

        progress.setValue(progressMax);

        QMessageBox::information(this,
                                 trUtf8("تحديث فهرس"),
                                 msg);

        delete bookDb;

        DEL_DB("indexDb");
        DEL_DB("shamelaBookDb");

    } else {
        QMessageBox::warning(this,
                             trUtf8("تحديث فهرس"),
                             trUtf8("لم تقم باختيار اي فهرس!"));
    }
}

void IndexesDialog::on_pushOptimize_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    if(items.count() > 0) {
        IndexInfo *indexInfo = m_indexInfoMap[items.at(0)->data(0, Qt::UserRole).toString()];
        int rep = QMessageBox::question(this,
                             trUtf8("ضغط فهرس"),
                             trUtf8("هل تريد ضغط فهرس <strong>%1</strong>؟"
                                    "<br>"
                                    "هذه العملية قد تأخذ بعض الوقت وقد يتجمد البرنامج قليلا.").arg(indexInfo->name()),
                             QMessageBox::Yes|QMessageBox::No, QMessageBox::No);

        if(rep == QMessageBox::Yes){
            QProgressDialog progress(trUtf8("جاري ضغط فهرس %1...").arg(indexInfo->name()),
                                     trUtf8("الغاء"),
                                     0,
                                     1,
                                     this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setCancelButton(0);
            progress.setMinimumDuration(0);

            optimizeIndex(indexInfo);

            progress.setValue(1);
        }

    } else {
        QMessageBox::warning(this,
                             trUtf8("ضغط فهرس"),
                             trUtf8("لم تقم باختيار اي فهرس!"));
    }
}

void IndexesDialog::indexBooks(QList<int> ids, BooksDB *bookDB, IndexInfo *info)
{
    IndexWriter *writer = NULL;
    QDir dir;
    ArabicAnalyzer *analyzer = new ArabicAnalyzer();
    if(!dir.exists(qPrintable(info->path())))
        dir.mkdir(qPrintable(info->path()));
    if ( IndexReader::indexExists(qPrintable(info->path())) ){
        if ( IndexReader::isLocked(qPrintable(info->path())) ){
            printf("Index was locked... unlocking it.\n");
            IndexReader::unlock(qPrintable(info->path()));
        }

        writer = _CLNEW IndexWriter( qPrintable(info->path()), analyzer, false);
    } else {
        QMessageBox::critical(this,
                              trUtf8("خطأ عند التحديث"),
                              trUtf8("لم يتم العثور على اي فهرس في المسار" "\n" "%1").arg(info->path()));
        return;
    }

    writer->setMaxFieldLength(IndexWriter::DEFAULT_MAX_FIELD_LENGTH);
    writer->setRAMBufferSizeMB(info->ramSize());

    bookDB->queryBooksToIndex(ids);


    IndexingThread *indexThread = new IndexingThread();
    indexThread->setIndexInfo(info);
    indexThread->setBookDB(bookDB);
    indexThread->setWirter(writer);

    indexThread->run();

//    writer->optimize();
    writer->close();

    _CLDELETE(writer);
    _CLDELETE(indexThread);
}

void IndexesDialog::deletBooksFromIndex(QList<int> ids, IndexInfo *info)
{
    IndexWriter *writer = NULL;
    QDir dir;
    ArabicAnalyzer *analyzer = new ArabicAnalyzer();
    if(!dir.exists(qPrintable(info->path())))
        dir.mkdir(qPrintable(info->path()));
    if ( IndexReader::indexExists(qPrintable(info->path())) ){
        if ( IndexReader::isLocked(qPrintable(info->path())) ){
            printf("Index was locked... unlocking it.\n");
            IndexReader::unlock(qPrintable(info->path()));
        }

        writer = _CLNEW IndexWriter( qPrintable(info->path()), analyzer, false);
    } else {
        QMessageBox::critical(this,
                              trUtf8("خطأ عند التحديث"),
                              trUtf8("لم يتم العثور على اي فهرس في المسار" "\n" "%1").arg(info->path()));
        return;
    }

    RefCountArray<Term*> terms(ids.count());
    for(int i=0; i<ids.count(); i++) {
        Term *term = new Term(QSTRING_TO_TCHAR(QString::number(ids.at(i))), _T("bookid"));
        terms[i] = term;
    }

    writer->deleteDocuments(&terms);

//    writer->optimize();
    writer->close();

    _CLDELETE(writer);
//    _CLDECDELETE(term);
}

void IndexesDialog::optimizeIndex(IndexInfo *info)
{
    IndexWriter *writer = NULL;
    QDir dir;
    ArabicAnalyzer *analyzer = new ArabicAnalyzer();
    if(!dir.exists(qPrintable(info->path())))
        dir.mkdir(qPrintable(info->path()));
    if ( IndexReader::indexExists(qPrintable(info->path())) ){
        if ( IndexReader::isLocked(qPrintable(info->path())) ){
            printf("Index was locked... unlocking it.\n");
            IndexReader::unlock(qPrintable(info->path()));
        }

        writer = _CLNEW IndexWriter( qPrintable(info->path()), analyzer, false);
    } else {
        QMessageBox::critical(this,
                              trUtf8("خطأ عند التحديث"),
                              trUtf8("لم يتم العثور على اي فهرس في المسار" "\n" "%1").arg(info->path()));
        return;
    }

    writer->optimize();
    writer->close();

    _CLDELETE(writer);
}
