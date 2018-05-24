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
#include <QStringList>
#include <QWidget>

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
    void load(QString logFile);
    void load(QString serverName, QString host, QString port, QString logFile);
    QString logFile() const {return logFile_;}

protected Q_SLOTS:
    void resolutionChanged(int);

private:
    void setAllVisible(bool);
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

    //void setData(LogLoadData* data,QList<bool>); //for subrequests
    void setData(const std::vector<LogLoadDataItem>& data,QList<bool> checkedLst); //for suites
    void setData(const std::vector<LogRequestItem>& data,QList<bool> checkedLst); //for subrequests

    bool hasData() const;
    void clearData();

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
};



//Model to dislay/select the suites
class LogLoadSuiteModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LogLoadSuiteModel(QObject *parent=0);
    ~LogLoadSuiteModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex& idx, const QVariant & value, int role );
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(LogLoadData* data,QList<bool>);
    bool hasData() const;
    void clearData();

Q_SIGNALS:
    void checkStateChanged(int,bool);

public Q_SLOTS:
    void updateSuite(int,bool,QColor);
    void unselectAllSuites();
    void selectFirstFourSuites();

protected:
    QString formatPrecentage(float perc) const;

    QList<LogLoadSuiteModelDataItem> data_;
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

class ServerLoadView : public QWidget
{
    Q_OBJECT
public:
    explicit ServerLoadView(QWidget* parent=0);
    ~ServerLoadView();

    void setData(LogLoadData*);
    LogLoadData* data() const {return data_;}
    QList<bool> suitePlotState() const {return suitePlotState_;}

    void clear();
    void load(const std::string& logFile);
    void setResolution(LogLoadData::TimeRes);

Q_SIGNALS:
    void scanDataChanged(QString);
    void suitePlotStateChanged(int,bool,QColor);
    void timeRangeChanged(qint64,qint64);
    void timeRangeHighlighted(qint64,qint64,qint64);
    void timeRangeReset();

public Q_SLOTS:
    void showFullRange();

protected Q_SLOTS:
    void slotZoom(QRectF);
    void addRemoveSuite(int idx, bool st);
    void scanPositionChanged(qreal);
    void scanPositionClicked(qreal);

protected:
    enum ChartType {TotalChartType=0,ChildChartType=1,UserChartType=2};

    void clearCharts();
    void load();
    void loadSuites();
    void addSuite(int);
    void build(ChartView* view,QLineSeries *series,QString title,int maxVal);
    void removeSuiteSeries(QChart* chart,QString id);
    QChart* getChart(ChartType);
    ChartView* getView(ChartType);
    QColor suiteSeriesColour(QChart* chart,size_t idx);
    QColor seriesColour(QChart* chart,QString id);
    void buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor col) const;
    void buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const;

    QList<ChartView*> views_;
    LogLoadData* data_;
    QList<bool> suitePlotState_;
    size_t lastScanIndex_;
};

class LogRequestView;


class LogRequestViewHandler : public QObject
{
    Q_OBJECT

    friend class LogRequestView;
    friend class LogTotalRequestView;
    friend class LogSuiteRequestView;

public:
    LogRequestViewHandler(QWidget* parent);
    ~LogRequestViewHandler();

    QList<LogRequestView*> views() const {return views_;}

    LogLoadData* data() const {return data_;}
    void clear();
    void load(const std::string& logFile);
    void setResolution(LogLoadData::TimeRes);
    QList<bool> suitePlotState() const {return suitePlotState_;}
    QList<bool> childPlotState() const {return childPlotState_;}
    QList<bool> userPlotState() const {return userPlotState_;}

public Q_SLOTS:
    void showFullRange();
    void addRemoveSuite(int idx, bool st);
    void addRemoveChildReq(int idx, bool st);

Q_SIGNALS:
    void scanDataChanged(QString);
    void suitePlotStateChanged(int,bool,QColor);
    void timeRangeChanged(qint64,qint64);
    void timeRangeHighlighted(qint64,qint64,qint64);
    void timeRangeReset();

protected:
    LogLoadData* data_;
    QList<LogRequestView*> views_;
    QList<bool> suitePlotState_;
    QList<bool> childPlotState_;
    QList<bool> userPlotState_;
    size_t lastScanIndex_;
};

class LogRequestView : public QWidget
{
    Q_OBJECT
public:
    explicit LogRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogRequestView();

    //void setData(LogLoadData*);
    //LogLoadData* data() const {return data_;}
    //QList<bool> suitePlotState() const {return suitePlotState_;}

