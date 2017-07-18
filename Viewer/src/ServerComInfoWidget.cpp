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
QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(219,238,246));
QBrush ServerRefreshInfoWidget::timeBgBrush_(QColor(210,210,210));
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
    fastMode_(false),
    hasInfo_(false),
    inRefresh_(true),
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

    adjustGeometry();

    //timeTextLen_=qMax(fmTime_.width(" <9:59m left"),fmUpdate_.width("updating"));
    buttonRect_=QRect(1,1,height()-2,height()-2);
    buttonRadius2_=pow(buttonRect_.width()/2,2);

    setMouseTracking(true);

    std::vector<std::string> propVec;
    propVec.push_back("server.update.showUpdateCountdownArc");
    propVec.push_back("server.update.showUpdateCountdownText");
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

void ServerRefreshInfoWidget::notifyChange(VProperty* p)
{
    updateSettings();
}

void ServerRefreshInfoWidget::updateSettings()
{
    if(VProperty* p=prop_->find("server.update.showUpdateCountdownArc"))
    {
        showCountdownArc_=p->value().toBool();
    }
    if(VProperty* p=prop_->find("server.update.showUpdateCountdownText"))
    {
        showCountdownText_=p->value().toBool();
    }
    reloadAll();
}

void ServerRefreshInfoWidget::adjustGeometry()
{
    if(server_)
    {
        timeTextLen_=qMax(fmTime_.width(" <9:59m"),fmUpdate_.width("updating"));
        setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width(serverText_) +
                      timeTextLen_ +6);
    }
    else
    {
        timeTextLen_=0;
        setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width("AAAAA"));
    }
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
    fastMode_=false;
    inRefresh_=false;

    //Cache some data
    serverName_.clear();
    serverText_.clear();
    if(server_)
    {
        serverName_=QString::fromStdString(server_->name());
        serverText_=serverName_ + " ";      
    }

    //Adjust width
    adjustGeometry();

    //get info and rerender
    reloadAll();
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
        fastMode_=false;
        inRefresh_=false;

        //get info and rerender
        reloadAll();
    }
}

//While the server is being reloaded the refresh button is disabled
void ServerRefreshInfoWidget::notifyBeginServerClear(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    refreshAction_->setEnabled(false);
    update();
}

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
    if(!inRefresh_ || fastMode_)
    {
        reloadAll(); //get info and rerender
    }
}

//While the refresh is being executed the the refresh button is disabled. In fast mode we
//do not do this because the refresh button is continuosly disabled.
void ServerRefreshInfoWidget::notifyRefreshScheduled(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    inRefresh_=true;
    if(!fastMode_)
    {
        //refreshAction_->setEnabled(false);
        timer_->stop();

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        printStatus();
#endif

        update();
    }
}

