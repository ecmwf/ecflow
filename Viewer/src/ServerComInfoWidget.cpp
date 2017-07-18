//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerComInfoWidget.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

#include <cmath>

#include <boost/current_function.hpp>

#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "ToolTipFormat.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"

QIcon* ServerRefreshInfoWidget::icon_=0;
QBrush ServerRefreshInfoWidget::bgBrush_(QColor(229,228,227));
QPen ServerRefreshInfoWidget::borderPen_(QColor(167,167,167));
QPen ServerRefreshInfoWidget::disabledBorderPen_(QColor(182,182,182));
QBrush ServerRefreshInfoWidget::buttonBgHoverBrush_(QColor(249,248,248));
QPen ServerRefreshInfoWidget::buttonHoverPen_(QColor(160,160,160));
//QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(219,238,246));
QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(213,222,227));
QBrush ServerRefreshInfoWidget::timeBgBrush_(QColor(214,214,214));
QBrush ServerRefreshInfoWidget::progBrush_(QColor(38,181,245));
QBrush ServerRefreshInfoWidget::progBgBrush_(QColor(248,248,248));
QPen ServerRefreshInfoWidget::buttonRefreshPen_(QColor(38,181,245),2);
QPen ServerRefreshInfoWidget::textPen_(QColor(80,80,80));
QPen ServerRefreshInfoWidget::refreshTextPen_(QColor(41,79,143));
QPen ServerRefreshInfoWidget::disabledTextPen_(QColor(180,180,180));

#define _UI_SERVERCOMINFOWIDGET_DEBUG

#if 0
ServerRefreshInfoWidget::ServerRefreshInfoWidget(QAction* refreshAction,QWidget *parent) :
    QWidget(parent),
    refreshAction_(refreshAction)
{
    Q_ASSERT(refreshAction_);

    QHBoxLayout *hb=new QHBoxLayout(this);
    hb->setContentsMargins(0,0,0,0);
    hb->setSpacing(0);

    //QToolButton* refreshTb=new QToolButton(this);
   //refreshTb->setAutoRaise(true);
    //refreshTb->setDefaultAction(refreshAction_);
    //hb->addWidget(refreshTb);

    infoW_=new ServerComLineDisplay(this);
    hb->addWidget(infoW_);

    IconProvider::add(":/viewer/reload_one.svg","reload_one");


}
#endif

ServerRefreshInfoWidget::ServerRefreshInfoWidget(QAction* refreshAction,QWidget *parent) :
    QWidget(parent),
    refreshAction_(refreshAction),
    server_(0),
    font_(QFont()),
    fontTime_(QFont()),
    fontUpdate_(QFont()),
    fm_(QFont()),
    fmTime_(QFont()),
    fmUpdate_(QFont()),
    currentComponent_(NoComponent),
    prop_(0),
    showCountdownArc_(true),
    showCountdownText_(true),
    mode_(NoMode),
    fastMode_(false),
    contModeLimit_(4),
    fastModeLimit_(15),
    hasInfo_(false),
    inRefresh_(true),
    userInitiatedRefresh_(false),
    showLastRefresh_(true),
    total_(-1),
    period_(-1),
    toNext_(-1)
{
    Q_ASSERT(refreshAction_);

    //The icon for the round refresh button
    if(!icon_)
        icon_=new QIcon(QPixmap(":/viewer/reload_black.svg"));

    font_=QFont();
    font_.setPointSize(font_.pointSize()-1);
    fm_=QFontMetrics(font_);

    fontTime_=QFont();
    fontTime_.setPointSize(fontTime_.pointSize()-2);
    fmTime_=QFontMetrics(fontTime_);

    fontUpdate_=QFont();
    fontUpdate_.setPointSize(fontUpdate_.pointSize()-3);
    fmUpdate_=QFontMetrics(fontUpdate_);

    int width_=200;
    int height_=fm_.height()+6;

    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(slotTimeOut()));

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);

    adjustGeometry(false);

    //timeTextLen_=qMax(fmTime_.width(" <9:59m left"),fmUpdate_.width("updating"));
    buttonRect_=QRect(1,1,height()-2,height()-2);
    buttonRadius2_=pow(buttonRect_.width()/2,2);

    setMouseTracking(true);

    std::vector<std::string> propVec;
    //propVec.push_back("server.update.showUpdateCountdownArc");
    //propVec.push_back("server.update.showUpdateCountdownText");
    propVec.push_back("server.update.blinkUpdateButtonLimit");
    prop_=new PropertyMapper(propVec,this);
    updateSettings();
}

