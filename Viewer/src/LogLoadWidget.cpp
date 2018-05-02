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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSortFilterProxyModel>
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

    //Temporal resolution combo box
    ui_->resCombo->addItem("seconds",0);
    ui_->resCombo->addItem("minutes",0);

    connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(resolutionChanged(int)));

    //View + model to display/select suites
    suiteModel_=new LogLoadSuiteModel(this);
    suiteSortModel_=new QSortFilterProxyModel(this);
    suiteSortModel_->setSourceModel(suiteModel_);
    suiteSortModel_->setSortRole(Qt::UserRole);
    suiteSortModel_->setDynamicSortFilter(true);

    ui_->suiteTree->setRootIsDecorated(false);
    ui_->suiteTree->setAllColumnsShowFocus(true);
    ui_->suiteTree->setUniformRowHeights(true);
    ui_->suiteTree->setSortingEnabled(true);
    ui_->suiteTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->suiteTree->setModel(suiteSortModel_);
    //suiteModel_->setData(data_);

    connect(suiteModel_,SIGNAL(checkStateChanged(int,bool)),
            ui_->loadView,SLOT(addRemoveSuite(int,bool)));

    //Log contents

    logModel_=new LogModel(this);

    ui_->logView->setProperty("log","1");
    ui_->logView->setRootIsDecorated(false);
    ui_->logView->setModel(logModel_);
    ui_->logView->setUniformRowHeights(true);
    ui_->logView->setItemDelegate(new LogDelegate(this));
    ui_->logView->setContextMenuPolicy(Qt::ActionsContextMenu);

    //make the horizontal scrollbar work
    ui_->logView->header()->setStretchLastSection(false);
    ui_->logView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    //Define context menu
    //ui_->logView->addAction(actionCopyEntry_);
    //ui_->logView->addAction(actionCopyRow_);

    //Charts
    QColor bg(50,52,58);
    ui_->scanLabel->setStyleSheet("QLabel{background: " + bg.name() + ";}");

#if 0
    ui_->scanLabel->setAutoFillBackground(true);
    QPalette pal=ui_->scanLabel->palette();
    pal.setColor(ui_->scanLabel->backgroundRole(),QColor(50,52,58));
    ui_->scanLabel->setPalette(pal);
#endif
    connect(ui_->loadView,SIGNAL(scanDataChanged(QString)),
            ui_->scanLabel,SLOT(setText(QString)));
}

LogLoadWidget::~LogLoadWidget()
{
}

void LogLoadWidget::load(const std::string& logFile)
{
    //view_->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    std::string fileName="/home/graphics/cgr/ecflow_dev/vsms1.ecf.log";
    ui_->loadView->load(fileName);

    suiteModel_->setData(ui_->loadView->data(),ui_->loadView->suitePlotState());

    logModel_->loadFromFile(fileName);
}

void LogLoadWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        ui_->loadView->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        ui_->loadView->setResolution(LogLoadData::MinuteResolution);
}


//=====================================================
//
//  LogLoadSuiteModel
//
//=====================================================

LogLoadSuiteModel::LogLoadSuiteModel(QObject *parent) :
          QAbstractItemModel(parent)
{
}

LogLoadSuiteModel::~LogLoadSuiteModel()
{
}

void LogLoadSuiteModel::setData(LogLoadData* data,QList<bool> checkedLst)
{
    Q_ASSERT(data);
    Q_ASSERT(data->suites().size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data->suites().size(); i++)
    {
        data_ << LogLoadSuiteModelDataItem(QString::fromStdString(data->suites()[i].name()),
                                           data->suites()[i].percentage(),checkedLst[i]);
    }

    endResetModel();
}


void LogLoadSuiteModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

bool LogLoadSuiteModel::hasData() const
{
    return !data_.isEmpty();
}

int LogLoadSuiteModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 2;
}

int LogLoadSuiteModel::rowCount( const QModelIndex& parent) const
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

Qt::ItemFlags LogLoadSuiteModel::flags ( const QModelIndex & index) const
{
    Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == 0)
    {
        defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;
    }
    return defaultFlags;
}

QVariant LogLoadSuiteModel::data( const QModelIndex& index, int role ) const
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
            return data_[row].suiteName_;
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
            return data_[row].suiteName_;
            break;
        case 1:
            return data_[row].percentage_;
            break;
        default:
            break;
        }
    }

    return QVariant();
}

bool LogLoadSuiteModel::setData(const QModelIndex& idx, const QVariant & value, int role )
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

