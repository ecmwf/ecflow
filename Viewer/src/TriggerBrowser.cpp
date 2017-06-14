//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerBrowser.hpp"

#include <QDebug>
#include <QPlainTextEdit>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "Highlighter.hpp"
#include "TriggerCollector.hpp"
#include "TriggerItemWidget.hpp"
#include "VAttribute.hpp"
#include "VItemPathParser.hpp"

TriggerBrowser::TriggerBrowser(QWidget *parent) : QWidget(parent), owner_(0)
{
    setupUi(this);

#if 0
    Highlighter* ih=new Highlighter(triggerTe_->document(),"trigger");
    triggerTe_->setReadOnly(true);
    triggerTe_->setBackgroundVisible(true);

    //We use this plaintextedit to generate the syntax highligted html text for
    //the trigger expression.
    exprTe_=new QPlainTextEdit(this);
    exprTe_->hide();
    exprHighlight_=new Highlighter(exprTe_->document(),"trigger");

    //Set the height of the trigger expression display area
    QFont fTe;
    fTe.setBold(true);
    QFontMetrics fm(fTe);
    triggerTe_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    triggerTe_->setFixedHeight(fm.size(0,"A\nA\nA").height()+fm.height()/2);

#endif
    triggerCollector_=new TriggerListCollector(false);

    tgCollector_=new TriggerTableCollector(false);
    tgdCollector_=new TriggerTableCollector(false);

    Q_ASSERT(stacked_->count() == 2);
    stacked_->setCurrentIndex(panelIndexToInt(TablePanelIndex));

    //tab_->hide();

    //Q_ASSERT(tab_->count() == 3);
    //tab_->setCurrentIndex(tabIndexToInt(TriggerTabIndex));

    QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    f.setPointSize(10);
    f.setStyleStrategy(QFont::PreferAntialias);
    triggerBrowser_->setFont(f);
    //triggeredBrowser_->setFont(f);
    //selectedItemTextEdit_->setFont(f);

    connect(triggerBrowser_,SIGNAL(anchorClicked(const QUrl&)),
            this,SLOT(anchorClicked(const QUrl&)));

    //connect(triggeredBrowser_,SIGNAL(anchorClicked(const QUrl&)),
    //        this,SLOT(anchorClicked(const QUrl&)));
}

TriggerBrowser::~TriggerBrowser()
{
    clear();
    delete triggerCollector_;
    delete tgCollector_;
    delete tgdCollector_;
}

void TriggerBrowser::setOwner(TriggerItemWidget* owner)
{
    Q_ASSERT(!owner_);
    owner_=owner;
}

void TriggerBrowser::clear()
{   
    loadedPanels_.clear();
    triggerBrowser_->clear();
    //triggeredBrowser_->clear();
    triggerGraph_->clear();

    //It is safre to clear it after the graph is cleared!
    triggerCollector_->clear();
}

void TriggerBrowser::suspend()
{
    //triggerCollector_->clear();
}

void TriggerBrowser::load()
{    
    Q_ASSERT(owner_);

    loadedPanels_.clear();

    loadTriggerTab(true);
    loadTriggerGraphTab(true);

#if 0
    loadedTabs_.clear();

    if(tab_->currentIndex() == tabIndexToInt(TriggerTabIndex))
    {
        loadTriggerTab(true);
    }
    else if(tab_->currentIndex() == tabIndexToInt(TriggeredTabIndex))
    {
        loadTriggeredTab(true);
    }
    else if(tab_->currentIndex() == tabIndexToInt(TriggerGraphTabIndex))
    {
        loadTriggerGraphTab(true);
    }
#endif
}

void TriggerBrowser::loadTriggerGraphTab(bool forceLoad)
{
    //if(!forceLoad && isTabLoaded(TriggerGraphTabIndex))
    //    return;

    if(!forceLoad && isPanelLoaded(TablePanelIndex))
        return;

    //Q_EMIT triggerUpdateBegin();

    VNode *n=owner_->info()->node();
    Q_ASSERT(n);

    // put the trigger expression into the text box in the middle
    std::string te,ce;
    n->triggerExpr(te,ce);
    //triggerGraph_->setTriggerExpression(te);

#if 0
    triggerTe_->setPlainText(QString::fromStdString(te));
#endif
    triggerGraph_->beginTriggerUpdate();

    // collect the list of triggers of this node        
    tgCollector_->setDependency(owner_->dependency());
    n->triggers(tgCollector_);

    tgdCollector_->setDependency(owner_->dependency());
    n->triggered(tgdCollector_,owner_->triggeredScanner());

    triggerGraph_->setTriggerCollector(tgCollector_,tgdCollector_);

    triggerGraph_->endTriggerUpdate();

    //Q_EMIT triggerUpdateEnd();

    loadedPanels_.insert(TablePanelIndex);
}


void TriggerBrowser::loadTriggerTab(bool forceLoad)
{
    if(!forceLoad && isPanelLoaded(TextPanelIndex))
        return;

    VNode *n=owner_->info()->node();
    Q_ASSERT(n);

    std::string te,ce;
    n->triggerExpr(te,ce);

    QString s="<table width=\'100%\'>";


#if 0
    //Trigger expression
    if(!te.empty())
    {
        //Generate syntax highlighted html text for the trigger expression
        QString tb;
        exprTe_->setPlainText(QString::fromStdString(te));
        exprHighlight_->toHtml(tb);

        //We extract the useful bit from the html text
        QRegExp rx("<!--StartFragment-->(.+)<!--EndFragment-->");
        if(rx.indexIn(tb) > -1 && rx.captureCount() == 1)
        {
            tb=rx.cap(1);
        }

        tb="<font face=\'monospace\'>" + tb + "</font>";

        s+="<tr><td colspan=\'2\' class=\'trigger_title\'>Trigger expression</td></tr><tr><td colspan=\'2\' class=\'trigger\'> <p>" +
                 tb + "</p></td></tr>";
    }
#endif

    triggerCollector_->setDependency(owner_->dependency());

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
    n->triggers(triggerCollector_);
    s+=makeHtml(triggerCollector_,"Triggers directly triggering the selected node","Triggers");
    s+="</table>";
    triggerBrowser_->setHtml(s);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::restoreOverrideCursor();
#endif

    loadedPanels_.insert(TextPanelIndex);
}