ServerRefreshInfoWidget::~ServerRefreshInfoWidget()
{
    //Detach from the server
    if(server_)
    {
        server_->removeServerObserver(this);
        server_->removeServerComObserver(this);
    }
    delete prop_;
}

bool ServerRefreshInfoWidget::isActionEnabled() const
{
    return refreshAction_->isEnabled();
}

void ServerRefreshInfoWidget::notifyChange(VProperty* p)
{
    updateSettings();
}

void ServerRefreshInfoWidget::updateSettings()
{
    /*if(VProperty* p=prop_->find("server.update.showUpdateCountdownArc"))
    {
        showCountdownArc_=p->value().toBool();
    }
    if(VProperty* p=prop_->find("server.update.showUpdateCountdownText"))
    {
        showCountdownText_=p->value().toBool();
    }*/
    if(VProperty* p=prop_->find("server.update.blinkUpdateButtonLimit"))
    {
        bool v=p->value().toInt();
        if(fastModeLimit_!= v)
        {
            fastModeLimit_=v;
            adjustGeometry(true);
        }
        else
            fastModeLimit_=v;
    }

    reloadAll();
}

void ServerRefreshInfoWidget::setServer(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
     UI_FUNCTION_LOG
#endif

    if(server_ != server && server_)
    {
        server_->removeServerObserver(this);
        server_->removeServerComObserver(this);
    }

    server_=server;

    if(server_)
    {
        server_->addServerObserver(this);
        server_->addServerComObserver(this);
    }
    else
    {
        hasInfo_=0;
    }

    refreshAction_->setEnabled(true);
    //fastMode_=false;
    inRefresh_=false;
    mode_=NoMode;

    //Cache some data
    serverName_.clear();
    serverText_.clear();
    if(server_)
    {
        serverName_=QString::fromStdString(server_->name());
        serverText_=serverName_ + " ";      
    }

    //Adjust width + get info
    adjustGeometry(true);

    //rerender
    update();
}

//-------------------------------
// Server observer notifications
//-------------------------------

void ServerRefreshInfoWidget::notifyServerDelete(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif

    Q_ASSERT(server_ == server);
    if(server_ == server)
    {
        server_->removeServerObserver(this);
        server_->removeServerComObserver(this);
        server_=0;
        serverName_.clear();
        serverText_.clear();
        hasInfo_=false;
        mode_=NoMode;
        inRefresh_=false;
        userInitiatedRefresh_=false;

        //get info and rerender
        reloadAll();
    }
}

//While the server is being reloaded the refresh button is disabled. It must be followed by a
//notifyEndServerScan() call!
void ServerRefreshInfoWidget::notifyBeginServerClear(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    timer_->stop();
    refreshAction_->setEnabled(false);
    hasInfo_=false;
    inRefresh_=false;
    userInitiatedRefresh_=false;
    mode_=NoMode;
    update();
}

//The server has been reloaded. We must get the current state.
void ServerRefreshInfoWidget::notifyEndServerScan(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    refreshAction_->setEnabled(true);
    reloadAll();
}

//virtual void notifyServerConnectState(ServerHandler* server) {}
void ServerRefreshInfoWidget::notifyServerActivityChanged(ServerHandler* /*server*/)
{

}

//-----------------------------------
// Server com observer notifications
//-----------------------------------

void ServerRefreshInfoWidget::notifyRefreshTimerStarted(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    reloadAll(); //get info and rerender
}

