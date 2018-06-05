//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LogLoadWidget.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "LogModel.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "ui_LogLoadWidget.h"

//=======================================================
//
// LogLoadWidget
//
//=======================================================

LogLoadWidget::LogLoadWidget(QWidget *parent) : ui_(new Ui::LogLoadWidget)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    //Chart views
    viewHandler_=new LogRequestViewHandler(this);
    for(int i=0; i < viewHandler_->views().count(); i++)
    {
        ui_->viewTab->addTab(viewHandler_->views()[i]," h");
    }

    connect(ui_->viewTab,SIGNAL(currentChanged(int)),
            this,SLOT(currentTabChanged(int)));

    ui_->viewTab->setTabText(0,tr("Totals"));
    ui_->viewTab->setTabText(1,tr("Commands"));
    ui_->viewTab->setTabText(2,tr("Stats"));

    ui_->viewTab->setCurrentIndex(0);

    //Cornerbutton for tab
    //QWidget *cornerW=new QWidget(this);
    //QHBoxLayout *cornerHb=new QHBoxLayout(cornerW);
    //cornerHb->setContentsMargins(0,0,0,0);

    //QToolButton* showFullTb=new QToolButton(this);
    //showFullTb->setText(tr("Full period"));
    //cornerHb->addWidget(showFullTb);

    connect(ui_->showFullTb,SIGNAL(clicked()),
            viewHandler_,SLOT(showFullRange()));

    //Temporal resolution combo box
    //resCombo_=new QComboBox(this);
    ui_->resCombo->addItem("seconds",0);
    ui_->resCombo->addItem("minutes",1);
    ui_->resCombo->addItem("hours",2);

    connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(resolutionChanged(int)));

    //cornerHb->addWidget(new QLabel("Resolution:",this));
    //cornerHb->addWidget(ui_->resCombo_);

    //ui_->viewTab->setCornerWidget(cornerW);

    //-----------------------------------------------
    // View + model to display/select suites
    //-----------------------------------------------

    suiteModel_=new LogLoadRequestModel(tr("Suite"),this);
    //suiteSortModel_=new QSortFilterProxyModel(this);
    suiteSortModel_=new LogLoadRequestSortModel(this);
    suiteSortModel_->setSourceModel(suiteModel_);
    //suiteSortModel_->setSortRole(Qt::UserRole);
    suiteSortModel_->setDynamicSortFilter(true);

    ui_->suiteTree->setRootIsDecorated(false);
    ui_->suiteTree->setAllColumnsShowFocus(true);
    ui_->suiteTree->setUniformRowHeights(true);
    ui_->suiteTree->setSortingEnabled(true);
    ui_->suiteTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->suiteTree->setModel(suiteSortModel_);

    connect(suiteModel_,SIGNAL(checkStateChanged(int,bool)),
            viewHandler_,SLOT(addRemoveSuite(int,bool)));

    connect(viewHandler_,SIGNAL(suitePlotStateChanged(int,bool,QColor)),
            suiteModel_,SLOT(updateItem(int,bool,QColor)));

    connect(ui_->unselectSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(unselectAll()));

    connect(ui_->selectFourSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(selectFirstFourItems()));

    //-----------------------------------------------
    // View + model to display/select child commands
    //-----------------------------------------------

    childReqModel_=new LogLoadRequestModel("Command",this);
    childReqSortModel_=new LogLoadRequestSortModel(this);
    childReqSortModel_->setSourceModel(childReqModel_);
    //childReqSortModel_->setSortRole(Qt::UserRole);
    childReqSortModel_->setDynamicSortFilter(true);

    ui_->childTree->setRootIsDecorated(false);
    ui_->childTree->setAllColumnsShowFocus(true);
    ui_->childTree->setUniformRowHeights(true);
    //ui_->childTree->setSortingEnabled(true);
    ui_->childTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->childTree->setModel(childReqSortModel_);


    connect(childReqModel_,SIGNAL(checkStateChanged(int,bool)),
            viewHandler_,SLOT(addRemoveChildReq(int,bool)));

    connect(viewHandler_,SIGNAL(childPlotStateChanged(int,bool,QColor)),
            childReqModel_,SLOT(updateItem(int,bool,QColor)));

#if 0
    connect(ui_->unselectSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(unselectAllSuites()));

    connect(ui_->selectFourSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(selectFirstFourSuites()));
#endif

    //-----------------------------------------------
    // View + model to display/select user commands
    //-----------------------------------------------

    userReqModel_=new LogLoadRequestModel("Command",this);
    userReqSortModel_=new LogLoadRequestSortModel(this);
    userReqSortModel_->setSourceModel(userReqModel_);
    //userReqSortModel_->setSortRole(Qt::UserRole);
    userReqSortModel_->setDynamicSortFilter(true);

    ui_->userTree->setRootIsDecorated(false);
    ui_->userTree->setAllColumnsShowFocus(true);
    ui_->userTree->setUniformRowHeights(true);
    ui_->userTree->setSortingEnabled(true);
    ui_->userTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->userTree->setModel(userReqSortModel_);


    connect(userReqModel_,SIGNAL(checkStateChanged(int,bool)),
            viewHandler_,SLOT(addRemoveUserReq(int,bool)));

    connect(viewHandler_,SIGNAL(userPlotStateChanged(int,bool,QColor)),
            userReqModel_,SLOT(updateItem(int,bool,QColor)));

#if 0
    connect(ui_->unselectSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(unselectAllSuites()));

    connect(ui_->selectFourSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(selectFirstFourSuites()));
#endif

    //Log contents
    logModel_=new LogModel(this);

    ui_->logView->setProperty("log","1");
    ui_->logView->setProperty("log","1");
    ui_->logView->setRootIsDecorated(false);
    ui_->logView->setLogModel(logModel_);
    ui_->logView->setUniformRowHeights(true);
    ui_->logView->setAlternatingRowColors(false);
    ui_->logView->setItemDelegate(new LogDelegate(this));
    ui_->logView->setContextMenuPolicy(Qt::ActionsContextMenu);

    //make the horizontal scrollbar work
    ui_->logView->header()->setStretchLastSection(false);
    ui_->logView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    //Define context menu
    //ui_->logView->addAction(actionCopyEntry_);
    //ui_->logView->addAction(actionCopyRow_);

    //Connect the views to the log model
    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            logModel_,SLOT(setPeriod(qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            this,SLOT(periodChanged(qint64,qint64)));

    connect(viewHandler_, SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
            logModel_,SLOT(setHighlightPeriod(qint64,qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            logModel_,SLOT(resetPeriod()));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            this,SLOT(periodWasReset()));

    //Scan label
    QColor bg(50,52,58);
    ui_->scanLabel->setStyleSheet("QLabel{background: " + bg.name() + ";}");

    QFont font;
    QFontMetrics fm(font);
    int w=fm.width("AAAAAverylongsuitename");
    QList<int> sizeLst=ui_->splitter->sizes();
    if(sizeLst.count()==3)
    {
        sizeLst[2]=w;
        sizeLst[0]=(width()-w)/2;
        sizeLst[1]=(width()-w)/2;
        ui_->splitter->setSizes(sizeLst);
    }

    //logInfo label
    ui_->logInfoLabel->setProperty("fileInfo","1");
    ui_->logInfoLabel->setWordWrap(true);
    ui_->logInfoLabel->setMargin(2);
    ui_->logInfoLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->logInfoLabel->setAutoFillBackground(true);
    ui_->logInfoLabel->setFrameShape(QFrame::StyledPanel);
    ui_->logInfoLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);


    //Charts
    connect(viewHandler_,SIGNAL(scanDataChanged(QString)),
            ui_->scanLabel,SLOT(setText(QString)));

    ui_->timeWidget->setStyleSheet("#timeWidget{background-color: rgb(212,212,212);}");

}

LogLoadWidget::~LogLoadWidget()
{
}

