/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef TAB_H
#define TAB_H

#include <QString>

#include "link.h"

class Tab
{
public:
    explicit Tab(int tabId, Link currentLink, int nextLinkId, int previousLinkId);
    explicit Tab();

    int tabId() const;
    void setTabId(int tabId);

    Link currentLink() const;
    void setCurrentLink(const Link &currentLink);

    int previousLink() const;
    void setPreviousLink(int previousLinkId);

    int nextLink() const;
    void setNextLink(int nextLinkId);

    bool isValid() const;

    bool operator==(const Tab &other) const;
    bool operator!=(const Tab &other) const;

private:
    int m_tabId;
    Link m_currentLink;
    int m_nextLinkId;
    int m_previousLinkId;
};
#endif // TAB_H
