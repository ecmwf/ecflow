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
    UiLoggable("ServerRefreshInfoWidget"),
    refreshAction_(refreshAction),
    server_(0),
    font_(QFont()),
    fontTime_(QFont()),
    fm_(QFont()),
    fmTime_(QFont()),
    bgBrush_(QColor(229,228,227)),
    borderPen_(QColor(167,167,167)),
    bgHoverBrush_(QColor(249,248,248)),
    borderHoverPen_(QColor(160,160,160)),
    arcPen_(QColor(45,200,29),2),
    textPen_(QColor(80,80,80)),
    currentComponent_(NoComponent),
    prop_(0),
    showCountdown_(true),
    fastMode_(false),
    hasInfo_(false),
    inRefresh_(true),
    total_(-1),
    period_(-1),
    toNext_(-1)
{
    Q_ASSERT(refreshAction_);

    //The icon for the round refresh button
    icon_=QIcon(QPixmap(":/viewer/reload_black.svg"));

    font_=QFont();
    font_.setPointSize(font_.pointSize()-1);
    fm_=QFontMetrics(font_);

    fontTime_=QFont();
    fontTime_.setPointSize(fontTime_.pointSize()-2);
    fmTime_=QFontMetrics(fontTime_);

    int width_=200;
    int height_=fm_.height()+10;

    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(slotTimeOut()));

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);

    buttonRect_=QRect(1,1,height()-2,height()-2);
    buttonRadius2_=pow(buttonRect_.width()/2,2);

    setMouseTracking(true);

    std::vector<std::string> propVec;
    propVec.push_back("server.update.showUpdateCountdown");
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
    if(VProperty* p=prop_->find("server.update.showUpdateCountdown"))
    {
        showCountdown_=p->value().toBool();
        slotTimeOut();
    }
}

void ServerRefreshInfoWidget::setServer(ServerHandler* server)
{
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

    //Adjust width
    if(server_)
    {
        QString serverText=QString::fromStdString(server_->name() + " ");
        setFixedWidth(buttonRect_.x()+buttonRect_.width()+fm_.width(serverText) + fmTime_.width(" <22m") +6);
    }

    //get info and rerender
    slotTimeOut();
}

//-------------------------------
// Server observer notifications
//-------------------------------

void ServerRefreshInfoWidget::notifyServerDelete(ServerHandler* server)
{
    Q_ASSERT(server_ == server);
    if(server_ == server)
    {
        server_->removeServerObserver(this);
        server_->removeServerComObserver(this);
        server_=0;
        hasInfo_=false;
        fastMode_=false;
        inRefresh_=false;

        //get info and rerender
        slotTimeOut();
    }
}

//While the server is being reloaded the refresh button is disabled
void ServerRefreshInfoWidget::notifyBeginServerClear(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    refreshAction_->setEnabled(false);
    update();
}

void ServerRefreshInfoWidget::notifyEndServerScan(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    refreshAction_->setEnabled(true);
    slotTimeOut();
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
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    slotTimeOut(); //get info and rerender
}

void ServerRefreshInfoWidget::notifyRefreshTimerStopped(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    slotTimeOut(); //get info and rerender
}

void ServerRefreshInfoWidget::notifyRefreshTimerChanged(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
    printStatus();
#endif
    Q_ASSERT(server_ == server);
    if(!inRefresh_ || fastMode_)
    {
        slotTimeOut(); //get info and rerender
    }
}

//While the refresh is being executed the the refresh button is disabled. In fast mode we
//do not do this because the refresh button is continuosly disabled.
void ServerRefreshInfoWidget::notifyRefreshScheduled(ServerHandler* server)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    inRefresh_=true;
    if(!fastMode_)
    {
        refreshAction_->setEnabled(false);
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
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
#endif
    Q_ASSERT(server_ == server);
    inRefresh_=false;
    if(!fastMode_)
    {
        //We keep the button disabled for 1.5 sec (the timer is stopped now!!)
        QTimer::singleShot(1500,this,SLOT(slotTimeOut()));
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        printStatus();
#endif
    }
}

void ServerRefreshInfoWidget::slotTimeOut()
{
    fetchInfo(); //get info
    update(); //renrender
}

void ServerRefreshInfoWidget::fetchInfo()
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiFunctionLog fclog(this,BOOST_CURRENT_FUNCTION);
    printStatus();
#endif

    if(!server_)
    {
        hasInfo_=false;
        fastMode_=false;
        adjustTimer(0); //timer must be stopped
    }
    else
    {
        int drift;
        QDateTime currentTime=QDateTime::currentDateTime();
        hasInfo_=server_->updateInfo(period_,total_,drift,toNext_);

#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
        UiLog().dbg() << "fetchInfo -- > period=" << period_ << " total=" << total_ <<
                         " toNext=" << toNext_;
#endif
        if(hasInfo_)
        {
            lastRefresh_=currentTime.addSecs(-total_+toNext_).time().toString();
            nextRefresh_=currentTime.addSecs(toNext_).time().toString();
            if(total_ < 10)
            {
                fastMode_=true;
                refreshAction_->setEnabled(false);
            }
            else
            {
                fastMode_=false;
                if(!inRefresh_)
                    refreshAction_->setEnabled(true);
            }
        }
        else
        {
            lastRefresh_=server_->lastRefresh().time().toString();
            fastMode_=false;
            if(!inRefresh_)
                refreshAction_->setEnabled(true);
        }

        if(currentComponent_ == TextComponent)
            adjustToolTip();

        adjustTimer(toNext_);
    }
}