void LogLoadWidget::clear()
{
    ui_->messageLabel->clear();
    ui_->messageLabel->hide();

    ui_->logInfoLabel->setText(QString());
    viewHandler_->clear();
    suiteModel_->clearData();
    childReqModel_->clearData();
    userReqModel_->clearData();
    logModel_->clearData();
    logFile_.clear();
    serverName_.clear();
    host_.clear();
    port_.clear();
    ui_->startTe->clear();
    ui_->endTe->clear();

    setAllVisible(false);
}

void LogLoadWidget::updateInfoLabel()
{
    QColor col(39,49,101);
    QString txt=Viewer::formatBoldText("Log file: ",col) + logFile_;
    txt+=Viewer::formatBoldText(" Server: ",col) + serverName_ +
         Viewer::formatBoldText(" Host: ",col) + host_ +
         Viewer::formatBoldText(" Port: ",col) + port_;


    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    txt+=Viewer::formatBoldText(" Full period: ",col) +
            startDt.toString("yyyy-MM-dd hh:mm:ss") + Viewer::formatBoldText(" to ",col) +
            endDt.toString("yyyy-MM-dd hh:mm:ss");

    int maxNum=viewHandler_->data()->maxNumOfRows();
    int num=viewHandler_->data()->numOfRows();
    if(maxNum != 0 && num == abs(maxNum))
    {
        txt+=Viewer::formatBoldText(" Log entries: ",col) +
           "last " + QString::number(abs(maxNum)) + " rows read (maximum reached)";
    }

    ui_->logInfoLabel->setText(txt);
}

void LogLoadWidget::setAllVisible(bool b)
{
    ui_->viewTab->setVisible(b);
    setSuiteControlVisible(b);
    setChildControlVisible(b);
    setUserControlVisible(b);
    ui_->scanLabel->setVisible(b);
    ui_->logView->setVisible(b);   
}

void LogLoadWidget::setSuiteControlVisible(bool b)
{
    ui_->suiteTree->setVisible(b);
    ui_->unselectSuitesTb->setVisible(b);
    ui_->selectFourSuitesTb->setVisible(b);
}

void LogLoadWidget::setChildControlVisible(bool b)
{
    //ui_->childLabel->setVisible(b);
    ui_->childTree->setVisible(b);
}

void LogLoadWidget::setUserControlVisible(bool b)
{
    //ui_->userLabel->setVisible(b);
    ui_->userTree->setVisible(b);
}

void LogLoadWidget::load(QString logFile,int numOfRows)
{
    load("","","",logFile,numOfRows);
}

void LogLoadWidget::load(QString serverName, QString host, QString port, QString logFile,int numOfRows)
{
    clear();

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;

    updateInfoLabel();

    QFileInfo fInfo(logFile);
    if(!fInfo.exists())
    {
        ui_->messageLabel->showError("The specified log file does not exist!");
        return;
    }

    if(!fInfo.isReadable())
    {
        ui_->messageLabel->showError("The specified log file is not readable!");
        return;
    }

    if(!fInfo.isFile())
    {
        ui_->messageLabel->showError("The specified log file is not a file!");
        return;
    }

    setAllVisible(true);

    //view_->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //std::string fileName="/home/graphics/cgr/ecflow_dev/vsms1.ecf.log";

    try
    {
        viewHandler_->load(logFile_.toStdString(),numOfRows);
    }
    catch(std::runtime_error e)
    {
        ui_->messageLabel->showError(e.what());
        setAllVisible(false);
    }

    suiteModel_->setData(viewHandler_->data()->suites(),viewHandler_->suitePlotState());
    childReqModel_->setData(viewHandler_->data()->total().childSubReq(),viewHandler_->childPlotState());
    userReqModel_->setData(viewHandler_->data()->total().userSubReq(),viewHandler_->userPlotState());

    for(int i=0; i < suiteModel_->columnCount()-1; i++)
        ui_->suiteTree->resizeColumnToContents(i);

    logModel_->loadFromFile(logFile_.toStdString(),viewHandler_->data()->startPos());

    QDateTime startTime=viewHandler_->data()->startTime();
    QDateTime endTime=viewHandler_->data()->endTime();
    ui_->startTe->setMinimumDateTime(startTime);
    ui_->startTe->setMaximumDateTime(endTime);
    ui_->startTe->setDateTime(startTime);
    ui_->endTe->setMinimumDateTime(startTime);
    ui_->endTe->setMaximumDateTime(endTime);
    ui_->endTe->setDateTime(endTime);

    updateInfoLabel();
}

void LogLoadWidget::periodChanged(qint64 start,qint64 end)
{
    QDateTime startDt=QDateTime::fromMSecsSinceEpoch(start);
    QDateTime endDt=QDateTime::fromMSecsSinceEpoch(end);
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
}

void LogLoadWidget::periodWasReset()
{
    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
}

void LogLoadWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        viewHandler_->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        viewHandler_->setResolution(LogLoadData::MinuteResolution);
    else if(idx == 2)
        viewHandler_->setResolution(LogLoadData::HourResolution);
}

void LogLoadWidget::currentTabChanged(int idx)
{
    if(idx == 0)
    {
        ui_->scanLabel->show();
        suiteModel_->setShowColour(true);
        childReqModel_->setShowColour(false);
        userReqModel_->setShowColour(false);
    }
    else if(idx == 1)
    {
        ui_->scanLabel->show();
        suiteModel_->setShowColour(false);
        childReqModel_->setShowColour(true);
        userReqModel_->setShowColour(true);
    }
    else if(idx == 2)
    {
        ui_->scanLabel->clear();
        ui_->scanLabel->hide();
        suiteModel_->setShowColour(false);
        childReqModel_->setShowColour(false);
        userReqModel_->setShowColour(false);
        ui_->controlToolBox->setCurrentIndex(0);
    }
}

LogLoadRequestSortModel::LogLoadRequestSortModel(QObject* parent) : QSortFilterProxyModel(parent)
{

}

bool LogLoadRequestSortModel::lessThan(const QModelIndex &left,
                                        const QModelIndex &right) const
{
    if(left.column() == 0)
    {
        QString leftData = sourceModel()->data(left).toString();
        QString rightData = sourceModel()->data(right).toString();

        return QString::localeAwareCompare(leftData, rightData) < 0;
    }
    else
    {
        qreal leftData = sourceModel()->data(left,Qt::UserRole).toFloat();
        qreal rightData = sourceModel()->data(right,Qt::UserRole).toFloat();

        if(leftData == rightData)
        {
            QModelIndex leftIdx=sourceModel()->index(left.row(),0);
            QModelIndex rightIdx=sourceModel()->index(right.row(),0);

            QString leftString = sourceModel()->data(leftIdx).toString();
            QString rightString = sourceModel()->data(rightIdx).toString();

            return QString::localeAwareCompare(leftString, rightString) < 0;
        }

        else
           return leftData < rightData;

    }

    return false;
}

//=====================================================
//
//  LogLoadRequestModel
//
//=====================================================

LogLoadRequestModel::LogLoadRequestModel(QString dataName,QObject *parent) :
          QAbstractItemModel(parent),
          dataName_(dataName),
          showColour_(true)
{
}

LogLoadRequestModel::~LogLoadRequestModel()
{
}



void LogLoadRequestModel::setData(const std::vector<LogRequestItem>& data,QList<bool> checkedLst)
{
    //Q_ASSERT(data);
    Q_ASSERT(data.size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data.size(); i++)
    {
        data_ << LogLoadRequestModelDataItem(QString::fromStdString(data[i].name_),
                                           data[i].percentage_,checkedLst[i],
                                           data[i].rank_);
    }
    endResetModel();
}

void LogLoadRequestModel::setData(const std::vector<LogLoadDataItem>& data,QList<bool> checkedLst)
{
    //Q_ASSERT(data);
    Q_ASSERT(data.size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data.size(); i++)
    {
        data_ << LogLoadRequestModelDataItem(QString::fromStdString(data[i].name()),
                                           data[i].percentage(),checkedLst[i],
                                           data[i].rank());
    }
    endResetModel();
}

void LogLoadRequestModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

bool LogLoadRequestModel::hasData() const
{
    return !data_.isEmpty();
}

int LogLoadRequestModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 2;
}

int LogLoadRequestModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return data_.count();
    }

    return 0;
}

Qt::ItemFlags LogLoadRequestModel::flags ( const QModelIndex & index) const
{
    Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == 0)
    {
        defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;
    }
    return defaultFlags;
}

QVariant LogLoadRequestModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }
    int row=index.row();
    if(row < 0 || row >= data_.count())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return data_[row].name_;
            break;
        case 1:
            return formatPrecentage(data_[row].percentage_);
            break;
        default:
            break;
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        if(index.column() == 0)
            return (data_[row].checked_)?QVariant(Qt::Checked):QVariant(Qt::Unchecked);

        return QVariant();
    }
    else if(role == Qt::UserRole)
    {
        switch(index.column())
        {
        case 0:
            return data_[row].name_;
            break;
        case 1:
            return data_[row].percentage_;
            break;
        default:
            break;
        }
    }
    else if(role == Qt::BackgroundRole && showColour_)
    {
        if(data_[row].checked_ && data_[row].col_ != QColor() )
            return data_[row].col_;

        return QVariant();
    }

    return QVariant();
}

bool LogLoadRequestModel::setData(const QModelIndex& idx, const QVariant & value, int role )
{
    if(idx.column() == 0 && role == Qt::CheckStateRole)
    {
        QModelIndex startIdx=index(idx.row(),0);
        QModelIndex endIdx=index(idx.row(),columnCount()-1);
        Q_EMIT dataChanged(startIdx,endIdx);

        bool checked=(value.toInt() == Qt::Checked)?true:false;
        data_[idx.row()].checked_=checked;
        Q_EMIT checkStateChanged(idx.row(),checked);

        return true;
    }
    return false;
}

