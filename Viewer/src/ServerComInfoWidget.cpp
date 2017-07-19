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
QBrush ServerRefreshInfoWidget::serverBgBrush_(QColor(229,228,227));
QPen ServerRefreshInfoWidget::borderPen_(QColor(167,167,167));
QPen ServerRefreshInfoWidget::disabledBorderPen_(QColor(182,182,182));
QBrush ServerRefreshInfoWidget::buttonBgHoverBrush_(QColor(249,248,248));
QPen ServerRefreshInfoWidget::buttonHoverPen_(QColor(160,160,160));
//QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(219,238,246));
//QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(213,222,227));
QBrush ServerRefreshInfoWidget::buttonBgRefreshBrush_(QColor(214,227,213));

//QBrush ServerRefreshInfoWidget::timeBgBrush_(QColor(214,214,214));
QBrush ServerRefreshInfoWidget::periodBgBrush_(QColor(238,238,238));
//QBrush ServerRefreshInfoWidget::progBrush_(QColor(38,181,245));
QBrush ServerRefreshInfoWidget::progBrush_(QColor(21,148,21));
QBrush ServerRefreshInfoWidget::progBgBrush_(QColor(255,255,255));
//QPen ServerRefreshInfoWidget::buttonRefreshPen_(QColor(38,181,245),2);
QBrush ServerRefreshInfoWidget::lastBgBrush_(QColor(238,238,238));
QPen ServerRefreshInfoWidget::buttonRefreshPen_(QColor(79,179,100),2);
QPen ServerRefreshInfoWidget::serverTextPen_(QColor(80,80,80));
QPen ServerRefreshInfoWidget::periodTextPen_(QColor(45,45,45));
QPen ServerRefreshInfoWidget::driftTextPen_(QColor(120,120,120));
QPen ServerRefreshInfoWidget::lastTextPen_(QColor(45,45,45));
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
    fontServer_(QFont()),
    fontPeriod_(QFont()),
    fontLast_(QFont()),
    fmServer_(QFont()),
    fmPeriod_(QFont()),
    fmLast_(QFont()),
    periodTextSize_(0),
    periodTextSizeMin_(0),
    currentComponent_(NoComponent),
    prop_(0),
    mode_(NoMode),
    noBlinkLimit_(15),
    hasInfo_(false),
    inRefresh_(true),
    userInitiatedRefresh_(false),
    showLastAutoRefresh_(true),
    total_(-1),
    period_(-1),
    toNext_(-1),
    drift_(-1)
{
    Q_ASSERT(refreshAction_);

    //The icon for the round refresh button
    if(!icon_)
        icon_=new QIcon(QPixmap(":/viewer/reload_black.svg"));

    //Init fonts
    fontServer_=QFont();
    fontServer_.setPointSize(fontServer_.pointSize()-1);
    fontServer_.setBold(true);
    fmServer_=QFontMetrics(fontServer_);

    fontPeriod_=QFont();
    fontPeriod_.setPointSize(fontPeriod_.pointSize()-2);
    fmPeriod_=QFontMetrics(fontPeriod_);

    fontLast_=QFont();
    fontLast_.setPointSize(fontLast_.pointSize()-2);
    fmLast_=QFontMetrics(fontLast_);

    int w=200;
    int h=fmServer_.height()+6;

    //timer
    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(slotTimeOut()));

    //set size
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(w,h);
    adjustGeometry(false);

    buttonRect_=QRect(1,1,height()-2,height()-2);
    buttonRadius2_=pow(buttonRect_.width()/2,2);

    //we need this for the mousemove event
    setMouseTracking(true);

    //properties to use
    std::vector<std::string> propVec;
    propVec.push_back("server.update.blinkUpdateButtonLimit");
    propVec.push_back("server.update.showLastRefreshTimeInAutoMode");
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
    bool fetched=false;
    bool changed=false;
    if(VProperty* p=prop_->find("server.update.showLastRefreshTimeInAutoMode"))
    {
        bool v=p->value().toBool();
        if(showLastAutoRefresh_!= v)
        {
            showLastAutoRefresh_=v;
            adjustGeometry(true);
            fetched=true;
            changed=true;
        }
    }

    if(VProperty* p=prop_->find("server.update.blinkUpdateButtonLimit"))
    {
        int v=p->value().toInt();
        if(noBlinkLimit_ != v)
        {
            noBlinkLimit_=v;
            changed=true;
        }
    }

    if(changed)
    {
        if(!fetched)
        {
            fetchInfo();
        }
        update();
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
    inRefresh_=false;
    userInitiatedRefresh_=false;
    mode_=NoMode;

    //Cache some data
    serverName_.clear();
    serverText_.clear();
    if(server_)
    {
        serverName_=QString::fromStdString(server_->name());
        serverText_=serverName_ + " ";      
    }

    periodText_.clear();
    driftText_.clear();
    periodTextSizeMin_=0;
    periodTextSizeMin_=0;
    periodTextWidth_=0;
    lastTextWidth_=0;

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
        periodText_.clear();
        driftText_.clear();
        periodTextSizeMin_=0;
        periodTextSizeMin_=0;
        periodTextWidth_=0;
        lastTextWidth_=0;

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
    periodText_.clear();
    driftText_.clear();
    periodTextSizeMin_=0;
    periodTextSizeMin_=0;
    periodTextWidth_=0;
    lastTextWidth_=0;

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
    //NoMode
    if(!inRefresh_ || mode_==NoMode)
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
    if(mode_ == NormalMode)
    {        
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

    if((mode_ == NormalMode) && elapsed < 450)
    {
        Q_ASSERT(500-elapsed > 0);
        //We keep the button in inRefresh state for 0.5 sec (the timer is stopped now!!)
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
        QDateTime currentTime=QDateTime::currentDateTime();
        bool v=server_->updateInfo(period_,total_,drift_,toNext_);

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

            mode_=NormalMode;
            if(!inRefresh_)
                refreshAction_->setEnabled(true);
        }
        //the server's automatic refresh is switched off
        else
        {
            lastRefresh_=server_->lastRefresh().time().toString();
            mode_=ManualMode;
            if(!inRefresh_)
                refreshAction_->setEnabled(true);
        }

        //Determine period text
        determinePeriodText();

        if(geoUpdateNeeded || periodTextSizeAboutToChange())
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
    if(!refreshAction_->isEnabled() || mode_ == NoMode || mode_ == ManualMode)
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


QString ServerRefreshInfoWidget::formatPeriodTime(int timeInSec) const
{
    int h=timeInSec/3600;
    int r=timeInSec%3600;
    int m=r/60;
    int s=r%60;

    QTime t(h,m,s);
    if(h > 0)
       return QString::number(h) +  QString("%1s").arg(m, 2, 10, QChar('0')) + "h";
    else if(m > 5)
       return QString::number(m) +  QString("%1s").arg(s, 2, 10, QChar('0')) + "s";
    else if(m> 0)
       return QString::number(m*60+s) + "s";
    else
       return QString::number(s) + "s";

    return QString();
}

void ServerRefreshInfoWidget::determinePeriodText()
{
    periodText_.clear();
    driftText_.clear();
    if(hasInfo_)
    {
        //Unicode 916=Greek capital delta
        periodText_=QChar(916) + QString("=") + formatPeriodTime(total_);
        if(drift_ > 0)
        {
            driftText_=" d=" + formatPeriodTime(total_-period_);
        }
    }
}

QString ServerRefreshInfoWidget::fullPeriodText() const
{
    return periodText_+driftText_;
}

int ServerRefreshInfoWidget::determinePeriodTextSizeMin() const
{
    int minv=QString(" D=300s ").size();
    if(hasInfo_ && drift_ > 0)
    {
        minv=QString(" D=99s d=99s").size();
    }
    return minv;
}

//Indicate if the full period text's size will change in such a way that the
//geometry needs to be adjusted
bool ServerRefreshInfoWidget::periodTextSizeAboutToChange() const
{
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UI_FUNCTION_LOG
#endif

    int mval=determinePeriodTextSizeMin();

    QString pt=fullPeriodText();
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiLog().dbg() << " full=" << pt << " pt.size=" << pt.size() <<
               " mval=" << mval << " periodTextSizeMin=" <<  periodTextSizeMin_ <<
               " periodTextSize "  <<  periodTextSize_;
#endif
    bool changed=false;
    if(periodTextSizeMin_ == mval)
    {
        if(pt.size() > periodTextSize_)
        {
            changed=pt.size() > periodTextSizeMin_;
        }
        else if(pt.size() < periodTextSize_)
        {
            changed=periodTextSize_ >= periodTextSizeMin_;
        }
    }
    else
    {
        changed=true;
    }
#ifdef _UI_SERVERCOMINFOWIDGET_DEBUG
    UiLog().dbg() << " changed=" << changed;
#endif
    return changed;
}

void ServerRefreshInfoWidget::adjustGeometry(bool doFetchInfo)
{
    if(server_)
    {
        //timeTextLen_=qMax(fmTime_.width(" <9:59m"),fmUpdate_.width("updating"));

        //Define server rect
        int yPadding=5;
        int h=height()-2*yPadding;
        int currentRight=0;

        serverRect_=QRect(buttonRect_.center().x()+4,yPadding,
                          buttonRect_.width()/2-4+4+fmServer_.width(serverText_),
                          h);
        currentRight=serverRect_.x()+serverRect_.width();

        if(doFetchInfo)
            fetchInfo();

        //Determine the minimum size for the period text
        periodTextSizeMin_=determinePeriodTextSizeMin();

        //Compute physical width of the period text
        QString pt=fullPeriodText();
        periodTextSize_=pt.size();
        if(pt.size() <= periodTextSizeMin_)
        {
            periodTextWidth_=fmPeriod_.width(QString().fill('A',periodTextSizeMin_));
        }
        else
        {
            periodTextWidth_=fmPeriod_.width(pt);
        }

        periodRect_=QRect();
        progRect_=QRect();
        lastRect_=QRect();

        if(hasInfo_)
        {
            periodRect_ = serverRect_;
            periodRect_.setX(serverRect_.x()+serverRect_.width());
            periodRect_.setWidth(periodTextWidth_);
            periodRect_.setHeight(fmPeriod_.height()-2);

            currentRight+=periodRect_.width();

            progRect_ = periodRect_;
            progRect_.setY(periodRect_.y()+periodRect_.height());
            progRect_.setHeight(serverRect_.height()-periodRect_.height());
        }

        lastTextWidth_=0;
        if((hasInfo_ && showLastAutoRefresh_) || !hasInfo_)
        {
            //Compute physical width of the last refresh text
            lastTextWidth_=fmLast_.width(" last: 22:22:22 ");

            lastRect_ = QRect(currentRight,serverRect_.y(),lastTextWidth_,h);
            currentRight+=lastRect_.width()+6;
        }
        else
           currentRight+=6;

        setFixedWidth(currentRight);
    }
    else
    {
        periodTextSize_=0;
        periodTextWidth_=0;
        lastTextWidth_=0;
        setFixedWidth(buttonRect_.x()+buttonRect_.width()+fmServer_.width("AAAAA"));
    }
}


void ServerRefreshInfoWidget::drawButton(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing,true);

    if(server_)
    {
        //blink
        if(inRefresh_ &&
           ((mode_ == NormalMode && period_ >= noBlinkLimit_) ||
             userInitiatedRefresh_ || mode_ == ManualMode ))
        {
            painter->setBrush(buttonBgRefreshBrush_);
            painter->setPen(buttonRefreshPen_);
        }
        else
        {
            painter->setBrush((currentComponent_ == ButtonComponent)?buttonBgHoverBrush_:serverBgBrush_);

            if(!refreshAction_->isEnabled())
                painter->setPen(disabledBorderPen_);
            else
                painter->setPen((currentComponent_ == ButtonComponent)?buttonHoverPen_:borderPen_);
        }
    }
    else
    {
        painter->setBrush(serverBgBrush_);
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

    painter->setBrush(Qt::NoBrush);
    painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
    painter->drawRect(serverRect_);

    //Server text
    QRect serverTextRect=serverRect_.adjusted(buttonRect_.width()/2-4+4,0,0,0);
    painter->setFont(fontServer_);
    painter->setPen((refreshAction_->isEnabled())?serverTextPen_:disabledTextPen_);
    painter->drawText(serverTextRect,Qt::AlignLeft | Qt::AlignVCenter,serverText_);

    //The time rects and texts
    if(hasInfo_)
    {
        //backgrounds
        painter->setPen(Qt::NoPen);
        painter->setBrush(periodBgBrush_);
        painter->drawRect(periodRect_);
        painter->setBrush(progBgBrush_);
        painter->drawRect(progRect_);

        //border
        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(periodRect_.adjusted(0,0,0,progRect_.height()));

        painter->setFont(fontPeriod_);

        //period + drift text
        if(drift_ > 0)
        {
            int tw=fmPeriod_.width(fullPeriodText());
            int w=periodRect_.width();
            QRect rr=periodRect_.adjusted((w-tw)/2,0,-(w-tw)/2,0);

            painter->setPen(periodTextPen_);
            painter->drawText(rr,Qt::AlignLeft | Qt::AlignVCenter,periodText_);
            painter->setPen(driftTextPen_);
            painter->drawText(rr,Qt::AlignRight| Qt::AlignVCenter,driftText_);
        }
        else
        {
            painter->setPen(periodTextPen_);
            painter->drawText(periodRect_,Qt::AlignHCenter | Qt::AlignVCenter,periodText_);
        }

        //Progress
        float progress;
        QRect actProgRect=progRect_;

        if(total_ < 2 || inRefresh_)
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

        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->drawLine(progRect_.topLeft(),progRect_.topRight());

        painter->fillRect(actProgRect,progBrush_);
    }

    //last refresh time
    if((hasInfo_ && showLastAutoRefresh_) || !hasInfo_)
    {
        painter->setBrush(lastBgBrush_);
        painter->setPen((refreshAction_->isEnabled())?borderPen_:disabledBorderPen_);
        painter->drawRect(lastRect_);

        painter->setFont(fontLast_);
        painter->setPen(lastTextPen_);
        painter->drawText(lastRect_,Qt::AlignHCenter | Qt::AlignVCenter,"last: " + lastRefresh_);
     }
}

void ServerRefreshInfoWidget::adjustToolTip()
{
    QString txt;

    /*
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
    */
    if(1)
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

#if 0
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
#endif



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



