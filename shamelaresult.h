#ifndef SHAMELARESULT_H
#define SHAMELARESULT_H

#include <qstring.h>

class ShamelaResult
{
public:
    ShamelaResult();
    ~ShamelaResult();
    QString text();
    QString title();
    QString snippet();
    QString bgColor();
    QString toHtml();
    int page();
    int part();
    int id();
    int bookId();
    int archive();
    int score();

    void setText(const QString &text);
    void setTitle(const QString &title);
    void setSnippet(const QString &snippet);
    void setBgColor(const QString &color);
    void setBookName(const QString &name);
    void setPage(int page);
    void setPart(int part);
    void setId(int id);
    void setBookId(int id);
    void setArchive(int archive);
    void setScore(int score);

protected:
    QString m_text;
    QString m_title;
    QString m_snippet;
    QString m_bgColor;
    QString m_bookName;
    QString m_html;
    bool m_edited;
    int m_page;
    int m_part;
    int m_id;
    int m_bookId;
    int m_archive;
    int m_score;
};

#endif // SHAMELARESULT_H