void TriggerBrowser::loadTriggeredTab(bool forceLoad)
{
#if 0
    if(!forceLoad && isTabLoaded(TriggeredTabIndex))
        return;

    VNode *n=owner_->info()->node();
    Q_ASSERT(n);

    TriggerListCollector c(owner_->dependency());
    n->triggered(&c,owner_->triggeredScanner());

    QString s="<table width=\'100%\'>";
    s+=makeHtml(&c,"Nodes directly triggered by the selected node","Triggered");
    s+="</table>";
    triggeredBrowser_->setHtml(s);

    //loadedPanels_.insert(TriggeredTabIndex);
#endif
}

#if 0
void TriggerBrowser::on_tab__currentChanged(int idx)
{
    if(owner_ && !owner_->isSuspended())
    {
        if(idx == tabIndexToInt(TriggerTabIndex))
            loadTriggerTab();
        else if(idx == tabIndexToInt(TriggeredTabIndex))
            loadTriggeredTab();
        else if(idx == tabIndexToInt(TriggerGraphTabIndex))
            loadTriggerGraphTab();
    }
}
#endif
void TriggerBrowser::on_stacked__currentChanged(int idx)
{
    if(owner_ && !owner_->isSuspended())
    {
        if(idx == panelIndexToInt(TablePanelIndex))
            loadTriggerGraphTab();
        else if(idx == panelIndexToInt(TextPanelIndex))
            loadTriggerTab();
    }
}

void TriggerBrowser::setTextMode()
{
    stacked_->setCurrentIndex(TextPanelIndex);
}

void TriggerBrowser::setTableMode()
{
    stacked_->setCurrentIndex(TablePanelIndex);
}

bool TriggerBrowser::isPanelLoaded(PanelIndex idx) const
{
    return (loadedPanels_.find(idx) != loadedPanels_.end());
}

int TriggerBrowser::panelIndexToInt(PanelIndex idx) const
{
    return static_cast<int>(idx);
}

//Updates the trigger list if the right type of change happened
void TriggerBrowser::nodeChanged(const VNode* n)
{
    //if(!isTabLoaded(TriggerTabIndex))
    //   return;

    VNode *node=owner_->info()->node();
    Q_ASSERT(node);
    if(n == node)
    {
        loadTriggerTab(true);
        return;
    }

    const std::vector<TriggerListItem*>& items=triggerCollector_->items();
    for(unsigned int i=0; i < items.size(); i++)
    {
        if(VItem *t=items[i]->item())
        {
            if(VAttribute* a=t->isAttribute())
            {
                if(a->parent() == n)
                {
                    loadTriggerTab(true);
                    return;
                }
            }
        }
    }
}

void TriggerBrowser::anchorClicked(const QUrl& link)
{
    owner_->linkSelected(link.toString().toStdString());
    //tab_->setCurrentIndex(TriggerTabIndex);
}

QString TriggerBrowser::makeHtml(TriggerListCollector *tc,QString directTitle,QString modeText) const
{
    QString s;
    bool firstDirectTrigger=true;
    const std::vector<TriggerListItem*>& items=tc->items();

    for(unsigned int i=0; i < items.size(); i++)
    {
        VItem *t=items[i]->item();
        VItem *d=items[i]->dep();
        if(!t)
            continue;

        if(!d)
        {
            if(firstDirectTrigger)
            {
                s+="<tr><td class=\'direct_title\' colspan=\'2\'>" + directTitle + "</td></tr>";
                firstDirectTrigger=false;
            }

            QString type=QString::fromStdString(t->typeName());
            QString path=QString::fromStdString(t->fullPath());
            QString anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

            s+="<tr>";
            s+="<td width=\'100\'>" + type + "</td>";
            s+="<td><a href=\'" + anchor  + "\'>" + path +"</a>";
        }
    }

    QString prevH;

    for(unsigned int i=0; i < items.size(); i++)
    {
        VItem *t=items[i]->item();
        VItem *d=items[i]->dep();
        if(!t || !d)
            continue;

        QString h;

        if(items[i]->mode()== TriggerCollector::Parent)
           h+=modeText + " through parent";
        else
           h+=modeText + " through child";

        QString type=QString::fromStdString(d->typeName());
        QString path=QString::fromStdString(d->fullPath());
        QString anchor=QString::fromStdString(VItemPathParser::encode(d->fullPath(),d->typeName()));

        h+="  " + type;
        h+=" <a class=\'chp\' href=\'" + anchor + "\'>" + path +"</a>";

        if(h != prevH)
        {
            s+="<tr><td class=\'title\' colspan=\'2\'>" + h + "</td></tr>";
            prevH=h;
        }

        type=QString::fromStdString(t->typeName());
        path=QString::fromStdString(t->fullPath());
        anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

        s+="<tr>";
        s+="<td width=\'100\'>" + type + "</td>";
        s+="<td><a href=\'" +  anchor + "\'>" + path +"</a>";
     }

    return s;
}
