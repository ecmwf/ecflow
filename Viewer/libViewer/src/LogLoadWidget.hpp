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

#include <string>
#include <vector>

#include <QAbstractItemModel>
#include <QGraphicsItem>
#include <QStringList>
#include <QWidget>

#include <QtCharts>
using namespace QtCharts;

class LogLoadData;
class LogLoadSuiteModel;
class LogModel;
class ServerLoadView;
class QSortFilterProxyModel;

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

    ServerLoadView* view_;
    Ui::LogLoadWidget* ui_;
    LogLoadSuiteModel* suiteModel_;
    QSortFilterProxyModel* suiteSortModel_;
    LogModel* logModel_;

    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
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

//Data structure containing load/request statistics
class LogLoadDataItem
{
public:
    LogLoadDataItem(const std::string& name) : sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.), name_(name) {}
    LogLoadDataItem() : sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.) {}

    void clear();
    void init(size_t num);

    size_t size() const {return childReq_.size();}
    float percentage() const {return percentage_;}
    void setPercentage(float v) {percentage_=v;}
    size_t sumTotal() const {return sumTotal_;}
    size_t maxTotal() const {return maxTotal_;}
    int rank() const {return rank_;}
    void setRank(int v) {rank_=v;}
    const std::vector<int>& childReq() const {return childReq_;}
    const std::vector<int>& userReq() const {return userReq_;}
    const std::string& name() const {return name_;}
    void valuesAt(size_t idx,size_t& total,size_t& child,size_t& user) const;

    void add(size_t childVal,size_t userVal)
    {
        childReq_.push_back(static_cast<int>(childVal));
        userReq_.push_back(static_cast<int>(userVal));

        size_t tot=childVal + userVal;
        sumTotal_+=tot;
        if(maxTotal_ < tot)
            maxTotal_ = tot;
    }

protected:
    std::vector<int> childReq_; //per seconds
    std::vector<int> userReq_;  //per seconds
    size_t sumTotal_; //sum of all the child and user requests
    size_t maxTotal_; //the maximum value of child+user requests
    int rank_; //the rank of this item within other items with regards to sumTotal_
    float percentage_; //0-100, the percentage of sumTotal_ with respect to the
                       //sum of sumTotal_ of all the items

    std::string name_; //the name of the item (only makes sense for suites)
};

//The top level class for load/request statistics
class LogLoadData
{
public:
    enum TimeRes {SecondResolution, MinuteResolution};

    LogLoadData() : timeRes_(SecondResolution)  {}

    void clear();
    const LogLoadDataItem& dataItem() const {return data_;}
    QStringList suiteNames() const {return suites_;}
    const std::vector<LogLoadDataItem>& suites() const {return suiteData_;}
    TimeRes timeRes() const {return timeRes_;}
    void setTimeRes(TimeRes);
    void loadLogFile(const std::string& logFile);
    void getChildReq(QLineSeries& series);
    void getUserReq(QLineSeries& series);
    void getTotalReq(QLineSeries& series,int& maxVal);
    void getSuiteChildReq(size_t,QLineSeries& series);
    void getSuiteUserReq(size_t,QLineSeries& series);
    void getSuiteTotalReq(size_t,QLineSeries& series);
    const std::vector<qint64>& time() const {return time_;}
    qint64 period() const;
    bool indexOfTime(qint64 t,size_t&,size_t,qint64) const;

private:
    //Helper structure for data collection
    struct SuiteLoad {
       SuiteLoad(const std::string& name) : name_(name),
           childReq_(0),userReq_(0)  {}

       std::string name_;
       size_t   childReq_;
       size_t   userReq_;
    };

    void getSeries(QLineSeries& series,const std::vector<int>& vals);
    void getSeries(QLineSeries& series,const std::vector<int>& vals1,const std::vector<int>& vals2,int& maxVal);
    void add(std::vector<std::string> time_stamp,size_t childReq,
             size_t userReq,std::vector<SuiteLoad>& suite_vec);

    void processSuites();

    bool extract_suite_path(const std::string& line,bool child_cmd,std::vector<SuiteLoad>& suite_vec,
                            size_t& column_index);

    TimeRes timeRes_;
    std::vector<qint64> time_; //times stored as msecs since the epoch
    LogLoadDataItem data_; //generic data item for
    std::vector<LogLoadDataItem> suiteData_; //suite,true-related data items

    QStringList suites_;
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



#endif // LOGLOADWIDGET_HPP