QVariant LogLoadSuiteModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch ( section )
        {
        case 0: return tr("Suite");
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    else if(role== Qt::ToolTipRole)
    {
        switch ( section )
        {
        case 0: return tr("Suite");
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex LogLoadSuiteModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex LogLoadSuiteModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QString LogLoadSuiteModel::formatPrecentage(float perc) const
{
    if(perc < 0.5)
        return "<0.5";

    return QString::number(perc,'f',1);
}

//=======================================================
//
// LogLoadDataItem
//
//=======================================================

bool sortVecFunction(const std::pair<size_t,size_t>& a, const std::pair<size_t,size_t>& b)
{
    return a.second < b.second;
}

void LogLoadDataItem::clear()
{
    sumTotal_=0;
    maxTotal_=0;
    rank_=-1;
    childReq_.clear();
    userReq_.clear();
    name_.clear();
}

void LogLoadDataItem::init(size_t num)
{
    sumTotal_=0;
    maxTotal_=0;
    rank_=-1;
    if(num==0)
    {
        childReq_=std::vector<int>();
        userReq_=std::vector<int>();
    }
    else
    {
        childReq_=std::vector<int>(num,0);
        userReq_=std::vector<int>(num,0);
    }
}

void LogLoadDataItem::valuesAt(size_t idx,size_t& total,size_t& child,size_t& user) const
{
    if(idx >=0 && idx < size())
    {
        child=childReq_[idx];
        user=userReq_[idx];
        total=child+user;
    }
}

//=======================================================
//
// LogLoadData
//
//=======================================================

void LogLoadData::clear()
{
    time_.clear();
    data_.clear();
    suiteData_.clear();
    suites_.clear();
}

void LogLoadData::setTimeRes(TimeRes res)
{
    timeRes_=res;
}

qint64 LogLoadData::period() const
{
    return (!time_.empty())?(time_[time_.size()-1]-time_[0]):0;
}

//t is in ms
bool LogLoadData::indexOfTime(qint64 t,size_t& idx) const
{
    idx=0;
    if(t < 0)
        return false;

    size_t num=time_.size();
    qint64 tolerance=10*1000;
    qint64 prevDelta=0;
    for(size_t i=0; i < num; i++)
    {
        if(time_[i] < t)
        {
            prevDelta=t-time_[i];
        }
        else if(t == time_[i])
        {
            idx=i;
            return true;
        }
        else if(time_[i] > t)
        {
            qint64 nextDelta=time_[i]-t;
            if(i > 0)
            {
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
            }
            if(nextDelta <= tolerance)
            {
                idx=i;
                return true;
            }

            return false;
        }
    }
    return false;

}

void LogLoadData::getSeries(QLineSeries& series,const std::vector<int>& vals)
{
    UI_ASSERT(time_.size() == vals.size(), "time_.size()=" << time_.size() << "vals.size()=" << vals.size());

    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], vals[i]);
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=vals[i];
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                    series.append(time_[i],sum);

                currentMinute=minute;
                sum=0;
            }
        }
    }
}

void LogLoadData::getSeries(QLineSeries& series,const std::vector<int>& vals1,const std::vector<int>& vals2,int & maxVal)
{
    UI_ASSERT(time_.size() == vals1.size(), "time_.size()=" << time_.size() << " vals1.size()=" << vals1.size());
    UI_ASSERT(time_.size() == vals2.size(), "time_.size()=" << time_.size() << " vals2.size()=" << vals2.size());

    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], vals1[i] + vals2[i]);
    }
    else if(timeRes_ == MinuteResolution)
    {
        maxVal=0;
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=vals1[i] + vals2[i];
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                {
                    series.append(time_[i],sum);
                    if(maxVal < sum)
                        maxVal=sum;
                }
                currentMinute=minute;
                sum=0;
            }
        }
    }
}

void LogLoadData::getChildReq(QLineSeries& series)
{
    getSeries(series,data_.childReq());
}

void LogLoadData::getUserReq(QLineSeries& series)
{
    getSeries(series,data_.userReq());
}

void LogLoadData::getTotalReq(QLineSeries& series,int& maxVal)
{
    getSeries(series,data_.childReq(),data_.userReq(),maxVal);
    if(timeRes_ == SecondResolution)
        maxVal=data_.maxTotal();
}

void LogLoadData::getSuiteChildReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal;
        getSeries(series,suiteData_[idx].childReq());
    }
}