QVariant LogLoadRequestModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch ( section )
        {
        case 0: return dataName_;
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    else if(role== Qt::ToolTipRole)
    {
        switch ( section )
        {
        case 0: return dataName_;
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex LogLoadRequestModel::index( int row, int column, const QModelIndex & parent ) const
{
    if(!hasData() || row < 0 || column < 0)
    {
        return QModelIndex();
    }

    //When parent is the root this index refers to a node or server
    if(!parent.isValid())
    {
        return createIndex(row,column);
    }

    return QModelIndex();

}

QModelIndex LogLoadRequestModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QString LogLoadRequestModel::formatPrecentage(float perc) const
{
    if(perc < 0.000000001)
        return "0";
    else if(perc < 0.5)
        return "<0.5";

    return QString::number(perc,'f',1);
}

void LogLoadRequestModel::updateItem(int idx,bool st,QColor col)
{
    if(idx>=0 && idx < data_.size())
    {
        data_[idx].col_=col.lighter(150);
        QModelIndex startIdx=index(idx,0);
        QModelIndex endIdx=index(idx,columnCount()-1);
        Q_EMIT dataChanged(startIdx,endIdx);
    }
}

void LogLoadRequestModel::unselectAll()
{
    for(int i=0; i < data_.size(); i++)
    {
        if(data_[i].checked_)
        {
            data_[i].checked_=false;
            Q_EMIT checkStateChanged(i,false);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::selectFirstFourItems()
{
    unselectAll();
    for(int i=0; i < data_.size(); i++)
    {
        if(data_[i].rank_ < 4 && data_[i].rank_ >=0)
        {
            data_[i].checked_=true;
            Q_EMIT checkStateChanged(i,true);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::setShowColour(bool b)
{
    showColour_=b;
    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

//=============================================
//
// ChartCallout
//
//=============================================

ChartCallout::ChartCallout(QChart *chart):
    QGraphicsItem(chart),
    chart_(chart)
{
    font_.setPointSize(font_.pointSize()-1);
}

QRectF ChartCallout::boundingRect() const
{
    QPointF anchor = mapFromParent(chart_->mapToPosition(anchor_));
    QPointF bottom = mapFromParent(chart_->mapToPosition(bottomPos_));
    QRectF rect;
    rect.setLeft(qMin(rect_.left(), anchor.x()));
    rect.setRight(qMax(rect_.right(), anchor.x()));
    rect.setTop(qMin(rect_.top(), anchor.y()));
    rect.setBottom(bottom.y());
    return rect;
}

void ChartCallout::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    QPainterPath path;
    path.addRoundedRect(rect_, 5, 5);

    //The callout shape
    QPointF anchor = mapFromParent(chart_->mapToPosition(anchor_));
    if(!rect_.contains(anchor))
    {
        QPointF point1, point2;

        bool above = anchor.y() <= rect_.top();
        bool aboveCenter = anchor.y() > rect_.top() && anchor.y() <= rect_.center().y();
        bool belowCenter = anchor.y() > rect_.center().y() && anchor.y() <= rect_.bottom();
        bool below = anchor.y() > rect_.bottom();

        bool onLeft = anchor.x() <= rect_.left();
        bool leftOfCenter = anchor.x() > rect_.left() && anchor.x() <= rect_.center().x();
        bool rightOfCenter = anchor.x() > rect_.center().x() && anchor.x() <= rect_.right();
        bool onRight = anchor.x() > rect_.right();

        // get the nearest rect corner.
        qreal x = (onRight + rightOfCenter) * rect_.width();
        qreal y = (below + belowCenter) * rect_.height();
        bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
        bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);

        qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
        qreal y1 = y + aboveCenter * 5 - belowCenter * 10 + cornerCase * vertical * (above * 5 - below * 10);
        point1.setX(x1);
        point1.setY(y1);

        qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);
        qreal y2 = y + aboveCenter * 10 - belowCenter * 5 + cornerCase * vertical * (above * 10 - below * 5);
        point2.setX(x2);
        point2.setY(y2);

        path.moveTo(point1);
        path.lineTo(anchor);
        path.lineTo(point2);
        path = path.simplified();
    }

    painter->setBrush(QColor(198,223,188));
    //painter->setBrush(QColor(255,245,204));
    painter->drawPath(path);

    painter->setFont(font_);
    //painter->setPen(QColor(255, 255, 255));
    painter->drawText(textRect_, text_);

    //Vertical line down from the anchor pos
    painter->setPen(QPen(QColor(80,80,80),1,Qt::DotLine));
    painter->drawLine(anchor,mapFromParent(chart_->mapToPosition(bottomPos_)));
}

void ChartCallout::setText(const QString &text)
{
    text_ = text;
    QFontMetrics metrics(font_);
    textRect_ = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, text_);
    textRect_.translate(5, 5);
    prepareGeometryChange();
    rect_ = textRect_.adjusted(-5, -5, 5, 5);
}

void ChartCallout::setAnchor(QPointF point)
{
    anchor_ = point; //in value coords
    bottomPos_ = QPointF(point.x(),0); //in value coords
    updateGeometry();
}

void ChartCallout::updateGeometry()
{
    prepareGeometryChange();
    setPos(chart_->mapToPosition(anchor_) + QPoint(10, -30));
}

//=============================================
//
// ChartView
//
//=============================================

ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent), callout_(0)
{
    setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    QChartView::mousePressEvent(event);
    if(event->button() == Qt::MidButton)
    {
       if(event->pos().x() <= chart()->plotArea().right() &&
          event->pos().x() >= chart()->plotArea().left())
       {
            qreal t=chart()->mapToValue(event->pos()).x();
            Q_EMIT positionClicked(t);
       }
       else
       {
            Q_EMIT positionClicked(0);
       }
    }
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);

    if(event->pos().x() <= chart()->plotArea().right() &&
       event->pos().x() >= chart()->plotArea().left())
    {
        qreal v=chart()->mapToValue(event->pos()).x();
        Q_EMIT positionChanged(v);
    }
    else
        Q_EMIT positionChanged(-1);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    QPointF oriLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF oriRight=chart()->mapToValue(chart()->plotArea().topRight());

    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QChartView::mouseReleaseEvent(event);

    //UiLog().dbg() << "   " << chart()->plotArea();
    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QPointF newLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF newRight=chart()->mapToValue(chart()->plotArea().topRight());

    if(newLeft != oriLeft || newRight != oriRight )
    {
        Q_EMIT chartZoomed(QRectF(newLeft,newRight));
    }

    qint64 period=newRight.x()-newLeft.x(); //in ms
    adjustTimeAxis(period);
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

void ChartView::doZoom(QRectF valRect)
{
    QRectF r(chart()->mapToPosition(valRect.bottomLeft()),
             chart()->mapToPosition(valRect.topRight()));

    if(r.isValid())
    {
        chart()->zoomIn(r);
        qint64 period=valRect.width(); //in ms
        adjustTimeAxis(period);
    }
}

void ChartView::doZoom(qint64 start,qint64 end)
{
    QPointF left=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF right=chart()->mapToValue(chart()->plotArea().topRight());

    QRectF valRect(QPointF(start,left.y()),QPointF(end,right.y()));
    QRectF r(chart()->mapToPosition(valRect.bottomLeft()),
             chart()->mapToPosition(valRect.topRight()));

    //QRectF r(chart()->mapToPosition(QPointF(start,left.y())),
    //         chart()->mapToPosition(QPointF(end,left.y()+1)));

    UiLog().dbg() << "start " << QDateTime::fromMSecsSinceEpoch(start);
    UiLog().dbg() << "end " << QDateTime::fromMSecsSinceEpoch(end);

    if(r.isValid())
    {
        chart()->zoomIn(r);
        qint64 period=end-start; //in ms
        adjustTimeAxis(period);
    }
}

void ChartView::currentTimeRange(qint64& start,qint64& end)
{
    start=chart()->mapToValue(chart()->plotArea().bottomLeft()).x();
    end=chart()->mapToValue(chart()->plotArea().topRight()).x();
}

qint64 ChartView::widthToTimeRange(float wPix)
{
    if(wPix > 0)
    {
        float pw=chart()->plotArea().width();
        if(pw >0)
        {
            qint64 start=chart()->mapToValue(chart()->plotArea().bottomLeft()).x();
            qint64 end=chart()->mapToValue(chart()->plotArea().topRight()).x();
            qint64 tw=(end-start)*(wPix/chart()->plotArea().width());
            return tw;
        }
    }
    return -1;
}

void ChartView::adjustTimeAxis(qint64 periodInMs)
{
    qint64 period=periodInMs/1000; //in seconds
    QString format;

    if(period < 60*60)
    {
        format="hh:mm:ss";
    }   
    else if(period < 2*24*3600)
    {
        format="hh:mm";
    }   
    else
    {
        format="dd MMM";
    }

    if(QDateTimeAxis *ax=static_cast<QDateTimeAxis*>(chart()->axisX()))
    {
        ax->setFormat(format);
    }
}

void ChartView::setCallout(qreal val)
{
    if(!callout_)
    {
        callout_=new ChartCallout(chart());
        scene()->addItem(callout_);
    }

    if(QValueAxis *axisY=static_cast<QValueAxis*>(chart()->axisY()))
    {
        qreal m=axisY->max();
        callout_->setAnchor(QPointF(val,m));
        QString txt=QDateTime::fromMSecsSinceEpoch(val).toString("hh:mm:ss dd/MM/yyyy");
        callout_->setText(txt);
    }
}

void ChartView::adjustCallout()
{
    if(callout_)
    {
        QPointF anchor=callout_->anchor();
        qint64 start,end;
        currentTimeRange(start,end);
        if(anchor.x() >= start && anchor.x() <=end)
            callout_->setAnchor(callout_->anchor());
        else
        {
            scene()->removeItem(callout_);
            delete callout_;
            callout_=0;
        }
    }
}

void ChartView::removeCallout()
{
    if(callout_)
    {
        scene()->removeItem(callout_);
        delete callout_;
        callout_=0;
    }
}

//=============================================
//
// LogRequestViewHandler
//
//=============================================

LogRequestViewHandler::LogRequestViewHandler(QWidget* parent) :
    data_(0), lastScanIndex_(0)
{
    //The data object - to read and store processed log data
    data_=new LogLoadData();

    views_ << new LogTotalRequestView(this,parent);
    views_ << new LogSuiteRequestView(this,parent);
    //views_ << new LogSubRequestView(this,parent);
    views_ << new LogStatRequestView(this,parent);

    for(int i=0; i < views_.count(); i++)
    {
        connect(views_[i],SIGNAL(scanDataChanged(QString)),
                this,SIGNAL(scanDataChanged(QString)));

        connect(views_[i],SIGNAL(suitePlotStateChanged(int,bool,QColor)),
                this,SIGNAL(suitePlotStateChanged(int,bool,QColor)));

        connect(views_[i],SIGNAL(childPlotStateChanged(int,bool,QColor)),
                this,SIGNAL(childPlotStateChanged(int,bool,QColor)));

        connect(views_[i],SIGNAL(userPlotStateChanged(int,bool,QColor)),
                this,SIGNAL(userPlotStateChanged(int,bool,QColor)));

        connect(views_[i],SIGNAL(timeRangeChanged(qint64,qint64)),
                this,SIGNAL(timeRangeChanged(qint64,qint64)));

        connect(views_[i],SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
                this,SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)));

        connect(views_[i],SIGNAL(timeRangeReset()),
                this,SIGNAL(timeRangeReset()));
    }
}

LogRequestViewHandler::~LogRequestViewHandler()
{
    delete data_;
}

void LogRequestViewHandler::clear()
{
    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->clear();
    }
}

void LogRequestViewHandler::load(const std::string& logFile,int numOfRows)
{
    data_->loadLogFile(logFile,numOfRows);

    suitePlotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suitePlotState_ << false;

    childPlotState_.clear();
    for(size_t i=0; i < data_->total().childSubReq().size(); i++)
        childPlotState_ << false;

    userPlotState_.clear();
    for(size_t i=0; i < data_->total().userSubReq().size(); i++)
        userPlotState_ << false;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->load();
    }
}

void LogRequestViewHandler::setResolution(LogLoadData::TimeRes res)
{
     data_->setTimeRes(res);
     for(int i=0; i < views_.count(); i++)
     {
         views_[i]->changeResolution();
     }
}

void LogRequestViewHandler::showFullRange()
{
    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->showFullRange();
    }
}

void LogRequestViewHandler::addRemoveSuite(int idx, bool st)
{
    suitePlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveSuite(idx,st);
    }
}

void LogRequestViewHandler::addRemoveChildReq(int idx, bool st)
{
    childPlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveChildReq(idx,st);
    }
}

void LogRequestViewHandler::addRemoveUserReq(int idx, bool st)
{
    userPlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveUserReq(idx,st);
    }
}

void LogRequestViewHandler::slotZoomHappened(QRectF r)
{
    if(LogRequestView* senderView=static_cast<LogRequestView*>(sender()))
    {
        Q_FOREACH(LogRequestView* v,views_)
        {
            if(v != senderView)
                v->adjustZoom(r);
        }
    }
}

//=============================================
//
// LogRequestView
//
//=============================================

LogRequestView::LogRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    QScrollArea(parent),
    handler_(handler),
    maxVal_(0)
{
    QWidget* w=new QWidget(this);
    mainLayout_=new QVBoxLayout(w);
    mainLayout_->setContentsMargins(0,0,0,0);
    mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
    setWidgetResizable(true);
    setWidget(w);
}

LogRequestView::~LogRequestView()
{
}

