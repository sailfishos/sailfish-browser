#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QObject>
#include <QMap>

class Bookmark;

class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    static BookmarkManager* instance();

    void save(const QMap<QString, Bookmark*> & bookmarks);
    void clear();
    QMap<QString, Bookmark*> load();

signals:
    void cleared();
};

#endif // BOOKMARKMANAGER_H