void ServerRefreshInfoWidget::notifyRefreshFinished(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(server_ == server);
    if(!fastMode_)
    {
        //We keep the button disabled for 1 sec (the timer is stopped now!!)
        QTimer::singleShot(1000,this,SLOT(slotTimeOutRefreshFinished()));
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        printStatus();
#endif
    }
    else
    {
        inRefresh_=false;
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
        hasInfo_=false;
        fastMode_=false;
        timer_->stop();
    }
    else
    {
        int drift;
        QDateTime currentTime=QDateTime::currentDateTime();
        hasInfo_=server_->updateInfo(period_,total_,drift,toNext_);

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        UiLog().dbg() << " period=" << period_ << " total=" << total_ <<
                         " toNext=" << toNext_;
#endif

        //the server has an automatic update
        if(hasInfo_)
        {
            lastRefresh_=currentTime.addSecs(-total_+toNext_).time().toString();
            nextRefresh_=currentTime.addSecs(toNext_).time().toString();

            //See if we are in fastmode. In fastmode:
            // -the action is disabled
            // -there is a special tooltip
            // -the timer is stopped
            // -a warning colour is used to draw the border
            if(total_ < 10)
            {
                fastMode_=true;
                refreshAction_->setEnabled(false);
            }
            //Not in fastmode
            else
            {
                fastMode_=false;
                if(!inRefresh_)
                    refreshAction_->setEnabled(true);
            }
        }
        //the server's automatic automatic is switched off
        else
        {
            lastRefresh_=server_->lastRefresh().time().toString();
            fastMode_=false;
            if(!inRefresh_)
                refreshAction_->setEnabled(true);
        }

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
    if(!refreshAction_->isEnabled() || fastMode_ || !showCountdown())
    {
        timer_->stop();

    }
    else if(hasInfo_)
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
            timer_->setInterval(1*1000);

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
    if(!refreshAction_->isEnabled() || fastMode_)
        return;

    //We are in the button
    if(isInButton(event->pos()))
    {
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
//        UiLog().dbg() << "pressed";
#endif
        if(currentComponent_ != ButtonComponent)
        {

        }
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
            if(!showCountdown())
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }
            if(refreshAction_->isEnabled() && !fastMode_)
            {
                update(); //rerender
            }
        }
    }
    //We are in the text
    else if(isInText(event->pos()))
    {
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
//        UiLog().dbg() << "inText";
#endif
        //we came from the button
        if(currentComponent_ == ButtonComponent)
        {
            currentComponent_=TextComponent;
            if(!showCountdown())
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }
            if(refreshAction_->isEnabled() && !fastMode_)
            {
                update(); //rerender
            }
        }
        //we came from outside
        else if(currentComponent_ != TextComponent)
        {
            currentComponent_=TextComponent;
            if(!showCountdown())
            {
                fetchInfo();
            }
            else
            {
                adjustToolTip();
            }
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


void ServerRefreshInfoWidget::drawButton(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing,true);

    if(server_)
    {
        //The filled circle
        if(inRefresh_)
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
    int yPadding=5;
    int h=height()-2*yPadding;

    QRect serverRect=QRect(buttonRect_.center().x()+4,yPadding,
                       buttonRect_.width()/2-4+4+fm_.width(serverText_),
                       h);

    QRect serverTextRect=serverRect.adjusted(buttonRect_.width()/2-4+4,0,0,0);

    QString timeText;
    QRect   timeRect,progRect;
    if(hasInfo_) // && showCountdownText_)
    {
        if(fastMode_)
            timeText = "<" + QString::number(total_) + "s";
        else if(inRefresh_)
            timeText = "updating";
        else
            timeText =QChar(916) + QString("=") + QString::number(total_) + "s"; //formatTime(toNext_) + " left";

        timeRect = serverRect;
        timeRect.setX(serverRect.x()+serverRect.width());
        timeRect.setWidth(timeTextLen_);

        timeRect.setHeight(fmTime_.height()-2);
        progRect = timeRect;
        progRect.setY(timeRect.y()+timeRect.height());
        progRect.setHeight(serverRect.height()-timeRect.height());

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        UiLog().dbg() << "timeText=" << timeText;
#endif
    }

    //Start painting
    painter->setFont(font_);

    //Server rect
    painter->setBrush(Qt::NoBrush);
    painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
    painter->drawRect(serverRect);

    //Server text
    painter->setPen((refreshAction_->isEnabled())?textPen_:disabledTextPen_);
    painter->drawText(serverTextRect,Qt::AlignLeft | Qt::AlignVCenter,serverText_);

    //The time rects and texts
    if(hasInfo_) // && showCountdownText_)
    {
        //Time rect
        if(inRefresh_ && !fastMode_)
            painter->setFont(fontUpdate_);
        else
            painter->setFont(fontTime_);

         painter->setBrush(timeBgBrush_);
         painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
         painter->drawRect(timeRect);

        //Time text
        if(fastMode_ || inRefresh_)
            //painter.setPen(QColor(0,136,0));
            painter->setPen(refreshTextPen_);
        else
            painter->setPen((refreshAction_->isEnabled())?textPen_:disabledTextPen_);

        painter->drawText(timeRect,Qt::AlignHCenter | Qt::AlignVCenter,timeText);

        float progress=(static_cast<float>(total_-toNext_)/static_cast<float>(total_));
        UI_ASSERT(progress >= 0. && progress <= 1.0001, "progress=" << progress);
        if(progress >= 1.) progress=1;

        QRect actProgRect=progRect;
        int progressW=static_cast<int>(static_cast<float>(actProgRect.width())*progress);
        if(progressW <0) progressW=0;
        else if(progressW > progRect.width()) progressW=progRect.width();
        actProgRect.setWidth(progressW);

        painter->setBrush(progBgBrush_);
        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->drawRect(progRect);

        painter->fillRect(actProgRect,progBrush_);
    }
}

void ServerRefreshInfoWidget::paintEvent(QPaintEvent*)
{    
    QPainter painter(this);
    drawProgress(&painter);
    drawButton(&painter);
}

void ServerRefreshInfoWidget::adjustToolTip()
{
    QString txt;

    if(fastMode_)
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
                      " fastMode=" << fastMode_ << " inRefresh=" << inRefresh_ << " timer="  << timer_->isActive() <<
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