void ServerRefreshInfoWidget::notifyRefreshTimerStopped(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    reloadAll(); //get info and rerender
}

void ServerRefreshInfoWidget::notifyRefreshTimerChanged(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
    printStatus();
#endif
    Q_ASSERT(server_ == server);

    //if we are in refresh we do not want to fetch any new information unless we are in
    //NoMode or ContMode
    if(!inRefresh_ || mode_==ContMode || mode_==NoMode)
    {
        reloadAll(); //get info and rerender
    }
}

//While the refresh is being executed the the refresh button
void ServerRefreshInfoWidget::notifyRefreshScheduled(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    inRefresh_=true;
    inRefreshElapsed_.restart();
    if(mode_ == NormalMode || mode_ == FastMode)
    {
        //refreshAction_->setEnabled(false);
        timer_->stop();

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        printStatus();
#endif
        //redraw
        update();
    }
}

void ServerRefreshInfoWidget::notifyRefreshFinished(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    int elapsed=inRefreshElapsed_.elapsed();

    if(mode_ == NormalMode || mode_ == FastMode && elapsed < 450)
    {
        Q_ASSERT(500-elapsed > 0);
        //We keep the button in inRefresh state for 1 sec (the timer is stopped now!!)
        QTimer::singleShot(500-elapsed,this,SLOT(slotTimeOutRefreshFinished()));
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        printStatus();
#endif
    }
    else
    {
        inRefresh_=false;
        userInitiatedRefresh_=false;
    }
}

void ServerRefreshInfoWidget::reloadAll()
{
    fetchInfo(); //get info
    update();    //renrender
}

void ServerRefreshInfoWidget::slotTimeOut()
{
    reloadAll();
}

void ServerRefreshInfoWidget::slotTimeOutRefreshFinished()
{
    inRefresh_=false;
    userInitiatedRefresh_=false;
    reloadAll();
}

void ServerRefreshInfoWidget::fetchInfo()
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
    printStatus();
#endif

    if(!server_)
    {
        mode_=NoMode;
        hasInfo_=false;
        timer_->stop();
    }
    else
    {
        int drift;
        QDateTime currentTime=QDateTime::currentDateTime();
        bool v=server_->updateInfo(period_,total_,drift,toNext_);

        bool geoUpdateNeeded=(v != hasInfo_);
        hasInfo_=v;

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        UiLog().dbg() << " period=" << period_ << " total=" << total_ <<
                         " toNext=" << toNext_;
#endif

        //the server has an automatic update
        if(hasInfo_)
        {
            lastRefresh_=server_->lastRefresh().time().toString(); //currentTime.addSecs(-total_+toNext_).time().toString();
            nextRefresh_=currentTime.addSecs(toNext_).time().toString();

            //In ContMode:
            // -the action is disabled
            // -there is a special tooltip
            // -the timer is stopped
            // -there is no blinking
            // -the progress bar is always at 100&
            // -a warning colour is used to draw the border
            if(total_ < 0) //contModeLimit_)
            {
                mode_=ContMode;
                refreshAction_->setEnabled(false);
            }
            //In FastMode:
            // -there is no blinking
            else if(total_ < 0) //fastModeLimit_)
            {
                mode_=FastMode;
                if(!inRefresh_)
                    refreshAction_->setEnabled(true);
            }
            //NormalMode
            else
            {
                mode_=NormalMode;
                if(!inRefresh_)
                    refreshAction_->setEnabled(true);
            }
        }
        //the server's automatic refresh is switched off
        else
        {
            lastRefresh_=server_->lastRefresh().time().toString();
            mode_=ManualMode;
            if(!inRefresh_)
                refreshAction_->setEnabled(true);
        }


        if(geoUpdateNeeded)
            adjustGeometry(false);

        //if(currentComponent_ == TextComponent)
        adjustToolTip();

        adjustTimer(toNext_);
    }

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    printStatus();
#endif
}