//Adjust timer interval
void ServerRefreshInfoWidget::adjustTimer(int toNext)
{
    if(!refreshAction_->isEnabled())
    {
        timer_->stop();
    }

    if(hasInfo_)
    {
        if(fastMode_)
        {
            timer_->stop();
            return;
        }

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
            adjustToolTip();

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
            adjustToolTip();
            if(refreshAction_->isEnabled() && !fastMode_)
            {
                update(); //rerender
            }
        }
        //we came from outside
        else if(currentComponent_ != TextComponent)
        {
            currentComponent_=TextComponent;
            adjustToolTip();
        }
    }
}

void ServerRefreshInfoWidget::leaveEvent(QEvent*)
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    //UiLog().dbg() << "leave";
#endif
    currentComponent_=NoComponent;
    update(); //rerender
}

void ServerRefreshInfoWidget::paintEvent(QPaintEvent*)
{    
    //There is no server defined. It a disabled state!
    if(!server_)
    {
        QPainter painter(this);
        painter.setFont(font_);

        painter.setRenderHint(QPainter::Antialiasing,true);

        painter.setBrush(bgBrush_);
        painter.setPen(borderPen_);
        painter.drawEllipse(buttonRect_);

        QRect r=buttonRect_; //.adjusted(1,1,-1,-1);
        QRect r1=r.adjusted(2,2,-2,-2);
        QRect r2=r1.adjusted(3,3,-3,-3);
        QPixmap pix=icon_.pixmap(QSize(r2.width(),r2.width()),QIcon::Disabled);
        painter.drawPixmap(r2,pix);

        hasInfo_=false;
        //adjustTimer(0); //timer must be stopped
    }
    else
    {
        //Define server rect
        int yPadding=5;
        int h=height()-2*yPadding;

        QString serverText=QString::fromStdString(server_->name() + " ");

        QRect serverRect=QRect(buttonRect_.center().x()+4,yPadding,
                           buttonRect_.width()/2-4+4+fm_.width(serverText),
                           h);

        QRect serverTextRect=serverRect.adjusted(buttonRect_.width()/2-4+4,0,0,0);

        QString timeText;
        QRect   timeRect;
        if(hasInfo_ && showCountdown_)
        {
            timeText = (inRefresh_)?"now":formatTime(toNext_);
            timeRect = serverRect;
            timeRect.setX(serverRect.x()+serverRect.width());
            timeRect.setWidth(fmTime_.width(" <22m"));
        }

        //Start painting
        QPainter painter(this);
        painter.setFont(font_);

        //Server rect
        painter.setBrush(Qt::NoBrush);
        painter.setPen(borderPen_);
        painter.drawRect(serverRect);

        //Server text
        painter.setPen(textPen_);
        painter.drawText(serverTextRect,Qt::AlignLeft | Qt::AlignVCenter,serverText);

        //The time rects and texts
        if(hasInfo_ && showCountdown_)
        {
            //Time rect
            painter.setFont(fontTime_);

            painter.setBrush(QColor(210,210,210));
            painter.setPen(borderPen_);
            painter.drawRect(timeRect);

            //Time text
            painter.setPen(textPen_);
            painter.drawText(timeRect,Qt::AlignHCenter | Qt::AlignVCenter,timeText);
        }

        painter.setRenderHint(QPainter::Antialiasing,true);

        //The filled circle
        painter.setBrush((currentComponent_ == ButtonComponent)?bgHoverBrush_:bgBrush_);
        painter.setPen((currentComponent_ == ButtonComponent)?borderHoverPen_:borderPen_);
        painter.drawEllipse(buttonRect_);

        //The countdown arc
        QRect r1=buttonRect_.adjusted(2,2,-2,-2);
        if(showCountdown_ && refreshAction_->isEnabled() && hasInfo_ && !fastMode_ && !inRefresh_)
        {
            painter.setBrush(Qt::NoBrush);

            painter.setPen(arcPen_);
            float progress=(static_cast<float>(total_-toNext_)/static_cast<float>(total_));
            UI_ASSERT(progress >= 0. && progress <= 1.0001, "progress=" << progress);
            if(progress >= 1.) progress=1;

            int span=static_cast<int>(360.*16.*progress);
            if(span> 360*16) span=360*16;

            //UiLog().dbg() << "span=" << span << " progress=" << progress;

            QRect r=buttonRect_; //.adjusted(1,1,-1,-1);

            painter.drawArc(buttonRect_,90*16,-span);

            painter.setPen(Qt::NoPen);
            painter.setBrush((currentComponent_ == ButtonComponent)?bgHoverBrush_:bgBrush_);
            painter.drawEllipse(r1);
        }

        //The reload icon
        QRect r2=r1.adjusted(3,3,-3,-3);
        QPixmap pix=icon_.pixmap(QSize(r2.width(),r2.width()),
                                 refreshAction_->isEnabled()? QIcon::Normal: QIcon::Disabled);
        painter.drawPixmap(r2,pix);
    }
}

void ServerRefreshInfoWidget::adjustToolTip()
{
    QString txt;
    if(currentComponent_ == ButtonComponent)
    {
        if(!server_)
        {
            txt=tr("Refresh <b>selected</b> server ");
        }
        else
        {
            txt=tr("Refresh server <b>") + QString::fromStdString(server_->name()) +
                  tr("</b> ");
        }

        txt+=Viewer::formatShortCut(refreshAction_);
    }

    if(hasInfo_)
    {
        Q_ASSERT(server_);
        if(!txt.isEmpty())
            txt+="<br>--------------------------------------------";
        else
            txt+="<b>Server:</b> " + QString::fromStdString(server_->name());

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
            txt+="<b>Server:</b> " + QString::fromStdString(server_->name());

        txt+="<br><b>Last refresh:</b> " + lastRefresh_ + "<br>" +
             "Automatic refresh is disabled!";

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
    UiLog().dbg()  << "  action=" << refreshAction_->isEnabled() << " hasInfo=" << hasInfo_ <<
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