void LogLoadData::getSuiteUserReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal;
        getSeries(series,suiteData_[idx].userReq());
    }
}

void LogLoadData::getSuiteTotalReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal;
        getSeries(series,suiteData_[idx].childReq(),suiteData_[idx].userReq(),maxVal);
    }
}

void LogLoadData::add(std::vector<std::string> time_stamp,size_t child_requests_per_second,
                   size_t user_requests_per_second,std::vector<SuiteLoad>& suite_vec)
{
    if(time_stamp.size() < 2)
        return;

    QString s=QString::fromStdString(time_stamp[0]) + " " +
            QString::fromStdString(time_stamp[1]);
    time_.push_back(QDateTime::fromString(s,"HH:mm:ss d.M.yyyy").toMSecsSinceEpoch());

    //all data
    data_.add(child_requests_per_second,user_requests_per_second);

    //suite specific data
    for(size_t i = suiteData_.size(); i < suite_vec.size(); i++)
    {
        suiteData_.push_back(LogLoadDataItem(suite_vec[i].name_));
        suiteData_.back().init(time_.size()-1);
    }

    assert(suiteData_.size() == suite_vec.size());

    //suite specific data
    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        suiteData_[i].add(suite_vec[i].childReq_,
                          suite_vec[i].userReq_);
    }
}

void LogLoadData::processSuites()
{
    if(suiteData_.size() == 0)
        return;

    if(suiteData_.size() == 1)
    {
        suiteData_[0].setRank(0);
        suiteData_[0].setPercentage(100.);
        return;
    }

    std::vector<size_t> sumVec;
    std::vector<std::pair<size_t,size_t> > sortVec;
    size_t sum=0;
    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        sum+=suiteData_[i].sumTotal();
        sumVec.push_back(suiteData_[i].sumTotal());
        sortVec.push_back(std::make_pair(i,suiteData_[i].sumTotal()));
    }

    if(sum <=0 )
        return;

    assert(sumVec.size() == suiteData_.size());
    std::sort(sortVec.begin(), sortVec.end(),sortVecFunction);

    for(size_t i = 0; i < sortVec.size(); i++)
    {
        int idx=sortVec[i].first;
        suiteData_[idx].setRank(i);
        suiteData_[idx].setPercentage(static_cast<float>(suiteData_[idx].sumTotal()*100)/static_cast<float>(sum));
    }
}