//Adjust timer interval
void ServerRefreshInfoWidget::adjustTimer(int toNext)
{
    //we stop the timer when:
    // -the action is disabled
    // -we are in fast mode
    // -the countdown is not shown
    //if(!refreshAction_->isEnabled() || fastMode_ || !showCountdown())
    if(!refreshAction_->isEnabled() || mode_ == NoMode || mode_ == ManualMode || mode_ == ContMode )
    {
        timer_->stop();

    }
    else if(hasInfo_)
    {
        if(period_ == 1)
           timer_->setInterval(1000);
        else if(period_==2)
           timer_->setInterval(750);
        else
        {
            if(toNext > 135)
                timer_->setInterval(60*1000);
            else if(toNext > 105)
                timer_->setInterval(30*1000);
            else  if(toNext > 75)
                timer_->setInterval(15*1000);
            else if(toNext > 30)
                timer_->setInterval(5*1000);
            else if(toNext > 15)
                timer_->setInterval(2500);
            else if(toNext > 6)
                timer_->setInterval(2000);
            else
                timer_->setInterval(1000);
        }

        if(!timer_->isActive())
            timer_->start();
    }
    else
    {
        timer_->stop();
    }
}

//Check if a point is inside the (round) button
bool ServerRefreshInfoWidget::isInButton(const QPoint& pos) const
{
    QPoint d=pos-buttonRect_.center();
    return d.x()*d.x()+d.y()*d.y() <= buttonRadius2_;
}

bool ServerRefreshInfoWidget::isInText(const QPoint& pos) const
{
    return (pos.x() > buttonRect_.right());
}

void ServerRefreshInfoWidget::resizeEvent(QResizeEvent* event)
{
    buttonRect_=QRect(1,1,height()-2,height()-2);
    buttonRadius2_=pow(buttonRect_.width()/2,2);
}

void ServerRefreshInfoWidget::mousePressEvent(QMouseEvent* event)
{
    if(!refreshAction_->isEnabled())
        return;

    //We are in the button
    if(isInButton(event->pos()))
    {
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
//        UiLog().dbg() << "pressed";
#endif
        if(currentComponent_ != ButtonComponent)
        {
            currentComponent_ == ButtonComponent;
        }
        userInitiatedRefresh_=true;
        refreshAction_->trigger();
    }
}

void ServerRefreshInfoWidget::mouseMoveEvent(QMouseEvent* event)
{
    //We are in the button
    if(isInButton(event->pos()))
    {
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
//        UiLog().dbg() << "inButton";
#endif
        //we just entered the button
        if(currentComponent_ != ButtonComponent)
        {
            currentComponent_=ButtonComponent;
/*            if(mode_ == ContMode || mode_ == NoMode )
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }*/

            adjustToolTip();
            if(refreshAction_->isEnabled())
            {
                update(); //rerender
            }
        }
    }
    //We are in the progress part
    else if(isInText(event->pos()))
    {
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
//        UiLog().dbg() << "inText";
#endif
        //we came from the button
        if(currentComponent_ == ButtonComponent)
        {
            currentComponent_=TextComponent;
            /*if(mode_ == NormalMode || mode_ == FastMode)
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }*/
            adjustToolTip();
            if(refreshAction_->isEnabled())
            {
                update(); //rerender
            }
        }
        //we came from outside
        else if(currentComponent_ != TextComponent)
        {
            currentComponent_=TextComponent;
            /*if(!showCountdown())
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }*/
            adjustToolTip();
        }
    }
}

void ServerRefreshInfoWidget::leaveEvent(QEvent*)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    //UI_FUNCTION_LOG
#endif
    currentComponent_=NoComponent;
    update(); //rerender
}


void ServerRefreshInfoWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    drawProgress(&painter);
    drawButton(&painter);
}