    void clear();
    void load();
    void changeResolution();

Q_SIGNALS:
    void scanDataChanged(QString);
    void suitePlotStateChanged(int,bool,QColor);
    void timeRangeChanged(qint64,qint64);
    void timeRangeHighlighted(qint64,qint64,qint64);
    void timeRangeReset();

public Q_SLOTS:
    void showFullRange();
    virtual void addRemoveSuite(int idx, bool st) {}
    virtual void addRemoveChildReq(int idx, bool st) {}

protected Q_SLOTS:
    void slotZoom(QRectF);
    void scanPositionChanged(qreal);
    void scanPositionClicked(qreal);

protected:
    //enum ChartType {TotalChartType=0,ChildChartType=1,UserChartType=2};

    QChart* addChartById(int id);
    void removeChartById(int id);
    void clearCharts();
    virtual void loadCore()=0;
    void loadSuites();
    virtual void addSuite(int)=0;
    void build(ChartView* view,QLineSeries *series,QString title,int maxVal);
    void removeSuiteSeries(QChart* chart,QString id);
    //QChart* getChart(ChartType);
    //ChartView* getView(ChartType);
    QColor suiteSeriesColour(QChart* chart,size_t idx);
    QColor seriesColour(QChart* chart,QString id);
    void buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor col) const;
    void buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const;

    LogRequestViewHandler* handler_;
    QList<ChartView*> views_;
    QMap<int,ChartView*> viewIds_;
    QVBoxLayout* mainLayout_;
    //LogLoadData* data_;
    //QList<bool> suitePlotState_;
    //size_t lastScanIndex_;
};




class LogTotalRequestView : public LogRequestView
{
    Q_OBJECT
public:
    explicit LogTotalRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogTotalRequestView() {}

    //void setData(LogLoadData*);
    //LogLoadData* data() const {return data_;}
    //QList<bool> suitePlotState() const {return suitePlotState_;}

    //void clear();
    //void load(const std::string& logFile);
    //void setResolution(LogLoadData::TimeRes);

//Q_SIGNALS:
    //void scanDataChanged(QString);
    //void suitePlotStateChanged(int,bool,QColor);
    //void timeRangeChanged(qint64,qint64);
    //void timeRangeHighlighted(qint64,qint64,qint64);
    //void timeRangeReset();

//public Q_SLOTS:
//    void showFullRange();

public Q_SLOTS:
    //void slotZoom(QRectF);
    void addRemoveSuite(int idx, bool st);
    //void scanPositionChanged(qreal);
    //void scanPositionClicked(qreal);

protected:
    enum ChartType {TotalChartType=0,ChildChartType=1,UserChartType=2};

    //void clearCharts();
    void loadCore();
    //void loadSuites();
    void addSuite(int);
    //void build(ChartView* view,QLineSeries *series,QString title,int maxVal);
    //void removeSuiteSeries(QChart* chart,QString id);
    QChart* getChart(ChartType);
    ChartView* getView(ChartType);
    //QColor suiteSeriesColour(QChart* chart,size_t idx);
    //QColor seriesColour(QChart* chart,QString id);
    //void buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor col) const;
    //void buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const;

    //QList<ChartView*> views_;
    //LogLoadData* data_;
    //QList<bool> suitePlotState_;
    //size_t lastScanIndex_;
};

class LogSuiteRequestView : public  LogRequestView
{
    Q_OBJECT
public:
    explicit LogSuiteRequestView(LogRequestViewHandler* handler,QWidget* parent=0);
    ~LogSuiteRequestView() {}

    //void clear();
    //void load(const std::string& logFile);
    //void setResolution();

//Q_SIGNALS:
    //void scanDataChanged(QString);
    //void suitePlotStateChanged(int,bool,QColor);
    //void timeRangeChanged(qint64,qint64);
    //void timeRangeHighlighted(qint64,qint64,qint64);
    //void timeRangeReset();

public Q_SLOTS:
    //void showFullRange();

public Q_SLOTS:
    //void slotZoom(QRectF);
    void addRemoveSuite(int idx, bool st);
    void addRemoveChildReq(int idx, bool st);
    //void scanPositionChanged(qreal);
    //void scanPositionClicked(qreal);

protected:
    //enum ChartType {TotalChartType=0,ChildChartType=1,UserChartType=2};

    //void clearCharts();
    void loadCore();
    //void loadSuites();

    void addSuite(int);
    int  chartId(ChartView* cv);
    void addChildReq(int childReqIdx);

    //void build(ChartView* view,QLineSeries *series,QString title,int maxVal);
    //void removeSuiteSeries(QChart* chart,QString id);
    //QChart* getChart(ChartType);
    //ChartView* getView(ChartType);
    //QColor suiteSeriesColour(QChart* chart,size_t idx);
    //QColor seriesColour(QChart* chart,QString id);
    //void buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor col) const;
    //void buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const;

    //LogRequestViewHandler* handler_;
    //QList<ChartView*> views_;

    //QVBoxLayout* mainLayout_;
    //LogLoadData* data_;
    //QList<bool> suitePlotState_;
    //size_t lastScanIndex_;
};


#endif // LOGLOADWIDGET_HPP