QChart* LogRequestView::addChartById(QString id)
{
    QChart* chart = new QChart();
    ChartView* chartView=new ChartView(chart,this);
    chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout_->addWidget(chartView);
    views_ << chartView;

    connect(chartView,SIGNAL(chartZoomed(QRectF)),
            this,SLOT(slotZoom(QRectF)));

    connect(chartView,SIGNAL(chartZoomed(QRectF)),
            this,SIGNAL(zoomHappened(QRectF)));

    connect(chartView,SIGNAL(positionChanged(qreal)),
            this,SLOT(scanPositionChanged(qreal)));

    connect(chartView,SIGNAL(positionClicked(qreal)),
            this,SLOT(scanPositionClicked(qreal)));

    viewIds_[id]=chartView;
    return chart;
}

void LogRequestView::removeChartById(QString id)
{
    if(ChartView* chartView=viewIds_.value(id,NULL))
    {
        mainLayout_->removeWidget(chartView);
        delete chartView;
        views_.removeOne(chartView);
    }

}

QString LogRequestView::chartId(ChartView* cv)
{
    return viewIds_.key(cv,"");
}

void LogRequestView::slotZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);

           v->adjustCallout();
        }

        qint64 start, end;
        Q_ASSERT(!views_.isEmpty());
        views_[0]->currentTimeRange(start,end);
        Q_EMIT timeRangeChanged(start,end);
    }
}

void LogRequestView::adjustZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);

           v->adjustCallout();
        }
    }
}

//Adjust the time range (zoom) of the last view using the time range
//in the first view
void LogRequestView::adjustZoom()
{
    qint64 start=-1, end=-1;
    if(views_.count() > 1)
    {
        views_[0]->currentTimeRange(start,end);

        if(start != -1 && end  !=-1)
        {
            ChartView* view=views_.back();
            view->doZoom(start,end);
            view->adjustCallout();
        }
   }
}


void LogRequestView::changeResolution()
{
    qint64 start=-1, end=-1;
    if(!views_.isEmpty())
    {
        views_[0]->currentTimeRange(start,end);
    }

    loadCore();
    loadSuites();

    if(start != -1 && end  !=-1)
    {
        Q_FOREACH(ChartView* v,views_)
        {
            v->doZoom(start,end);
            v->adjustCallout();
        }
    }
}

#if 0
void LogRequestView::addRemoveSuite(int idx, bool st)
{
    if(idx >= 0 && idx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(idx);
            Q_EMIT suitePlotStateChanged(idx,true,
                   suiteSeriesColour(getChart(TotalChartType),idx));
        }
        //remove
        else
        {
            //suitePlotState_[idx]=false;

            removeSuiteSeries(getChart(TotalChartType),
                              "s_main_" + QString::number(idx));

            removeSuiteSeries(getChart(ChildChartType),
                              "s_ch_" + QString::number(idx));

            removeSuiteSeries(getChart(UserChartType),
                              "s_us_" + QString::number(idx));
        }
    }
}
#endif

#if 0
void LogRequestView::addSuite(int idx)
{
    //suitePlotState_[idx]=true;

    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName("s_main_" + QString::number(idx));
    data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("s_ch_" + QString::number(idx));
    data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("s_us_" + QString::number(idx));
    data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}
#endif

void LogRequestView::removeSeries(QChart* chart,QString id)
{
    Q_FOREACH(QAbstractSeries *s,chart->series())
    {
        if(s->name() == id)
        {
            chart->removeSeries(s);
            return;
        }
    }
}

QColor LogRequestView::seriesColour(QChart* chart,QString id)
{
    Q_FOREACH(QAbstractSeries *s,chart->series())
    {
        if(s->name().endsWith(id))
        {
            if(QLineSeries *ls=static_cast<QLineSeries*>(s))
                return ls->color();
            break;
        }
    }
    return QColor();
}

void LogRequestView::clear()
{
    clearCharts();
    //suitePlotState_.clear();
    //size_t lastScanIndex_=0;;
}

void LogRequestView::clearCharts()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        v->chart()->removeAllSeries();
        v->chart()->removeAxis(v->chart()->axisX());
        v->chart()->removeAxis(v->chart()->axisY());
    }
    maxVal_=0;
}

void LogRequestView::clearViews()
{
    QLayoutItem* child=0;
    while ((child = mainLayout_->takeAt(0)) != 0)
    {
        QWidget* w=child->widget();
        delete child;
        if(w)
            delete w;
    }

    Q_ASSERT(mainLayout_->count() == 0);
    views_.clear();
    maxVal_=0;
}

void LogRequestView::load()
{
    //data_->loadLogFile(logFile);

    loadCore();
    loadSuites();

    //suitePlotState_.clear();
    //for(size_t i=0; i < data_->suites().size(); i++)
    //    suitePlotState_ << false;
}

#if 0
void LogRequestView::loadCore()
{
    clearCharts();

    int maxVal=0;
    QLineSeries* tSeries=new QLineSeries();
    tSeries->setName("all");
    data_->getTotalReq(*tSeries,maxVal);

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("all");
    data_->getChildReq(*chSeries);

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("all");
    data_->getUserReq(*usSeries);

    build(getView(TotalChartType),tSeries,"Child+User requests",maxVal);
    build(getView(ChildChartType),chSeries,"Child requests",maxVal);
    build(getView(UserChartType),usSeries,"User requests",maxVal);
}
#endif

void LogRequestView::loadSuites()
{
    for(int i=0; i < handler_->suitePlotState_.count(); i++)
    {
        if(handler_->suitePlotState_[i])
        {
            addSuite(i);
        }
    }
}

void LogRequestView::build(ChartView* view,QLineSeries *series, QString title,int maxVal)
{
    Q_ASSERT(view);
    QChart *chart=view->chart();
    Q_ASSERT(chart);

    chart->addSeries(series);
    chart->setTitle(title);

    if(!chart->axisX())
    {
        chart->legend()->hide();
        QDateTimeAxis *axisX = new QDateTimeAxis;
        axisX->setTickCount(10);
        axisX->setFormat("HH dd/MM");
        chart->setAxisX(axisX, series);
        view->adjustTimeAxis(handler_->data_->period());

        QValueAxis *axisY = new QValueAxis;
        axisY->setLabelFormat("%i");

        QString yTitle;
        if(handler_->data_->timeRes() == LogLoadData::SecondResolution)
            yTitle="Req. per second";
        else if(handler_->data_->timeRes() == LogLoadData::MinuteResolution)
            yTitle="Req. per minute";
        else if(handler_->data_->timeRes() == LogLoadData::HourResolution)
            yTitle="Req. per hour";

        axisY->setTitleText(yTitle);
        axisY->setMin(0.);
        chart->setAxisY(axisY, series);
        axisY->setMin(0.);
        axisY->setMax(maxVal);
    }
    else
    {
        chart->addSeries(series);
        series->attachAxis(chart->axisX());
        series->attachAxis(chart->axisY());
    }
}

void LogRequestView::adjustMaxVal()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        if(QValueAxis *axisY=static_cast<QValueAxis*>(v->chart()->axisY()))
        {
            axisY->setMax(maxVal_);
        }
    }
}

void LogRequestView::showFullRange()
{
    Q_FOREACH(ChartView* view,views_)
        view->chart()->zoomReset();

    Q_EMIT(timeRangeReset());
}

void LogRequestView::scanPositionClicked(qreal pos)
{
    if(pos < 1)
    {
        Q_FOREACH(ChartView* view,views_)
            view->removeCallout();

        Q_EMIT(timeRangeHighlighted(0,0,0));
    }
    else
    {
        qint64 t1(pos);
        qint64 t2=t1;
        if(handler_->data_->timeRes() == LogLoadData::MinuteResolution)
            t2=t1+60*1000;

        Q_FOREACH(ChartView* view,views_)
            view->setCallout(pos);

        qint64 tw=0;
        if(!views_.isEmpty())
            tw=views_[0]->widthToTimeRange(50.);

        Q_EMIT(timeRangeHighlighted(t1,t2,tw));
    }
}

