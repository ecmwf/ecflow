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

    void load(const std::string& logFile);

protected Q_SLOTS:
    void resolutionChanged(int);

private:
    void load();

    ServerLoadView* view_;
    Ui::LogLoadWidget* ui_;
    LogLoadSuiteModel* suiteModel_;
    QSortFilterProxyModel* suiteSortModel_;
    LogModel* logModel_;
};

struct LogLoadSuiteModelDataItem
{
    LogLoadSuiteModelDataItem(QString suiteName, float percentage, bool checked) :
        suiteName_(suiteName), percentage_(percentage), checked_(checked) {}

    QString suiteName_;
    float percentage_;
    bool checked_;
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

    QStringList suiteNames() const {return suites_;}
    const std::vector<LogLoadDataItem>& suites() const {return suiteData_;}
    void setTimeRes(TimeRes);
    void loadLogFile(const std::string& logFile);
    void getChildReq(QLineSeries& series);
    void getUserReq(QLineSeries& series);
    void getTotalReq(QLineSeries& series,int& maxVal);
    void getSuiteChildReq(size_t,QLineSeries& series);
    void getSuiteUserReq(size_t,QLineSeries& series);
    void getSuiteTotalReq(size_t,QLineSeries& series);

private:
    //Helper structure for data collection
    struct SuiteLoad {
       SuiteLoad(const std::string& name) : name_(name),
           childReq_(0),userReq_(0)  {}

       std::string name_;
       size_t   childReq_;
       size_t   userReq_;
    };

    void clear();
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
    std::vector<LogLoadDataItem> suiteData_; //suite-related data items

    QStringList suites_;
};

class ChartView : public QChartView
{
    Q_OBJECT
public:
    ChartView(QChart *chart, QWidget *parent);

    void doZoom(QRectF);


Q_SIGNALS:
    void chartZoomed(QRectF);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void adjustTimeAxis(qint64 periodInMs);
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

    void load(const std::string& logFile);
    void setResolution(LogLoadData::TimeRes);

protected Q_SLOTS:
    void slotZoom(QRectF);
    void addRemoveSuite(int idx, bool st);

protected:
    void load();
    void loadSuites();
    void build(QChart* chart,QLineSeries *series,int maxVal);

    QChart* chart_;
    ChartView* chartView_;
    QChart* chartUserReq_;
    QChart* chartChildReq_;
    QList<ChartView*> views_;

    LogLoadData* data_;
    QList<bool> suitePlotState_;
};

#endif // LOGLOADWIDGET_HPP
