//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LOGLOADWIDGET_HPP
#define LOGLOADWIDGET_HPP

#include <map>
#include <string>
#include <vector>

#include <QAbstractItemModel>
#include <QGraphicsItem>
#include <QMap>
#include <QScrollArea>

#include <QStringList>
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QtCharts>
using namespace QtCharts;

#include "LogLoadData.hpp"

class LogLoadData;
class LogLoadDataItem;
class LogLoadSuiteModel;
class LogLoadRequestModel;
class LogModel;
class LogRequestView;
class LogRequestViewHandler;
class ServerLoadView;
class QSortFilterProxyModel;
class QVBoxLayout;
class QComboBox;
class QTextBrowser;

namespace Ui {
    class LogLoadWidget;
}

//the main widget containing all components
class LogLoadWidget : public QWidget
{
Q_OBJECT

public:
    explicit LogLoadWidget(QWidget *parent=0);
    ~LogLoadWidget();

    void clear();
    void load(QString logFile,int numOfRows=0);
    void load(QString serverName, QString host, QString port, QString logFile,int numOfRows=0);
    QString logFile() const {return logFile_;}

protected Q_SLOTS:
    void resolutionChanged(int);
    void currentTabChanged(int);
    void periodChanged(qint64 start,qint64 end);
    void periodWasReset();

private:
    void setAllVisible(bool);
    void setSuiteControlVisible(bool b);
    void setChildControlVisible(bool b);
    void setUserControlVisible(bool b);
    void load();
    void updateInfoLabel();

    enum TabIndex {TotalTab=0,SuiteTab=1,SubReqTab=2};

    LogRequestViewHandler *viewHandler_;
    //ServerLoadView* view_;
    QMap<TabIndex,ServerLoadView*> views_;
    Ui::LogLoadWidget* ui_;
    LogLoadRequestModel* suiteModel_;
    QSortFilterProxyModel* suiteSortModel_;
    LogLoadRequestModel* childReqModel_;
    QSortFilterProxyModel* childReqSortModel_;
    LogLoadRequestModel* userReqModel_;
    QSortFilterProxyModel* userReqSortModel_;
    LogModel* logModel_;

    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
};

struct LogLoadRequestModelDataItem
{
    LogLoadRequestModelDataItem(QString name, float percentage, bool checked,int rank) :
        name_(name), percentage_(percentage), checked_(checked) {}

    QString name_;
    float percentage_;
    bool checked_;
    QColor col_;
    int rank_;
};


struct LogLoadSuiteModelDataItem
{
    LogLoadSuiteModelDataItem(QString suiteName, float percentage, bool checked,int rank) :
        suiteName_(suiteName), percentage_(percentage), checked_(checked), rank_(rank) {}

    QString suiteName_;
    float percentage_;
    bool checked_;
    QColor col_;
    int rank_;
};

class LogLoadRequestSortModel : public QSortFilterProxyModel
{
public:
    LogLoadRequestSortModel(QObject* parent=0);

protected:
    bool lessThan(const QModelIndex &left,const QModelIndex &right) const;
};

//Model to dislay/select the suites
class LogLoadRequestModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LogLoadRequestModel(QString dataName,QObject *parent=0);
    ~LogLoadRequestModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex& idx, const QVariant & value, int role );
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(const std::vector<LogLoadDataItem>& data,QList<bool> checkedLst); //for suites
    void setData(const std::vector<LogRequestItem>& data,QList<bool> checkedLst); //for subrequests

    bool hasData() const;
    void clearData();

    void setShowColour(bool);

Q_SIGNALS:
    void checkStateChanged(int,bool);

public Q_SLOTS:
    void updateItem(int,bool,QColor);
    void unselectAll();
    void selectFirstFourItems();

protected:
    QString formatPrecentage(float perc) const;

    QString dataName_;
    QList<LogLoadRequestModelDataItem> data_;
    bool showColour_;
};

class ChartCallout : public QGraphicsItem
{
public:
    ChartCallout(QChart *parent);