bool LogRequestView::seriesIndex(qint64 t,int startIdx,qint64 tolerance,int& idx)
{
    QChart *chart=views_[0]->chart();

    QList<QAbstractSeries*> lst=chart->series();
    if(lst.empty())
        return false;

    QLineSeries *ser=static_cast<QLineSeries*>(lst[0]);
    Q_ASSERT(ser);

    idx=-1;
    if(t < 0)
        return false;

    int num=ser->count();
    if(num == 0)
        return false;

    if(startIdx > num-1)
        startIdx=0;

    if(startIdx >= num)
        return false;

    if(t >= ser->at(startIdx).x())
    {
        if(tolerance <=0)
            tolerance=10*1000; //ms

        for(int i=startIdx; i < num; i++)
        {
            if(ser->at(i).x() >= t)
            {
                qint64 nextDelta=ser->at(i).x()-t;
                qint64 prevDelta=(i > 0)?(t-ser->at(i-1).x()):(nextDelta+1);
                if(prevDelta > nextDelta && nextDelta <=tolerance)
                {
                    idx=i;
                    return true;
                }
                else if(prevDelta < nextDelta && prevDelta <=tolerance)
                {
                    idx=i-1;
                    return true;
                }
                return false;
             }
        }
    }
    else
    {
        if(tolerance <=0)
            tolerance=10*1000; //ms

        for(int i=startIdx; i >=0; i--)
        {
            if(ser->at(i).x() <= t)
            {
                qint64 nextDelta=t-ser->at(i).x();
                qint64 prevDelta=(i < startIdx)?(ser->at(i+1).x()-t):(nextDelta+1);
                if(prevDelta > nextDelta && nextDelta <=tolerance)
                {
                    idx=i;
                    return true;
                }
                else if(prevDelta < nextDelta && prevDelta <=tolerance)
                {
                    idx=i+1;
                    return true;
                }
                return false;
            }
        }
    }

    return false;
}

qint64 LogRequestView::seriesTime(int idx)
{
    QChart *chart=views_[0]->chart();

    QList<QAbstractSeries*> lst=chart->series();
    if(lst.empty())
        return 0;

    QLineSeries *ser=static_cast<QLineSeries*>(lst[0]);
    Q_ASSERT(ser);
    return ser->at(idx).x();
}

int LogRequestView::seriesValue(QChart* chart,QString id,int idx)
{
    if(chart)
    {
        Q_FOREACH(QAbstractSeries *s,chart->series())
        {
            if(s->name().endsWith(id))
            {
                if(QLineSeries *ls=static_cast<QLineSeries*>(s))
                    return ls->at(idx).y();
                break;
            }
        }
        return 0;
    }

    return 0;
}


void LogRequestView::scanPositionChanged(qreal pos)
{
    qint64 t(pos);
    int idx=-1;

    if(views_.isEmpty())
    {
        QString t;
        Q_EMIT scanDataChanged(t);
        return;
    }

    qint64 tw=views_[0]->widthToTimeRange(50.);
    bool hasData=seriesIndex(t,handler_->lastScanIndex_,tw,idx);

    handler_->lastScanIndex_=idx;
    //UiLog().dbg() << "idx=" << idx;

    //QColor dateCol(210,212,218);
    QColor dateCol(210,211,214);
    //QString dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(data_->time()[idx]).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    //QString txt=Viewer::formatText("date (): " + dateTxt, dateCol);

    QString txt="<table width=\'100%\' cellpadding=\'4px\'>";
    QString dateTxt=QDateTime::fromMSecsSinceEpoch(t).toString("hh:mm:ss dd/MM/yyyy");

    txt="<tr>" +
        Viewer::formatTableTdText("Date (cursor): ",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";

    //Viewer::formatText("date (at cursor): </td>" +
    //                QDateTime::fromMSecsSinceEpoch(t).toString("hh:mm:ss dd/MM/yyyy"),
    //                dateCol);

    dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(seriesTime(idx)).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    txt+="<tr>" +
        Viewer::formatTableTdText("Date (nearest):",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";

    buildScanTable(txt,idx);

    Q_EMIT scanDataChanged(txt);
}

void LogRequestView::buildScanRow(QString &txt,QString name,size_t val,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(QString::number(val),numBg) + "</tr>";
}

void LogRequestView::buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(QString::number(tot),numBg) +
          Viewer::formatTableTdBg(QString::number(ch),numBg) +
          Viewer::formatTableTdBg(QString::number(us),numBg) + "</tr>";
}

void LogRequestView::buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) + "</tr>";
}

//=============================================================================
//
// LogTotalRequestView
//
//=============================================================================

LogTotalRequestView::LogTotalRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    for(int i=0; i < 3; i++)
    {
        addChartById(QString::number(i));
    }

    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
}

QChart* LogTotalRequestView::getChart(ChartType type)
{
    if(ChartView *v=getView(type))
        return v->chart();
    return 0;
}

ChartView* LogTotalRequestView::getView(ChartType type)
{
    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
    switch(type)
    {
    case TotalChartType:
        return views_[0];
    case ChildChartType:
        return views_[1];
    case UserChartType:
        return views_[2];
    default:
        break;
    }

    return 0;
}

void LogTotalRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            Q_EMIT suitePlotStateChanged(suiteIdx,true,
                   seriesColour(getChart(TotalChartType),suiteSeriesId(suiteIdx)));
        }
        //remove
        else
        {
            removeSeries(getChart(TotalChartType),
                              suiteSeriesId(suiteIdx));

            removeSeries(getChart(ChildChartType),
                              suiteSeriesId(suiteIdx));

            removeSeries(getChart(UserChartType),
                              suiteSeriesId(suiteIdx));
        }
    }
}

void LogTotalRequestView::addSuite(int idx)
{
    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName(suiteSeriesId(idx));
    handler_->data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName(suiteSeriesId(idx));
    handler_->data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName(suiteSeriesId(idx));
    handler_->data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}

void LogTotalRequestView::loadCore()
{
    clearCharts();

    int maxVal=0;
    QLineSeries* tSeries=new QLineSeries();
    tSeries->setName("all");
    handler_->data_->getTotalReq(*tSeries,maxVal);

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("all");
    handler_->data_->getChildReq(*chSeries);

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("all");
    handler_->data_->getUserReq(*usSeries);

    build(getView(TotalChartType),tSeries,"Child+User requests",maxVal);
    build(getView(ChildChartType),chSeries,"Child requests",maxVal);
    build(getView(UserChartType),usSeries,"User requests",maxVal);
}

QString LogTotalRequestView::suiteSeriesId(int idx) const
{
    return "s_" + QString::number(idx);
}


void LogTotalRequestView::buildScanTable(QString& txt,int idx)
{
    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Item",hdrCol) +
          Viewer::formatTableThText("Total",hdrCol) +
          Viewer::formatTableThText("Child",hdrCol) +
          Viewer::formatTableThText("User",hdrCol) + "</tr>";

    QChart* tChart=getChart(TotalChartType);
    QChart* cChart=getChart(ChildChartType);
    QChart* uChart=getChart(UserChartType);

    if(idx != -1)
    {
        size_t tot=0,ch=0,us=0;
        QColor col=seriesColour(tChart,"all");
        QString name="all";

        tot=seriesValue(tChart,"all",idx);
        ch=seriesValue(cChart,"all",idx);
        us=seriesValue(uChart,"all",idx);
        buildScanRow(txt,name,tot,ch,us,col);

        for(int i=0; i < handler_->suitePlotState().size(); i++)
        {
            if(handler_->suitePlotState()[i])
            {
                tot=0;ch=0;us=0;
                QString id=suiteSeriesId(i);
                col=seriesColour(tChart,id);
                name=QString::fromStdString(handler_->data_->suites()[i].name());
                tot=seriesValue(tChart,id,idx);
                ch=seriesValue(cChart,id,idx);
                us=seriesValue(uChart,id,idx);
                buildScanRow(txt,name,tot,ch,us,col);
            }
        }
    }
    else
    {
        QColor col=seriesColour(tChart,"all");
        QString name="all";
        buildEmptyScanRow(txt,name,col);
        for(int i=0; i < handler_->suitePlotState().size(); i++)
        {
            if(handler_->suitePlotState()[i])
            {
                QString id=suiteSeriesId(i);
                col=seriesColour(tChart,id);
                name=QString::fromStdString(handler_->data_->suites()[i].name());
                buildEmptyScanRow(txt,name,col);
            }
        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogSuiteRequestView
//
//=============================================================================

LogSuiteRequestView::LogSuiteRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);
}

//One chart = one suite with all the subrequests (child + user)
void LogSuiteRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            QString id=QString::number(suiteIdx);
            ChartView* view=viewIds_.value(id,NULL);
            Q_ASSERT(view);
            //It only woks if the chart is already displayed, so we need a delayed
            //adjustment
            QTimer::singleShot(0,this, SLOT(adjustZoom()));
        }
        //remove
        else
        {
            removeChartById(QString::number(suiteIdx));
        }
    }
}

