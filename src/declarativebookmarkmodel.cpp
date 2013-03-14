#include "declarativebookmarkmodel.h"

Bookmark::Bookmark(QString url, QString title, QString favicon) : url(url), title(title),favicon(favicon) {}

DeclarativeBookmarkModel::DeclarativeBookmarkModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[FaviconRole] = "favicon";
    setRoleNames(roles);
}

// TODO cleanup

void DeclarativeBookmarkModel::addBookmark(const QString& url, const QString& title) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    titles.append(new Bookmark(url, title, ""));
    bookmarks.insert(url, titles.count()-1);
    endInsertRows();

    emit countChanged();
}

int DeclarativeBookmarkModel::rowCount(const QModelIndex & parent) const {
    return titles.count();
}

QVariant DeclarativeBookmarkModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() > titles.count())
        return QVariant();

    const Bookmark * bookmark = titles[index.row()];
    if (role == UrlRole) {
        return bookmark->url;
    } else if (role == TitleRole) {
        return bookmark->title;
    } else if (role == FaviconRole) {
        return bookmark->favicon;
    }
    return QVariant();
}

bool DeclarativeBookmarkModel::contains(const QString& url) const {
    return bookmarks.contains(url);
}
