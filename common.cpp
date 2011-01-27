#include "common.h"
#include <qstringlist.h>

TCHAR* QStringToTChar(const QString &str)
{
    TCHAR *string = new TCHAR[(str.length() +1) * sizeof(TCHAR)];
    memset(string, 0, (str.length() +1) * sizeof(TCHAR));
    str.toWCharArray(string);

    return string;
}

QString TCharToQString(const TCHAR *string)
{
    QString retValue = QString::fromWCharArray(string);
    return retValue;
}

QString arPlural(int count, PULRAL word, bool html)
{
    QStringList list;
    QString str;

    if(word == SECOND)
        list <<  QObject::trUtf8("ثانية")
        << QObject::trUtf8("ثانيتين")
        << QObject::trUtf8("ثوان")
        << QObject::trUtf8("ثانية");
    else if(word == MINUTE)
        list <<  QObject::trUtf8("دقيقة")
        << QObject::trUtf8("دقيقتين")
        << QObject::trUtf8("دقائق")
        << QObject::trUtf8("دقيقة");
    else if(word == HOUR)
        list <<  QObject::trUtf8("ساعة")
        << QObject::trUtf8("ساعتين")
        << QObject::trUtf8("ساعات")
        << QObject::trUtf8("ساعة");
    else if(word == BOOK)
        list <<  QObject::trUtf8("كتاب واحد")
        << QObject::trUtf8("كتابين")
        << QObject::trUtf8("كتب")
        << QObject::trUtf8("كتابا");

    if(count <= 1)
        str = list.at(0);
    else if(count == 2)
        str = list.at(1);
    else if (count > 2 && count <= 10)
        str = QString("%1 %2").arg(count).arg(list.at(2));
    else if (count > 10)
        str = QString("%1 %2").arg(count).arg(list.at(3));
    else
        str = QString();

    return html ? QString("<strong>%1</strong>").arg(str) : str;
}