void LogSuiteRequestView::addSuite(int suiteIdx)
{
    //at this point total must be already added, so we do not need to adjust the
    //maxval
    QString id=QString::number(suiteIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="suite: " +
            QString::fromStdString(handler_->data_->suites()[suiteIdx].name());

    for(int i=0; i < handler_->childPlotState_.count(); i++)
    {
        if(handler_->childPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(childSeriesId(i));

            handler_->data_->getSuiteChildSubReq(suiteIdx,i,*series);
            build(view,series,title,maxVal_);

            //if(views_.count() ==1)
           // {
               // Q_EMIT childPlotStateChanged(i,true,
               //     childSeriesColour(view->chart(),i));
            //}
        }
    }

    for(int i=0; i < handler_->userPlotState_.count(); i++)
    {
        if(handler_->userPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(userSeriesId(i));
            handler_->data_->getSuiteUserSubReq(suiteIdx,i,*series);
            build(view,series,title,maxVal_);

            //if(views_.count() ==1)
            //{
              //  Q_EMIT userPlotStateChanged(i,true,
              //      userSeriesColour(view->chart(),i));
            //}
        }
    }
}

void LogSuiteRequestView::addTotal()
{
    int prevMaxVal=maxVal_;

    QString id="total";
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="All suites";

    for(int i=0; i < handler_->childPlotState_.count(); i++)
    {
        if(handler_->childPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(childSeriesId(i));

            int maxVal=0;
            handler_->data_->getChildSubReq(i,*series,maxVal);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal_);

            Q_EMIT childPlotStateChanged(i,true,
                    childSeriesColour(view->chart(),i));
        }
    }

    for(int i=0; i < handler_->userPlotState_.count(); i++)
    {
        if(handler_->userPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(userSeriesId(i));
            int maxVal=0;
            handler_->data_->getUserSubReq(i,*series,maxVal);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal);

            Q_EMIT userPlotStateChanged(i,true,
               userSeriesColour(view->chart(),i));
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}


//One chart = one suite
void LogSuiteRequestView::addRemoveChildReq(int childReqIdx, bool st)
{
    //if(childReqIdx >= 0 && childReqIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addChildReq(childReqIdx);
            if(views_.count() >0)
            {
                Q_EMIT childPlotStateChanged(childReqIdx,true,
                   childSeriesColour(views_[0]->chart(),childReqIdx));
            }
        }
        //remove
        else
        {
            removeChildReq(childReqIdx);
        }
    }
}

void LogSuiteRequestView::addChildReq(int childReqIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        if(chartId(views_[i]) == "total")
        {
            QString title="All suites";

            QLineSeries* series=new QLineSeries();
            series->setName(childSeriesId(childReqIdx));

            int maxVal=0;
            handler_->data_->getChildSubReq(childReqIdx,*series,maxVal);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            int suiteIdx=chartId(views_[i]).toInt();
            Q_ASSERT(suiteIdx >= 0);

            QString title="suite: " +
                QString::fromStdString(handler_->data_->suites()[suiteIdx].name());

            QLineSeries* series=new QLineSeries();
            series->setName(childSeriesId(childReqIdx));
            handler_->data_->getSuiteChildSubReq(suiteIdx,childReqIdx,*series);

            build(views_[i],series,title,maxVal_);
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogSuiteRequestView::removeChildReq(int childReqIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),childSeriesId(childReqIdx));
    }
}

//One chart = one suite
void LogSuiteRequestView::addRemoveUserReq(int userReqIdx, bool st)
{
    //if(userReqIdx >= 0 && userReqIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addUserReq(userReqIdx);
            if(views_.count() >0)
            {
                Q_EMIT userPlotStateChanged(userReqIdx,true,
                   userSeriesColour(views_[0]->chart(),userReqIdx));
            }
        }
        //remove
        else
        {
            removeUserReq(userReqIdx);
        }
    }
}

void LogSuiteRequestView::addUserReq(int userReqIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        //bool global=handler_->data_->suerSubReqIsGlobal(userReqIdx);

        if(chartId(views_[i]) == "total")
        {
            QString title="All suites";

            QLineSeries* series=new QLineSeries();
            series->setName(userSeriesId(userReqIdx));

            int maxVal=0;
            handler_->data_->getUserSubReq(userReqIdx,*series,maxVal);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            int suiteIdx=chartId(views_[i]).toInt();
            Q_ASSERT(suiteIdx >= 0);

            QString title="suite: " +
                QString::fromStdString(handler_->data_->suites()[suiteIdx].name());

            QLineSeries* series=new QLineSeries();
            series->setName(userSeriesId(userReqIdx));
            handler_->data_->getSuiteUserSubReq(suiteIdx,userReqIdx,*series);

            build(views_[i],series,title,maxVal_);

        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogSuiteRequestView::removeUserReq(int userReqIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),userSeriesId(userReqIdx));
    }
}

QString LogSuiteRequestView::childSeriesId(int childIdx) const
{
    return "c_" + QString::number(childIdx);
}

QString LogSuiteRequestView::userSeriesId(int userIdx) const
{
    return "u_" + QString::number(userIdx);
}

//The child colour is the same for all the suites
QColor LogSuiteRequestView::childSeriesColour(QChart* chart,size_t childReqIdx)
{
    return seriesColour(chart,childSeriesId(childReqIdx));
}

//The user colour is the same for all the suites
QColor LogSuiteRequestView::userSeriesColour(QChart* chart,size_t userReqIdx)
{
    return seriesColour(chart,userSeriesId(userReqIdx));
}

void LogSuiteRequestView::loadCore()
{
    //Removes everything
    clearViews();

    //Total
    addTotal();
}

void LogSuiteRequestView::buildScanTable(QString& txt,int idx)
{
    if(views_.count() == 0)
        return;

    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Command",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);
        QString name;
        if(id == "total")
        {
            name="all suites";
        }
        else
        {
            int suiteIdx=chartId(views_[i]).toInt();
            Q_ASSERT(suiteIdx >= 0);
            name="suite: " + handler_->data_->suiteNames()[suiteIdx];
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText(name,QColor(230,230,230)) + "</td></tr>";

        if(idx != -1)
        {
            for(int j=0; j < handler_->childPlotState().count(); j++)
            {
                if(handler_->childPlotState()[j])
                {
                    QString id=childSeriesId(j);
                    QColor col=seriesColour(chart,id);
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,handler_->data_->childSubReqName(j),val,col);
                }
            }
            for(int j=0; j < handler_->userPlotState().count(); j++)
            {
                if(handler_->userPlotState()[j])
                {
                    QString id=userSeriesId(j);
                    QColor col=seriesColour(chart,id);
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,handler_->data_->userSubReqName(j),val,col);
                }
            }
        }
        else
        {

        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogSubRequestView
//
//=============================================================================

LogSubRequestView::LogSubRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);
}