void ServerRefreshInfoWidget::adjustGeometry(bool doFetchInfo)
{
    if(server_)
    {
        timeTextLen_=qMax(fmTime_.width(" <9:59m"),fmUpdate_.width("updating"));
        lastTextLen_=fmTime_.width(" last: 22:22:22 ");

        //Define server rect
        int yPadding=5;
        int h=height()-2*yPadding;
        int currentRight=0;

        serverRect_=QRect(buttonRect_.center().x()+4,yPadding,
                          buttonRect_.width()/2-4+4+fm_.width(serverText_),
                          h);
        currentRight=serverRect_.x()+serverRect_.width();

        if(doFetchInfo)
            fetchInfo();

        timeRect_=QRect();
        progRect_=QRect();
        lastRect_=QRect();

        if(hasInfo_)
        {
            timeRect_ = serverRect_;
            timeRect_.setX(serverRect_.x()+serverRect_.width());
            timeRect_.setWidth(timeTextLen_);
            timeRect_.setHeight(fmTime_.height()-2);

            progRect_ = timeRect_;
            progRect_.setY(timeRect_.y()+timeRect_.height());
            progRect_.setHeight(serverRect_.height()-timeRect_.height());

            currentRight+=timeRect_.width();

        }

        if((hasInfo_ && showLastRefresh_) || !hasInfo_)
        {
            lastRect_ = QRect(currentRight,serverRect_.y(),lastTextLen_,h);
            currentRight+=lastRect_.width()+6;

            //setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width(serverText_) +
            //          timeTextLen_ +6 + lastTextLen_);
        }
        else
        {
            //setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width(serverText_) +
            //          timeTextLen_ +6);
        }

        setFixedWidth(currentRight);
    }
    else
    {
        timeTextLen_=0;
        lastTextLen_=0;
        setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width("AAAAA"));
    }
}


void ServerRefreshInfoWidget::drawButton(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing,true);

    if(server_)
    {
        //blink
        if(inRefresh_ &&
           (mode_ == NormalMode || mode_ == ManualMode || userInitiatedRefresh_))
        {
            painter->setBrush(buttonBgRefreshBrush_);
            painter->setPen(buttonRefreshPen_);
        }
        else
        {
            painter->setBrush((currentComponent_ == ButtonComponent)?buttonBgHoverBrush_:bgBrush_);

            if(!refreshAction_->isEnabled())
                painter->setPen(disabledBorderPen_);
            else
                painter->setPen((currentComponent_ == ButtonComponent)?buttonHoverPen_:borderPen_);
        }
    }
    else
    {
        painter->setBrush(bgBrush_);
        painter->setPen(borderPen_);
    }

    //The filled circle
    painter->drawEllipse(buttonRect_);

    //The reload icon
    QRect r1=buttonRect_.adjusted(2,2,-2,-2);
    QRect r2=r1.adjusted(3,3,-3,-3);
    QPixmap pix=icon_->pixmap(QSize(r2.width(),r2.width()),
                             refreshAction_->isEnabled()? QIcon::Normal: QIcon::Disabled);
    painter->drawPixmap(r2,pix);
}