void LogLoadData::loadLogFile(const std::string& logFile )
{
   /// Will read the log file.
   /// We will collate each request/cmd to the server made over a second.
   /// There are two kinds of commands:
   ///   o User Commands: these start with --
   ///   o Child Command: these start with chd:
   /// All child commands specify a path and hence suite, whereas for user commands this is optional
   /// We will trap all use of paths, so that we can show which suites are contributing to the server load
   /// This will be done for 4 suites
   ///
   /// Will convert: FROM:
   ///   XXX:[HH:MM:SS D.M.YYYY] chd:init [+additional information]
   ///   XXX:[HH:MM:SS D.M.YYYY] --begin  [+additional information]
   /// -------------:

    //Clear all collected data
    clear();

    //A collector fr suite related data
    std::vector<SuiteLoad> suite_vec;

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
        throw std::runtime_error("LogLoadData::loadLogFile: Could not open log file " + logFile );

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;
    size_t childReq = 0, userReq = 0 ;
    std::string line;

    while ( log_file.good() )
    {
        log_file.getline( line); // default delimiter is /n

        // The log file format we are interested is :
        // 0             1         2            3
        // MSG:[HH:MM:SS D.M.YYYY] chd:fullname [path +additional information]
        // MSG:[HH:MM:SS D.M.YYYY] --begin      [args | path(optional) ]    :<user>

        /// We are only interested in Commands (i.e MSG:), and not state changes
        if (line.empty())
            continue;

        if (line[0] != 'M')
            continue;

        std::string::size_type msg_pos = line.find("MSG:");
        if (msg_pos != 0)
            continue;

        bool child_cmd = false;
        bool user_cmd = false;
        if (line.find(ecf::Str::CHILD_CMD()) != std::string::npos)
        {
            child_cmd = true;
        }
        else if (line.find(ecf::Str::USER_CMD()) != std::string::npos)
        {
            user_cmd = true;
        }

        if(!child_cmd && !user_cmd)
            continue;

        new_time_stamp.clear();
        {
            /// MSG:[HH:MM:SS D.M.YYYY] chd:fullname [+additional information] ---> HH:MM:SS D.M.YYYY
            /// EXTRACT the date
            std::string::size_type first_open_bracket = line.find('[');
            if ( first_open_bracket == std::string::npos)
            {
                //std::cout << line << "\n";
                assert(false);
                continue;
            }
            line.erase(0,first_open_bracket+1);

            std::string::size_type first_closed_bracket = line.find(']');
            if ( first_closed_bracket ==  std::string::npos)
            {
                //std::cout << line << "\n";
                assert(false);
                continue;
            }
            std::string time_stamp = line.substr(0, first_closed_bracket);

            ecf::Str::split(time_stamp, new_time_stamp);
            if (new_time_stamp.size() != 2)
                continue;

         line.erase(0,first_closed_bracket+1);
      }

      // Should be just left with " chd:<child command> " or " --<user command>, since we have remove time stamp
//#ifdef DEBUG
//      std::cout << line << "\n";
//#endif

      if (old_time_stamp.empty())
      {
         if (child_cmd)
             childReq++;
         else
             userReq++;

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }
      else if (old_time_stamp[0] == new_time_stamp[0])
      { // HH:MM:SS == HH:MM:SS
         if (child_cmd)
             childReq++;
         else
             userReq++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }



      else {
         /// Start of *NEW* time,
         /// write the *OLD* time line should contain time date without []
         ///    1         2         3             4              5            6        7      8       9       10
         ///  HH:MM:SS D.M.YYYY total_request child_request  users_requests suite_0 suite_1 suite_2 suite_3 suite_n
#if 0
         //plot.add(old_time_stamp[0],old_time_stamp[1],child_requests_per_second, user_request_per_second,


         plot_data_line_number++;
         gnuplot_file << old_time_stamp[0] << " "
                      << old_time_stamp[1] << " "
                      << (child_requests_per_second + user_request_per_second) << " "
                      << child_requests_per_second << " "
                      << user_request_per_second << " ";
         for(size_t i = 0; i < suite_vec.size(); i++) { gnuplot_file << suite_vec[i].request_per_second_ << " ";}
         gnuplot_file << "\n";
#endif

         add(old_time_stamp,childReq,userReq,suite_vec);

         // clear request per second
         childReq = 0;
         userReq = 0;
         for(size_t i= 0; i < suite_vec.size();i++)
         {
            suite_vec[i].childReq_ = 0;
            suite_vec[i].userReq_ = 0;
         }

         // start of *new* time
         if (child_cmd)
             childReq++;
         else
             userReq++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }

      old_time_stamp = new_time_stamp;
   }

   //if (plot_data_line_number < 3) {
   //   throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Log file empty or not enough data for plot\n");
   //}

   for(size_t i=0; i < suite_vec.size(); i++)
   {
       suites_ << QString::fromStdString(suite_vec[i].name_);
   }

   processSuites();
}

bool LogLoadData::extract_suite_path(
         const std::string& line,
         bool child_cmd,
         std::vector<SuiteLoad>& suite_vec,
         size_t& column_index   // 0 based
         )
{
    // line should either
    //  chd:<childcommand> path
    //  --<user command)   path<optional> :<user>
    size_t forward_slash = line.find('/');
    if ( forward_slash != std::string::npos)
    {
        std::string path;
        if (child_cmd)
        {
            // For labels ignore paths in the label part
            // MSG:[14:55:04 17.10.2013] chd:label progress 'core/nodeattr/nodeAParser' /suite/build/cray/cray_gnu/build_release/test
            if (line.find("chd:label") != std::string::npos)
            {
                size_t last_tick = line.rfind("'");
                if ( last_tick != std::string::npos )
                {
                    size_t the_forward_slash = line.find('/',last_tick);
                    if (the_forward_slash != std::string::npos)
                    {
                        forward_slash = the_forward_slash;
                    }
                }
            }
            path = line.substr(forward_slash);
        }
        else
        {
            // Ignore the --news command, they dont have a path, hence i.e to ignore line like:
            //  MSG:[09:36:05 22.10.2013] --news=1 36506 6  :ma0 [server handle(36508,7) server(36508,7)
            //                     : *Large* scale changes (new handle or suites added/removed) :NEWS]
            //   the /removed was being interpreted as a suite
            if (line.find("--news") != std::string::npos)
                return false;
        }

        // find the space after the path
        size_t space_pos = line.find(" ",forward_slash);
        if (space_pos != std::string::npos &&  space_pos > forward_slash)
        {
            path = line.substr(forward_slash,space_pos-forward_slash);
        }

        if (!path.empty())
        {
            std::vector<std::string> theNodeNames;
            theNodeNames.reserve(4);
            NodePath::split(path,theNodeNames);
            if (!theNodeNames.empty())
            {
                for(size_t n = 0; n < suite_vec.size(); n++)
                {
                    if (suite_vec[n].name_ == theNodeNames[0] )
                    {
                        if(child_cmd)
                            suite_vec[n].childReq_++;
                        else
                            suite_vec[n].userReq_++;

                        //suite_vec[n].total_request_per_second_++;
                        column_index = n;
                        return true;
                    }
                }

                suite_vec.push_back( SuiteLoad(theNodeNames[0]) );
                column_index = suite_vec.size() - 1;
                if(child_cmd)
                    suite_vec[column_index].childReq_++;
                else
                    suite_vec[column_index].userReq_++;

                return true;
         }
      }
   }
   return false;
}