//One chart = all the suites for the given subrequest/command
void LogSubRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            if(views_.size() >0)
            {
                Q_EMIT suitePlotStateChanged(suiteIdx,true,
                   seriesColour(views_[0]->chart(),suiteSeriesId(suiteIdx)));
            }
        }
        //remove
        else
        {
            removeSuite(suiteIdx);
        }
    }
}
void LogSubRequestView::addSuite(int suiteIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);
        QString id=chartId(views_[i]);
        QString type;
        int idx=-1;
        parseChartId(id,type,idx);
        Q_ASSERT(idx >= 0);

        QString title;
        if(type == "c")
        {
            title="child cmd: " + handler_->data_->childSubReqName(idx);
        }
        else if(type == "u")
        {
            title="user cmd: " + handler_->data_->userSubReqName(idx);
        }

        QLineSeries* series=new QLineSeries();

        if(type == "c")
        {
            int childReqIdx=idx;
            series->setName(suiteSeriesId(suiteIdx));
            handler_->data_->getSuiteChildSubReq(suiteIdx,childReqIdx,*series);
        }
        else if(type == "u")
        {
            int userReqIdx=idx;
            series->setName(suiteSeriesId(suiteIdx));
            handler_->data_->getSuiteUserSubReq(suiteIdx,userReqIdx,*series);
        }

        build(views_[i],series,title,handler_->data_->subReqMax());
    }
}

void LogSubRequestView::removeSuite(int suiteIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),suiteSeriesId(suiteIdx));
    }
}

//One chart = one suite
void LogSubRequestView::addRemoveChildReq(int childReqIdx, bool st)
{
    if(st)
    {
        addChildReq(childReqIdx);
    }
    else
    {
        removeChartById(childChartId(childReqIdx));
    }
}

void LogSubRequestView::addChildReq(int childReqIdx)
{
    QString id=childChartId(childReqIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="child cmd: " + handler_->data_->childSubReqName( childReqIdx);

    for(int i=0; i < handler_->suitePlotState_.count(); i++)
    {
        if(handler_->suitePlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(i));
            handler_->data_->getSuiteChildSubReq(i,childReqIdx,*series);
            build(view,series,title,handler_->data_->subReqMax());
        }
    }
}

void LogSubRequestView::addRemoveUserReq(int userReqIdx, bool st)
{
    if(st)
    {
        addUserReq(userReqIdx);
    }
    else
    {
        removeChartById(userChartId(userReqIdx));
    }
}

void LogSubRequestView::addUserReq(int userReqIdx)
{
    QString id=userChartId(userReqIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="user cmd: " + handler_->data_->userSubReqName(userReqIdx);

    for(int i=0; i < handler_->suitePlotState_.count(); i++)
    {
        if(handler_->suitePlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(i));
            handler_->data_->getSuiteUserSubReq(i,userReqIdx,*series);
            build(view,series,title,handler_->data_->subReqMax());
        }
    }
}

QString LogSubRequestView::suiteSeriesId(int suiteIdx) const
{
    return "s_" + QString::number(suiteIdx);
}

QColor LogSubRequestView::suiteSeriesColour(QChart* chart,int suiteIdx)
{
    return seriesColour(chart,suiteSeriesId(suiteIdx));
}

QString LogSubRequestView::childChartId(int idx) const
{
    return "c_" + QString::number(idx);
}

QString LogSubRequestView::userChartId(int idx) const
{
    return "u_" + QString::number(idx);
}

void LogSubRequestView::parseChartId(QString id,QString& type,int& idx)
{
    type.clear();
    idx=-1;
    QStringList lst=id.split("_");
    if(lst.count() ==  2)
    {
        type=lst[0];
        idx=lst[1].toInt();
    }
}

void LogSubRequestView::loadCore()
{
    clearCharts();
}

void LogSubRequestView::buildScanTable(QString& txt,int idx)
{
    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Suite",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);
        QString type;
        int cmdIdx=-1;
        parseChartId(id,type,cmdIdx);
        Q_ASSERT(cmdIdx >= 0);

        QString cmd;
        if(type == "c")
        {
            cmd=handler_->data_->childSubReqName(cmdIdx);
        }
        else if(type == "u")
        {
            cmd=handler_->data_->userSubReqName(cmdIdx);
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText("command: " + cmd,QColor(230,230,230)) +
                "</td></tr>";

        //txt+="</table>";
        //txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

        if(idx != -1)
        {
            for(int j=0; j < handler_->suitePlotState().count(); j++)
            {
                if(handler_->suitePlotState()[j])
                {
                    QString id=suiteSeriesId(j);
                    QColor col=seriesColour(chart,id);
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,handler_->data_->suiteNames()[j],val,col);
                }
            }
        }
        else
        {

        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogStatRequestView
//
//=============================================================================

LogStatRequestView::LogStatRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    textView_=new QTextBrowser(this);
    textView_->setReadOnly(true);
    mainLayout_->addWidget(textView_);

    //Read css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    //QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();

#if 0
    //Add css for state names
    std::vector<VParam*> states=VNState::filterItems();
    for(std::vector<VParam*>::const_iterator it=states.begin(); it!=states.end();++it)
    {
       cssDoc+="font." + (*it)->name() +
               " {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
               (*it)->colour().lighter(120).name() + ", stop: 1 " +  (*it)->colour().name() +
               "); color: " +  (*it)->typeColour().name() + ";}";

    }
#endif

    //Add css for false statements
    //QColor falseCol(218,219,219);
    //cssDoc+="font.false {background-color: " + falseCol.name() + ";}";

    textView_->document()->setDefaultStyleSheet(cssDoc);

    textView_->setStyleSheet("QTextEdit{ background-color: rgb(255,255,255);}");
}

//One chart = one suite with all the subrequests (child + user)
void LogStatRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        buildTable();
    }
}

void LogStatRequestView::buildTable()
{
    QString txt;
    QColor col;

    txt+="<table width=\'100%\'>";
    //txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    txt+="<tr><th>Child command</th><th>Total</th>";
    for(int i=0; i < handler_->suitePlotState().count(); i++)
    {
        if(handler_->suitePlotState()[i])
            txt+="<th>" + handler_->data_->suiteNames()[i] + "</th>";
    }
    txt+="</tr>";

    const std::vector<LogRequestItem>& subCh=handler_->data_->total().childSubReq();
    for(size_t i=0; i < subCh.size(); i++)
    {
        txt+="<tr>";
        txt+=Viewer::formatTableTdText(QString::fromStdString(subCh[i].name_),col);

        int val=subCh[i].sumTotal_;
        txt+=Viewer::formatTableTdText(QString::number(val),col);

        for(int j=0; j < handler_->suitePlotState().count(); j++)
        {
            if(handler_->suitePlotState()[j])
            {
                val=handler_->data_->suites()[j].childSubReq()[i].sumTotal_;
                txt+=Viewer::formatTableTdText(QString::number(val),col);
            }
        }

        txt+="</tr>";
    }

    txt+="<tr><th>User command</th><th>Total</th>";
    for(int i=0; i < handler_->suitePlotState().count(); i++)
    {
        if(handler_->suitePlotState()[i])
            txt+="<th>" + handler_->data_->suiteNames()[i] + "</th>";
    }
    txt+="</tr>";

    const std::vector<LogRequestItem>& subUs=handler_->data_->total().userSubReq();
    for(size_t i=0; i < subUs.size(); i++)
    {
        txt+="<tr>";
        txt+=Viewer::formatTableTdText(QString::fromStdString(subUs[i].name_),col);

        int val=subUs[i].sumTotal_;
        txt+=Viewer::formatTableTdText(QString::number(val),col);

        for(int j=0; j < handler_->suitePlotState().count(); j++)
        {
            if(handler_->suitePlotState()[j])
            {
                int val=handler_->data_->suites()[j].userSubReq()[i].sumTotal_;
                txt+=Viewer::formatTableTdText(QString::number(val),col);
            }
        }

        txt+="</tr>";
    }

    txt+="</table>";

    textView_->setHtml(txt);
}

void LogStatRequestView::loadCore()
{
    buildTable();
}