void ServerRefreshInfoWidget::drawProgress(QPainter* painter)
{
     if(!server_)
         return;

    //Define server rect
    //int yPadding=5;
    //int h=height()-2*yPadding;

    //QRect serverRect=QRect(buttonRect_.center().x()+4,yPadding,
    //                   buttonRect_.width()/2-4+4+fm_.width(serverText_),
    //                   h);

    QRect serverTextRect=serverRect_.adjusted(buttonRect_.width()/2-4+4,0,0,0);

    QString timeText;
    //QRect   timeRect,progRect;
    if(hasInfo_) // && showCountdownText_)
    {
        timeText =QChar(916) + QString("=") + QString::number(total_) + "s";

        /*if(fastMode_)
            timeText = "<" + QString::number(total_) + "s";
        else if(inRefresh_)
            timeText = "updating";
        else
            timeText =QChar(916) + QString("=") + QString::number(total_) + "s"; //formatTime(toNext_) + " left";
*/
        //timeRect = serverRect;
        //timeRect.setX(serverRect.x()+serverRect.width());
        //timeRect.setWidth(timeTextLen_);

        //timeRect.setHeight(fmTime_.height()-2);
        //progRect = timeRect;
        //progRect.setY(timeRect.y()+timeRect.height());
        //progRect.setHeight(serverRect.height()-timeRect.height());

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        UiLog().dbg() << "timeText=" << timeText;
#endif
    }

    //Start painting
    painter->setFont(font_);

    //Server rect
    painter->setBrush(Qt::NoBrush);
    painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
    painter->drawRect(serverRect_);

    //Server text
    painter->setPen((refreshAction_->isEnabled())?textPen_:disabledTextPen_);
    painter->drawText(serverTextRect,Qt::AlignLeft | Qt::AlignVCenter,serverText_);

    //The time rects and texts
    if(hasInfo_)
    {
        //Time rect
        //if(inRefresh_ && !fastMode_)
        //    painter->setFont(fontUpdate_);
       // else
            painter->setFont(fontTime_);

         painter->setBrush(timeBgBrush_);
         painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
         painter->drawRect(timeRect_);

        //Time text
        //if(fastMode_ || inRefresh_)
        //    //painter.setPen(QColor(0,136,0));
        //    painter->setPen(refreshTextPen_);
        //else
            painter->setPen((refreshAction_->isEnabled())?textPen_:disabledTextPen_);

        painter->drawText(timeRect_,Qt::AlignHCenter | Qt::AlignVCenter,timeText);

        float progress;
        QRect actProgRect=progRect_;

        if(mode_==ContMode || inRefresh_)
            progress=1;
        else
        {
            UI_ASSERT(total_ != 0, "total_=" << total_);
            progress=(static_cast<float>(total_-toNext_)/static_cast<float>(total_));
            UI_ASSERT(progress >= 0. && progress <= 1.0001, "progress=" << progress);
            if(progress >= 1.) progress=1;

            int progressW=static_cast<int>(static_cast<float>(actProgRect.width())*progress);
            if(progressW <0) progressW=0;
            else if(progressW > progRect_.width()) progressW=progRect_.width();
            actProgRect.setWidth(progressW);
        }

        painter->setBrush(progBgBrush_);
        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->drawRect(progRect_);

        painter->fillRect(actProgRect,progBrush_);
    }

    if((hasInfo_ && showLastRefresh_) || !hasInfo_)
    {
        painter->setBrush(timeBgBrush_);
        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->drawRect(lastRect_);
        painter->setFont(fontTime_);
        painter->setPen((refreshAction_->isEnabled())?textPen_:disabledTextPen_);
        painter->drawText(lastRect_,Qt::AlignHCenter | Qt::AlignVCenter,"last: " + lastRefresh_);
     }
}


void ServerRefreshInfoWidget::adjustToolTip()
{
    QString txt;

    if(mode_ == ContMode)
    {
        Q_ASSERT(server_);
        Q_ASSERT(hasInfo_);
        txt=tr("Refresh period is too short! Manual refreshing is disabled for server <b>") +
                serverName_ +
                "</b><br>--------------------------------------------"+
                tr("<br><b>Refresh period:</b> ") + QString::number(total_) + "s" +
                " (base=" + QString::number(period_) + "s" + ",drifted=" + QString::number(total_-period_) +"s)";

    }
    else
    {
        if(currentComponent_ == ButtonComponent)
        {
            if(!server_)
            {
                txt=tr("Refresh <b>selected</b> server ");
            }
            else
            {
                txt=tr("Refresh server <b>") + serverName_ +"</b> ";
            }

            txt+=Viewer::formatShortCut(refreshAction_);
        }

        if(hasInfo_)
        {
            Q_ASSERT(server_);
            if(!txt.isEmpty())
                txt+="<br>--------------------------------------------";
            else
                txt+="<b>Server:</b> " + serverName_;

            txt+="<br><b>Last refresh:</b> " + lastRefresh_ +
                "<br><b>Next refresh:</b> " + nextRefresh_ +
                "<br><b>Refresh period:</b> " + QString::number(total_) + "s" +
                " (base=" + QString::number(period_) + "s" + ",drifted=" + QString::number(total_-period_) +"s)";

        }
        else if(server_)
        {
            if(!txt.isEmpty())
                txt+="<br>--------------------------------------------";
            else
                txt+="<b>Server:</b> " + serverName_;

            txt+="<br><b>Last refresh:</b> " + lastRefresh_ + "<br>" +
                 "Automatic refresh is disabled!";

        }
    }

    setToolTip(txt);
}

