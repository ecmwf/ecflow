//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextFilterWidget.hpp"
#include "TextFilterHandlerDialog.hpp"

#include <QtGlobal>
#include <QCompleter>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QPalette>
#include <QPainter>
#include <QStyleOption>
#include <QTreeView>
#include <QWidgetAction>

#include "TextFormat.hpp"
#include "ViewerUtil.hpp"

TextFilterWidget::TextFilterWidget(QWidget *parent) :
    QWidget(parent),
    status_(EditStatus),
    statusTb_(0),
    optionTb_(0)
{
    setupUi(this);

    setProperty("textFilter","1");

    //match
    matchCb_->addItem(QIcon(QPixmap(":/viewer/filter_match.svg")),tr("match"),0);
    matchCb_->addItem(QIcon(QPixmap(":/viewer/filter_no_match.svg")),tr("no match"),1);
    matchCb_->setItemData(0,tr("Show only the <b>lines matching</b> the filter experssion"),Qt::ToolTipRole);
    matchCb_->setItemData(1,tr("Show only the <b>lines not matching</b> the filter experssion"),Qt::ToolTipRole);

    //Editor
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    le_->setPlaceholderText(tr(" Regexp (grep)"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
#endif

    oriBrush_=QBrush(le_->palette().brush(QPalette::Base));
    redBrush_=ViewerUtil::lineEditRedBg();
    greenBrush_=ViewerUtil::lineEditGreenBg();

    completer_=new QCompleter(this);    
    le_->setCompleter(completer_);

    QIcon icon;
    icon.addPixmap(QPixmap(":/viewer/close_grey.svg"),QIcon::Normal);
    icon.addPixmap(QPixmap(":/viewer/close_red.svg"),QIcon::Active);
    closeTb_->setIcon(icon);

    refreshCompleter();

    //Initially it is hidden
    hide();
}

bool TextFilterWidget::isActive() const
{
    return !le_->text().isEmpty();
}

bool TextFilterWidget::isCaseSensitive() const
{
    return caseCb_->isChecked();
}

bool TextFilterWidget::isMatched() const
{
    return matchCb_->currentIndex() == 0;
}

QString TextFilterWidget::filterText() const
{
    return le_->text();
}

void TextFilterWidget::init(const TextFilterItem& item)
{
    matchCb_->setCurrentIndex(item.matched()?0:1);
    caseCb_->setChecked(item.caseSensitive());
    le_->setText(QString::fromStdString(item.filter()));
}

void TextFilterWidget::setExternalButtons(QToolButton* statusTb,QToolButton* optionTb)
{
    Q_ASSERT(statusTb);
    Q_ASSERT(optionTb);

    statusTb_=statusTb;
    optionTb_=optionTb;

    statusTb_->setChecked(false);

    connect(statusTb_,SIGNAL(toggled(bool)),
            this,SLOT(slotStatusTb(bool)));

    connect(optionTb_,SIGNAL(clicked()),
            this,SLOT(slotOptionTb()));
}

void TextFilterWidget::refreshCompleter()
{
    QStringList lst;
    std::set<std::string> vals;
    TextFilterHandler::Instance()->allFilters(vals);

    for(std::set<std::string>::const_iterator it=vals.begin(); it != vals.end(); ++it)
    {
        lst << QString::fromStdString(*it);
    }
    le_->setCompleter(new QCompleter(lst,le_));
}

void TextFilterWidget::slotFilterEditor()
{

}

void TextFilterWidget::buildMenu(QToolButton *tb)
{
    QMenu* menu=new QMenu(tb);

    QAction *manageAc=new QAction(menu);
    manageAc->setText(tr("Manage filters ..."));
    manageAc->setIcon(QPixmap(":/viewer/configure.svg"));
    menu->addAction(manageAc);

    QAction *saveAc=0;
    if(isVisible())
    {
        saveAc=new QAction(menu);
        saveAc->setText(tr("Save filter"));
        saveAc->setIcon(QPixmap(":/viewer/filesaveas.svg"));
        if(!isActive() || isCurrentSaved()) saveAc->setEnabled(false);
        menu->addAction(saveAc);
    }

    QAction *sep=new QAction(menu);
    sep->setSeparator(true);
    menu->addAction(sep);

    QAction *clearAc=new QAction(menu);
    clearAc->setText(tr("Clear filter"));
    if(!isActive()) clearAc->setEnabled(false);
    menu->addAction(clearAc);

    QAction *sep1=new QAction(menu);
    sep1->setSeparator(true);
    menu->addAction(sep1);

    addMenuSection(menu,TextFilterHandler::Instance()->items(),tr("Saved"),"s");
    addMenuSection(menu,TextFilterHandler::Instance()->latestItems(),tr("Recent"),"r");

    if(QAction *ac=menu->exec(QCursor::pos()))
    {      
        //Start manage filters dialogue
        if(ac == manageAc)
        {
            TextFilterHandlerDialog diag;
            diag.exec();
            int pos=diag.applyIndex();
            if(pos >=0 && TextFilterHandler::Instance()->latestItems().size())
            {
                TextFilterItem item=TextFilterHandler::Instance()->items()[pos];
                if(!item.filter().empty())
                {
                    init(item);
                    Q_EMIT runRequested(QString::fromStdString(item.filter()),item.matched(),item.caseSensitive());
                }
            }
            else
            {
                refreshCompleter();
            }
        }
        //Save current filter
        else if(ac == saveAc)
        {
            std::string filter=filterText().toStdString();
            bool matchMode=isMatched();
            bool caseSensitive=isCaseSensitive();

            int pos=TextFilterHandler::Instance()->indexOf(filter,matchMode,caseSensitive);
            if(pos != -1)
            {
                TextFilterItem it=TextFilterHandler::Instance()->items()[pos];

                //Enable context menu for already saved items
                if(!it.contextMenu())
                {
                   it.setContextMenu(true);
                   TextFilterHandler::Instance()->update(pos,it);
                }
                return;
            }

            TextFilterHandler::Instance()->add(filter,matchMode,caseSensitive,true);
            refreshCompleter();
        }
         //Clear current filter
        else if(ac == clearAc)
        {
            le_->clear();
            setStatus(EditStatus);
            refreshCompleter();
        }
        //Load a filter
        else
        {
            QStringList id=ac->data().toString().split("_");
            if(id.count() == 2)
            {
                std::size_t pos=id[1].toInt();
                TextFilterItem item("","");
                if(id[0] == "s")
                {
                    if(pos >=0 && TextFilterHandler::Instance()->items().size())
                    {
                        item=TextFilterHandler::Instance()->items()[pos];
                    }
                }

                else if(id[0] == "r")
                {
                    if(pos >=0 && TextFilterHandler::Instance()->latestItems().size())
                    {
                        item=TextFilterHandler::Instance()->latestItems()[pos];
                    }
                }

                if(!item.filter().empty())
                {
                    init(item);
                    Q_EMIT runRequested(QString::fromStdString(item.filter()),item.matched(),item.caseSensitive());
                }
            }
        }

    }

    menu->clear();
    menu->deleteLater();
}

void TextFilterWidget::addMenuSection(QMenu* menu,const std::vector<TextFilterItem>& items,QString title,QString data)
{
    if(items.empty())
        return;

    QAction *sep1=new QAction(menu);
    sep1->setSeparator(true);
    menu->addAction(sep1);

    if(!title.isEmpty())
    {
        QAction* acTitle = new QAction(menu);
        acTitle->setText(title);
        QFont f=acTitle->font();
        f.setBold(true);
        acTitle->setFont(f);
        menu->addAction(acTitle);
    }

    for(std::size_t i=0 ; i < items.size(); i++)
    {
        if(data != "s" || items[i].contextMenu())
        {
            QAction* ac=new QAction(this);

            QString txt=QString::fromStdString(items[i].filter());
            //Replace whitespace with Open Box U+2423 just for better interpretation
            txt.replace(QChar(' '),QChar(9251));
            txt+="   ("  + QString(items[i].caseSensitive()?"cs":"ci") + ")";

            ac->setText(txt);
            ac->setData(data + "_" + QString::number(i)); //set an id for the action

            if(items[i].matched())
                ac->setIcon(QPixmap(":/viewer/filter_match.svg"));
            else
                ac->setIcon(QPixmap(":/viewer/filter_no_match.svg"));

            menu->addAction(ac);
        }
    }
}

void TextFilterWidget::runIt()
{
    QString t=le_->text();
    if(!t.isEmpty())
        Q_EMIT runRequested(t,isMatched(),isCaseSensitive());
}

void TextFilterWidget::slotStatusTb(bool b)
{
    if(b)
    {
        show();
        setEditFocus();
    }
    else
    {
        hide();
        adjustToolTip();
    }
}

void TextFilterWidget::on_closeTb__clicked()
{
    //clear
    le_->clear();
    setStatus(EditStatus);
    refreshCompleter();

    //hide
    statusTb_->setChecked(false);
    Q_EMIT closeRequested(); //this will clear the filter

}

void TextFilterWidget::on_le__returnPressed()
{
    runIt();
}

void TextFilterWidget::on_le__textChanged()
{
    if(status_ != EditStatus)
        setStatus(EditStatus);

    if(!isActive())
        Q_EMIT clearRequested();
}

void TextFilterWidget::on_matchCb__currentIndexChanged(int)
{
    runIt();
}

void TextFilterWidget::on_caseCb__stateChanged(int)
{
    runIt();
}

void TextFilterWidget::slotOptionTb()
{
    Q_ASSERT(optionTb_);
    buildMenu(optionTb_);
}

void TextFilterWidget::setEditFocus()
{
    le_->setFocus();
}

void TextFilterWidget::adjustToolTip()
{
    if(!statusTb_)
        return;

    QString filterDesc;
    if(status_ == FoundStatus || status_ == NotFoundStatus)
    {
        if(!isVisible())
          filterDesc+=tr("Click to <b>show</b> text filter bar<br>----------------------------------------<br>");
        else
          filterDesc+=tr("Click to <b>hide</b> text filter bar. The filter remains active (if defined). <br>----------------------------------------<br>");

          filterDesc+=tr("Current filter:") +
                "<br><b>&nbsp;regexp:</b> " + filterText() +
                "<br><b>&nbsp;mode: </b>" + (isMatched()?"match":"no match") + ", " +
                + (isCaseSensitive()?"case sensitive":"case insensitive") +
                "<br><br>";
     }

     switch(status_)
     {
     case EditStatus:
        statusTb_->setToolTip(tr(isVisible()?"<b>Hide</b> text filter bar. The filter remains active (if defined).":"<b>Show</b> text filter bar"));
        break;
     case FoundStatus:
            statusTb_->setToolTip(filterDesc + tr("There ") +
                Viewer::formatText("are lines",QColor(100,220,120)) +
                tr(" matching the filter in the output file"));
        break;
     case NotFoundStatus:
        statusTb_->setToolTip(filterDesc + tr("There ") +
            Viewer::formatText("are no lines",QColor(255,95,95)) +
            tr(" matching the filter in the output file"));
        break;
     default:
        break;
     }
}

void TextFilterWidget::setStatus(FilterStatus status,bool force)
{
    if(force || status_ != status)
    {
        status_=status;

        QBrush br=oriBrush_;
        QPalette p=le_->palette();
        switch(status_)
        {
        case EditStatus:
            br=oriBrush_;
            if(statusTb_)
            {
                statusTb_->setIcon(QPixmap(":/viewer/filter_decor.svg"));               
            }
            break;
        case FoundStatus:
            br=greenBrush_;
            if(statusTb_)
            {
                statusTb_->setIcon(QPixmap(":/viewer/filter_decor_green.svg"));                
            }
            addCurrentToLatest();
            break;
        case NotFoundStatus:
            br=redBrush_;
            if(statusTb_)
            {
                statusTb_->setIcon(QPixmap(":/viewer/filter_decor_red.svg"));              
            }
            break;
        default:
            break;
        }

        p.setBrush(QPalette::Base,br);
        le_->setPalette(p);

        adjustToolTip();
    }
}

void TextFilterWidget::addCurrentToLatest()
{
    TextFilterHandler::Instance()->addLatest(filterText().toStdString(),
                                             isMatched(),isCaseSensitive(),true);
}

bool TextFilterWidget::isCurrentSaved() const
{
    return TextFilterHandler::Instance()->contains(filterText().toStdString(),
                                             isMatched(),isCaseSensitive());
}

void TextFilterWidget::paintEvent(QPaintEvent *)
{
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void TextFilterWidget::showEvent(QShowEvent *)
{
    adjustToolTip();
}