//=============================================
//
// ChartView
//
//=============================================

ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent)
{
    setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    Q_EMIT positionChanged(chart()->mapToValue(event->pos()).x());
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);

    if(event->pos().x() <= chart()->plotArea().right() &&
       event->pos().x() >= chart()->plotArea().left())
        Q_EMIT positionChanged(chart()->mapToValue(event->pos()).x());
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

void ChartView::adjustTimeAxis(qint64 periodInMs)
{
    qint64 period=periodInMs/1000; //in seconds
    QString format;

    if(period < 60)
    {
        format="hh:mm:ss";
    }
    else if(period < 24*3600)
    {
        format="hh:mm";
    }
    else if(period < 24*5*3600)
    {
        format="hh dd/MM";
    }
    else
    {
        format="DD/MM";
    }

    if(QDateTimeAxis *ax=static_cast<QDateTimeAxis*>(chart()->axisX()))
    {
        ax->setFormat(format);
    }
}

//=============================================
//
// ServerLoadView
//
//=============================================

ServerLoadView::ServerLoadView(QWidget* parent) : QWidget(parent), data_(NULL)
{
    //The data object - to read and store processed log data
    data_=new LogLoadData();

    QVBoxLayout* vb=new QVBoxLayout(this);
    for(int i=0; i < 3; i++)
    {
        QChart* chart = new QChart();
        ChartView* chartView=new ChartView(chart,this);
        chartView->setRenderHint(QPainter::Antialiasing);
        vb->addWidget(chartView);
        views_ << chartView;

        connect(chartView,SIGNAL(chartZoomed(QRectF)),
                this,SLOT(slotZoom(QRectF)));

        connect(chartView,SIGNAL(positionChanged(float)),
                this,SLOT(scanPositionChanged(float)));
    }

    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
}

ServerLoadView::~ServerLoadView()
{
    Q_ASSERT(data_);
    delete data_;
}

QChart* ServerLoadView::getChart(ChartType type)
{
    if(ChartView *v=getView(type))
        return v->chart();
    return 0;
}

ChartView* ServerLoadView::getView(ChartType type)
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

void ServerLoadView::slotZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);
        }
    }
}

void ServerLoadView::setResolution(LogLoadData::TimeRes res)
{
    data_->setTimeRes(res);
    load();
    loadSuites();
}

void ServerLoadView::addRemoveSuite(int idx, bool st)
{
    if(idx >= 0 && idx < static_cast<int>(data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            suitePlotState_[idx]=true;

            QLineSeries* series=new QLineSeries();
            series->setName("s_main_" + QString::number(idx));
            data_->getSuiteTotalReq(idx,*series);
            getChart(TotalChartType)->addSeries(series);

            QLineSeries* chSeries=new QLineSeries();
            chSeries->setName("s_ch_" + QString::number(idx));
            data_->getSuiteChildReq(idx,*chSeries);
            getChart(ChildChartType)->addSeries(chSeries);

            QLineSeries* usSeries=new QLineSeries();
            usSeries->setName("s_us_" + QString::number(idx));
            data_->getSuiteUserReq(idx,*usSeries);
            getChart(UserChartType)->addSeries(usSeries);
        }
        //remove
        else
        {
            suitePlotState_[idx]=false;

            removeSuiteSeries(getChart(TotalChartType),
                              "s_main_" + QString::number(idx));

            removeSuiteSeries(getChart(ChildChartType),
                              "s_ch_" + QString::number(idx));

            removeSuiteSeries(getChart(UserChartType),
                              "s_us_" + QString::number(idx));
        }
    }
}

