//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerBrowser.hpp"

#include <QDebug>

#include "TriggerCollector.hpp"
#include "TriggerItemWidget.hpp"

TriggerBrowser::TriggerBrowser(QWidget *parent) : QWidget(parent), owner_(0)
{
    setupUi(this);

    Q_ASSERT(tab_->count() == 2);
    tab_->setCurrentIndex(tabIndexToInt(TriggerTabIndex));

    connect(triggerBrowser_,SIGNAL(anchorClicked(const QUrl&)),
            this,SLOT(anchorClicked(const QUrl&)));

    connect(triggeredBrowser_,SIGNAL(anchorClicked(const QUrl&)),
            this,SLOT(anchorClicked(const QUrl&)));

}

void TriggerBrowser::setOwner(TriggerItemWidget* owner)
{
    Q_ASSERT(!owner_);
    owner_=owner;
}

void TriggerBrowser::clear()
{
    loadedTabs_.clear();
    triggerBrowser_->clear();
    triggeredBrowser_->clear();
}

void TriggerBrowser::load()
{    
    Q_ASSERT(owner_);

    loadedTabs_.clear();

    if(tab_->currentIndex() == tabIndexToInt(TriggerTabIndex))
    {
        loadTriggerTab(true);
    }
    else if(tab_->currentIndex() == tabIndexToInt(TriggeredTabIndex))
    {
        loadTriggeredTab(true);
    }
}

void TriggerBrowser::loadTriggerTab(bool forceLoad)
{
    if(!forceLoad && isTabLoaded(TriggerTabIndex))
        return;

    VNode *n=owner_->info()->node();
    Q_ASSERT(n);

    std::string te,ce;
    n->triggerExpr(te,ce);

    QString s;
    if(!te.empty())
    {
        s+="<b>Trigger expression:</b><p>" + QString::fromStdString(te) + "<p>";
    }

    TriggerListCollector c(0,"",owner_->dependency());
    n->triggers(&c);

    triggerBrowser_->setHtml(s + c.text());

    loadedTabs_.insert(TriggerTabIndex);
}

void TriggerBrowser::loadTriggeredTab(bool forceLoad)
{
    if(!forceLoad && isTabLoaded(TriggeredTabIndex))
        return;

    VNode *n=owner_->info()->node();
    Q_ASSERT(n);

    TriggerListCollector c(0,"",owner_->dependency());
    n->triggered(&c,owner_->triggeredScanner());

    triggeredBrowser_->setHtml(c.text());

    loadedTabs_.insert(TriggeredTabIndex);
}

void TriggerBrowser::on_tab__currentChanged(int idx)
{
    if(owner_)
    {
        if(idx == tabIndexToInt(TriggerTabIndex))
            loadTriggerTab();
        else if(idx == tabIndexToInt(TriggeredTabIndex))
            loadTriggeredTab();
    }
}

bool TriggerBrowser::isTabLoaded(TabIndex idx) const
{
    return (loadedTabs_.find(idx) != loadedTabs_.end());
}

int TriggerBrowser::tabIndexToInt(TabIndex idx) const
{
    return static_cast<int>(idx);
}

void TriggerBrowser::anchorClicked(const QUrl& link)
{
    qDebug() << link.url();
    owner_->linkSelected(link.url().toStdString());
}
