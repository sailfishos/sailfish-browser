#ifndef DECLARATIVEBOOKMARKMODEL_H
#define DECLARATIVEBOOKMARKMODEL_H

#include <QAbstractListModel>
#include <QMap>


class Bookmark {

public:
    Bookmark(QString url, QString title, QString favicon);

    QString title;
    QString url;
    QString favicon;
};

class DeclarativeBookmarkModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    DeclarativeBookmarkModel(QObject *parent = 0);
    
    enum BookmarkRoles {
           UrlRole = Qt::UserRole + 1,
           TitleRole,
           FaviconRole
    };

    Q_INVOKABLE void addBookmark(const QString& url, const QString& title);
    Q_INVOKABLE bool contains(const QString& url) const;

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:
    void countChanged();

private:
    QMultiMap<QString, int> bookmarks;
    QList<Bookmark*> titles;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