    void setText(const QString &text);
    void setAnchor(QPointF point);
    QPointF anchor() const {return anchor_;}
    void updateGeometry();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,QWidget *widget);

private:
    QString text_;
    QRectF textRect_;
    QRectF rect_;
    QPointF anchor_;
    QPointF bottomPos_;
    QFont font_;
    QChart *chart_;
};

class ChartView : public QChartView
{
    Q_OBJECT
public:
    ChartView(QChart *chart, QWidget *parent);

    void doZoom(QRectF);
    void doZoom(qint64,qint64);
    void adjustTimeAxis(qint64 periodInMs);
    qint64 widthToTimeRange(float wPix);
    void currentTimeRange(qint64& start,qint64& end);
    void setCallout(qreal);
    void adjustCallout();
    void removeCallout();

Q_SIGNALS:
    void chartZoomed(QRectF);
    void positionChanged(qreal);
    void positionClicked(qreal);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    ChartCallout* callout_;
};

class LogRequestView;

class LogRequestViewHandler : public QObject
{
    Q_OBJECT

    friend class LogRequestView;
    friend class LogTotalRequestView;
    friend class LogSuiteRequestView;
    friend class LogSubRequestView;
    friend class LogStatRequestView;

public:
    LogRequestViewHandler(QWidget* parent);
    ~LogRequestViewHandler();

    QList<LogRequestView*> views() const {return views_;}

    LogLoadData* data() const {return data_;}
    void clear();
    void load(const std::string& logFile,int numOfRows=0);
    void setResolution(LogLoadData::TimeRes);
    QList<bool> suitePlotState() const {return suitePlotState_;}
    QList<bool> childPlotState() const {return childPlotState_;}
    QList<bool> userPlotState() const {return userPlotState_;}

public Q_SLOTS:
    void showFullRange();
    void addRemoveSuite(int idx, bool st);
    void addRemoveChildReq(int idx, bool st);
    void addRemoveUserReq(int idx, bool st);

protected Q_SLOTS:
    void slotZoomHappened(QRectF);

Q_SIGNALS:
    void scanDataChanged(QString);
    void suitePlotStateChanged(int,bool,QColor);
    void childPlotStateChanged(int,bool,QColor);
    void userPlotStateChanged(int,bool,QColor);
    void timeRangeChanged(qint64,qint64);
    void timeRangeHighlighted(qint64,qint64,qint64);
    void timeRangeReset();

protected:
    LogLoadData* data_;
    QList<LogRequestView*> views_;
    QList<bool> suitePlotState_;
    QList<bool> childPlotState_;
    QList<bool> userPlotState_;
    int lastScanIndex_;
};

class LogRequestView : public QScrollArea
{
    Q_OBJECT
public:
    explicit LogRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogRequestView();

    void clear();
    void load();
    void changeResolution();
    virtual void adjustZoom(QRectF r);

Q_SIGNALS:
    void scanDataChanged(QString);
    void suitePlotStateChanged(int,bool,QColor);
    void childPlotStateChanged(int,bool,QColor);
    void userPlotStateChanged(int,bool,QColor);
    void timeRangeChanged(qint64,qint64);
    void timeRangeHighlighted(qint64,qint64,qint64);
    void timeRangeReset();
    void zoomHappened(QRectF);

public Q_SLOTS:
    void showFullRange();
    virtual void addRemoveSuite(int idx, bool st) {}
    virtual void addRemoveChildReq(int idx, bool st) {}
    virtual void addRemoveUserReq(int idx, bool st) {}

protected Q_SLOTS:
    void slotZoom(QRectF);
    void adjustZoom();
    void scanPositionChanged(qreal);
    void scanPositionClicked(qreal);

protected:
    QChart* addChartById(QString id);
    void removeChartById(QString id);
    QString chartId(ChartView* cv);
    void clearCharts();
    void clearViews();
    virtual void loadCore()=0;
    void loadSuites();