QString ServerRefreshInfoWidget::formatTime(int timeInSec) const
{
    int h=timeInSec/3600;
    int r=timeInSec%3600;
    int m=r/60;
    int s=r%60;

    QTime t(h,m,s);
    if(h > 0)
       return QString::number(h) + "h";
    else if(m > 2)
    {
        if(s >= 30)
            return QString::number(m+1) + "m";
        else
            return QString::number(m) + "m";
    }
    else if(m >= 1)
    {
        if(s >= 45)
            return "1:45m";
        else if(s >= 30)
            return "1:30m";
        else if(s >= 15)
            return "1:15m";
        else
           return "1m";
    }
    else if(s > 50)
       return "<1m";
    else if(s > 45)
       return "<50s";
    else if(s > 40)
        return "<45s";
    else if(s > 35)
       return "<40s";
    else if(s > 30)
       return "<35s";
    else if(s > 25)
       return "<30s";
    else if(s > 20)
        return "<25s";
    else if(s > 15)
       return "<20s";
    else if(s > 10)
       return "<15s";
    else if(s > 5)
        return "<10s";
    else
        return "<5s"; //return QString("%1s").arg(s, 2, 10, QChar('0'));

    return QString();
}


void ServerRefreshInfoWidget::printStatus() const
{
    UiLog().dbg()  << "  server=" << server_ << " action=" << refreshAction_->isEnabled() << " hasInfo=" << hasInfo_ <<
                      " mode=" << mode_ << " inRefresh=" << inRefresh_ << " timer="  << timer_->isActive() <<
                      " timeout=" << timer_->interval()/1000.  << "s";
}

ServerComActivityLine::ServerComActivityLine(QWidget *parent) :
    QWidget(parent),
    server_(0),
    font_(QFont()),
    fm_(font_)
{
    font_.setPointSize(font_.pointSize()-1);
    fm_=QFontMetrics(font_);

    int width_=200;
    int height_=fm_.height()+4+6;

    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(update()));

    timer_->setInterval(1000);
    timer_->start();

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);
}

void ServerComActivityLine::setServer(ServerHandler* server)
{
    server_=server;
}

void ServerComActivityLine::paintEvent(QPaintEvent*)
{
#if 0
    if(!server_)
        return;

    int currentRight=0;
    int offset=4;
    int yPadding=2;

    int period,total,drift,toNext;
    bool hasUpdate=server_->updateInfo(period,total,drift,toNext);
    bool hasDrift=(hasUpdate && drift > 0);
    int h=height()-2*yPadding;
    int progHeight=h;
    int driftHeight=0;


    if(hasDrift)
    {
        progHeight=fm_.height()+2;
        driftHeight=h-progHeight;
    }

    //progress rect - with the server name in it
    QRect progRect=QRect(offset,yPadding,
                       fm_.width("ABCDEABDCEABCD"),
                       progHeight);

    QString progText=fm_.elidedText(QString::fromStdString(server_->name()),
                               Qt::ElideRight,progRect.width());

    //drif rect
    QRect driftRect;
    if(hasDrift)
    {
        driftRect=progRect;
        driftRect.setY(yPadding+progHeight);
        driftRect.setHeight(driftHeight);
    }

   //toNext rect
   currentRight=progRect.x()+progRect.width()+offset;

   QString toNextText=QString::number(total) + "s";
   if(hasUpdate)
       toNextText=formatTime(toNext);

   QRect toNextRect=progRect;
   toNextRect.setX(currentRight);
   toNextRect.setWidth(fm_.wi

#endif


}