void ServerLoadView::removeSuiteSeries(QChart* chart,QString id)
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

QColor ServerLoadView::suiteSeriesColour(QChart* chart,size_t idx)
{
    QString id="_" + QString::number(idx);
    return seriesColour(chart,id);
}

QColor ServerLoadView::seriesColour(QChart* chart,QString id)
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

void ServerLoadView::clear()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        v->chart()->removeAllSeries();
        v->chart()->removeAxis(v->chart()->axisX());
        v->chart()->removeAxis(v->chart()->axisY());
    }
}

void ServerLoadView::load(const std::string& logFile)
{
    data_->loadLogFile(logFile);

    load();
    loadSuites();

    suitePlotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suitePlotState_ << false;

}

void ServerLoadView::load()
{
    clear();

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

void ServerLoadView::loadSuites()
{
    for(int i=0; i < suitePlotState_.count(); i++)
    {
        if(suitePlotState_[i])
        {
            addRemoveSuite(i,true);
        }
    }
}

void  ServerLoadView::build(ChartView* view,QLineSeries *series, QString title,int maxVal)
{
    Q_ASSERT(view);
    QChart *chart=view->chart();
    Q_ASSERT(chart);

    chart->addSeries(series);
    chart->setTitle(title);

    chart->legend()->hide();
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    //axisX->setTitleText("Date");
    chart->setAxisX(axisX, series);
    view->adjustTimeAxis(data_->period());

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");

    QString yTitle;
    if(data_->timeRes() == LogLoadData::SecondResolution)
        yTitle="Req. per second";
    else if(data_->timeRes() == LogLoadData::MinuteResolution)
        yTitle="Req. per minute";

    axisY->setTitleText(yTitle);
    axisY->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chart->setAxisY(axisY, series);
    axisY->setMin(0.);
    axisY->setMax(maxVal);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);

    //connect(series, &QLineSeries::hovered, this, &View::tooltip);
}

void ServerLoadView::scanPositionChanged(float pos)
{
    qint64 t(pos);
    size_t idx=0;
    if(data_->indexOfTime(t,idx))
    {
        QString txt="date: " + QDateTime::fromMSecsSinceEpoch(data_->time()[idx]).toString("hh:mm:ss dd/MM/yyyy");
        txt+="<table width=\'100%\' cellpadding=\'2px\'>";
        //header
        txt+="<tr><th>Item</th><th>Total</th><th>Child</th><th>User</th></tr>";

        size_t tot=0,ch=0,us=0;
        QColor col=seriesColour(getChart(TotalChartType),"all");
        QString name="all";
        data_->dataItem().valuesAt(idx,tot,ch,us);
        buildScanRow(txt,name,tot,ch,us,col);

        for(int i=0; i < suitePlotState_.size(); i++)
        {
            if(suitePlotState_[i])
            {
                tot=0;ch=0;us=0;
                QColor col=suiteSeriesColour(getChart(TotalChartType),i);
                QString name=QString::fromStdString(data_->suites()[i].name());
                data_->suites()[i].valuesAt(idx,tot,ch,us);
                buildScanRow(txt,name,tot,ch,us,col);
            }
        }

        txt+="</table>";

        Q_EMIT scanDataChanged(txt);
    }
}

void ServerLoadView::buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor lineCol) const
{
    //txt+="<tr bgcolor=\'" + col.lighter(120).name() + "\'><td>" +
    QColor numBg(230,230,230);
    txt+="<tr>" + Viewer::formatTableTd(name,lineCol.lighter(150)) +
          Viewer::formatTableTd(QString::number(tot),numBg) +
          Viewer::formatTableTd(QString::number(ch),numBg) +
          Viewer::formatTableTd(QString::number(us),numBg) + "</tr>";
#if 0
    txt+="<tr><td bgcolor=\'" + col.lighter(150).name() + "\'>&nbsp;" +
         name + "</td><td>&nbsp;" + QString::number(tot) +
         "</td><td>&nbsp;" + QString::number(ch) +
         "</td><td>&nbsp;" + QString::number(us) + "</td></tr>";
#endif
}

#if 0
void View::tooltip(QPointF point, bool state)
{
    if (m_tooltip == 0)
        m_tooltip = new Callout(m_chart);

    if (state) {
        m_tooltip->setText(QString("X: %1 \nY: %2 ").arg(point.x()).arg(point.y()));
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}
#endif