    virtual void addSuite(int)=0;
    virtual void removeSuite(int)=0;
    virtual void addChildReq(int) {}
    virtual void removeChildReq(int) {}
    virtual void addUserReq(int) {}
    virtual void removeUserReq(int) {}

    int seriesValue(QChart* chart,QString id,int idx);
    QColor seriesColour(QChart* chart,QString id);
    bool seriesIndex(qint64 t,int startId,qint64 tolerance,int& idx);
    qint64 seriesTime(int idx);

    void adjustMaxVal();
    void build(ChartView* view,QLineSeries *series,QString title,int maxVal);

    void removeSeries(QChart* chart,QString id);

    virtual void buildScanTable(QString& txt,int idx)=0;
    void buildScanRow(QString &txt,QString name,size_t val,QColor col) const;
    void buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor col) const;
    void buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const;

    LogRequestViewHandler* handler_;
    QList<ChartView*> views_;
    QMap<QString,ChartView*> viewIds_;
    QVBoxLayout* mainLayout_;
    int maxVal_;
};

class LogTotalRequestView : public LogRequestView
{
    Q_OBJECT
public:
    explicit LogTotalRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogTotalRequestView() {}

public Q_SLOTS:
    void addRemoveSuite(int idx, bool st);

protected:
    enum ChartType {TotalChartType=0,ChildChartType=1,UserChartType=2};

    void loadCore();

    void addSuite(int);
    void removeSuite(int) {}

    QString suiteSeriesId(int idx) const;
    QChart* getChart(ChartType);
    ChartView* getView(ChartType);
    void buildScanTable(QString& txt,int idx);
};

class LogSuiteRequestView : public  LogRequestView
{
    Q_OBJECT
public:
    explicit LogSuiteRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogSuiteRequestView() {}

public Q_SLOTS:
    void addRemoveSuite(int idx, bool st);
    void addRemoveChildReq(int idx, bool st);
    void addRemoveUserReq(int idx, bool st);

protected:
    void loadCore();

    void addTotal();
    void addSuite(int);
    void removeSuite(int) {}
    void addChildReq(int childReqIdx);
    void removeChildReq(int childReqIdx);
    void addUserReq(int userReqIdx);
    void removeUserReq(int userReqIdx);

    QString childSeriesId(int childIdx) const;
    QString userSeriesId(int userIdx) const;
    QColor childSeriesColour(QChart* chart,size_t idx);
    QColor userSeriesColour(QChart* chart,size_t idx);
    void buildScanTable(QString& txt,int idx);
};

class LogSubRequestView : public  LogRequestView
{
    Q_OBJECT
public:
    explicit LogSubRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogSubRequestView() {}

public Q_SLOTS:
    void addRemoveSuite(int idx, bool st);
    void addRemoveChildReq(int idx, bool st);
    void addRemoveUserReq(int idx, bool st);

protected:
    void loadCore();

    void addSuite(int);
    void removeSuite(int);
    void addChildReq(int childReqIdx);
    void removeChildReq(int childReqIdx) {}
    void addUserReq(int userReqIdx);
    void removeUserReq(int userReqIdx) {}

    QString childChartId(int idx) const;
    QString userChartId(int idx) const;
    void parseChartId(QString id,QString& type,int& idx);

    QString suiteSeriesId(int suiteIdx) const;
    QColor suiteSeriesColour(QChart*,int suiteIdx);
    void buildScanTable(QString& txt,int idx);
};

class LogStatRequestView : public  LogRequestView
{
    Q_OBJECT
public:
    explicit LogStatRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogStatRequestView() {}

    void adjustZoom(QRectF r) {}

public Q_SLOTS:
    void addRemoveSuite(int idx, bool st);

protected:
    void loadCore();

    void buildTable();
    void addSuite(int) {}
    void removeSuite(int) {}
    void buildScanTable(QString& txt,int idx) {}

    QTextBrowser* textView_;
};

#endif // LOGLOADWIDGET_HPP
