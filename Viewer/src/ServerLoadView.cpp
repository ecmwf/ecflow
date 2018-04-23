//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerLoadView.hpp"

#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "File_r.hpp"
#include "File.hpp"
#include "NodePath.hpp"
#include "Str.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
//#include <QChartView>
#include <QLineSeries>

//using namespace QtCharts;

void ServerLoadData::clear()
{
    time_.clear();
    childReqPerSec_.clear();
    userReqPerSec_.clear();
    suiteTmp_.clear();
    suite_.clear();
    suiteNames_.clear();
}


void ServerLoadData::getChildReq(QLineSeries& series,TimeRes res)
{
    for(int i=0; i < time_.count(); i++)
        series.append(time_[i].toMSecsSinceEpoch(), childReqPerSec_[i]);
}

void ServerLoadData::getUserReq(QLineSeries& series,TimeRes res)
{
    for(int i=0; i < time_.count(); i++)
        series.append(time_[i].toMSecsSinceEpoch(), userReqPerSec_[i]);
}

void ServerLoadData::getTotalReq(QLineSeries& series,TimeRes res)
{
    for(int i=0; i < time_.count(); i++)
        series.append(time_[i].toMSecsSinceEpoch(), userReqPerSec_[i] + childReqPerSec_[i]);
}

void ServerLoadData::getSuiteReq(QString suiteName,QLineSeries& series,TimeRes res)
{
    int idx=suiteNames_.indexOf(suiteName);
    if(idx >=0)
    {
        for(int i=0; i < time_.count(); i++)
            series.append(time_[i].toMSecsSinceEpoch(), suite_[idx][i]);
    }
}

void ServerLoadData::add(std::vector<std::string> time_stamp,size_t child_requests_per_second,
                   size_t user_requests_per_second,std::vector<SuiteLoad>& suite_vec)
{
    if(time_stamp.size() < 2)
        return;

    QString s=QString::fromStdString(time_stamp[0]) + " " +
            QString::fromStdString(time_stamp[1]);
    time_ << QDateTime::fromString(s,"HH:mm:ss d.M.yyyy");

    childReqPerSec_ << child_requests_per_second;
    userReqPerSec_ << user_requests_per_second;

    QString sTxt;
    for(size_t i = 0; i < suite_vec.size(); i++)
    {
        if(!sTxt.isEmpty()) sTxt+"/";
        sTxt+=QString::number(suite_vec[i].request_per_second_);
    }
    suiteTmp_ << sTxt;
}

void ServerLoadData::collate(const std::vector<SuiteLoad>& suite_vec)
{
    int timeCnt=time_.count();
    if(timeCnt != childReqPerSec_.count())
    {
        clear();
        return;
    }

    if(timeCnt != userReqPerSec_.count())
    {
        clear();
        return;
    }
    if(timeCnt != suiteTmp_.count())
    {
        clear();
        return;
    }

    suiteNames_.clear();
    for(std::size_t i=0; i < suite_vec.size(); i++)
    {
       suite_[i]=QVector<std::size_t>(timeCnt,0);
       suiteNames_ << QString::fromStdString(suite_vec[i].suite_name_);
    }

    for(int i=0; i < timeCnt; i++)
    {
        QStringList lst=suiteTmp_[i].split("/");
        for(int j=0; j < lst.count(); j++)
        {
           suite_[j][i]=lst[j].toInt();
        }
    }

    suiteTmp_.clear();
}

void ServerLoadData::loadLogFile(const std::string& logFile )
{
   /// Will read the log file and create a new file that can be used as input to gnuplot
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
   /// -------------: TO:
   ///    1         2         3             4              5            6        7       8         9       10
   ///  HH:MM:SS D.M.YYYY request/sec  child_request  users_requests  suite_0 suite_1  suite_2  suite_3  suite_n

    clear();

    std::vector<SuiteLoad> suite_vec;

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
        throw std::runtime_error( "prepare_for_gnuplot: Could not open log file " + logFile );

   //gnuplot_file << "#time    date     total-request child user suite_0  suite_1 suite_2  suite_3  suite_n\n";

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;
    size_t child_requests_per_second = 0;
    size_t user_request_per_second = 0;
    unsigned int plot_data_line_number = 0;
    std::string line;
    while ( log_file.good() )
    {
        log_file.getline( line); // default delimiter is /n

        // The log file format we are interested is :
        // 0             1         2            3
        // MSG:[HH:MM:SS D.M.YYYY] chd:fullname [path +additional information]
        // MSG:[HH:MM:SS D.M.YYYY] --begin      [args | path(optional) ]    :<user>

        /// We are only interested in Commands (i.e MSG:), and not state changes
        if (line.empty())  continue;
        if (line[0] != 'M')  continue;
        std::string::size_type msg_pos = line.find("MSG:");
        if (msg_pos != 0)  continue;

        bool child_cmd = false;
        bool user_cmd = false;
        if (line.find(ecf::Str::CHILD_CMD()) != std::string::npos) child_cmd = true;
        else if (line.find(ecf::Str::USER_CMD()) != std::string::npos)  user_cmd = true;
        if (!child_cmd && !user_cmd) continue;

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
            if (new_time_stamp.size() != 2)  continue;

         line.erase(0,first_closed_bracket+1);
      }

      // Should be just left with " chd:<child command> " or " --<user command>, since we have remove time stamp
//#ifdef DEBUG
//      std::cout << line << "\n";
//#endif

      if (old_time_stamp.empty())
      {
         if (child_cmd)
             child_requests_per_second++;
         else
             user_request_per_second++;

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if ( suite_path_found )
             assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }
      else if (old_time_stamp[0] == new_time_stamp[0]) { // HH:MM:SS == HH:MM:SS
         if (child_cmd) child_requests_per_second++;
         else           user_request_per_second++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if ( suite_path_found ) assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
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

         add(old_time_stamp,child_requests_per_second,user_request_per_second,suite_vec);

         // clear request per second
         child_requests_per_second = 0;
         user_request_per_second = 0;
         for(size_t i= 0; i < suite_vec.size();i++)
            suite_vec[i].request_per_second_ = 0;

         // start of *new* time
         if (child_cmd)
             child_requests_per_second++;
         else
             user_request_per_second++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if ( suite_path_found )
             assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }

      old_time_stamp = new_time_stamp;
   }

   //if (plot_data_line_number < 3) {
   //   throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Log file empty or not enough data for plot\n");
   //}

   collate(suite_vec);
}

bool ServerLoadData::extract_suite_path(
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
         if (line.find("--news") != std::string::npos) return false;
      }

      // find the space after the path
      size_t space_pos = line.find(" ",forward_slash);
      if (space_pos != std::string::npos &&  space_pos > forward_slash)
      {
         path = line.substr(forward_slash,space_pos-forward_slash);
      }

      if (!path.empty())
      {
         std::vector<std::string> theNodeNames; theNodeNames.reserve(4);
         NodePath::split(path,theNodeNames);
         if (!theNodeNames.empty())
         {
            for(size_t n = 0; n < suite_vec.size(); n++)
            {
               if (suite_vec[n].suite_name_ == theNodeNames[0] )
               {
                  suite_vec[n].request_per_second_++;
                  suite_vec[n].total_request_per_second_++;
                  column_index = n;
                  return true;
               }
            }

            suite_vec.push_back( SuiteLoad(theNodeNames[0]) );
            column_index = suite_vec.size() - 1;
            return true;
         }
      }
   }
   return false;
}

ServerLoadView::ServerLoadView(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* hb=new QVBoxLayout(this);


    chart_ = new QChart();
    chartView_=new QChartView(chart_);
    chartView_->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartView_);


    chartUserReq_ = new QChart();
    QChartView* chartViewUser=new QChartView(chartUserReq_);
    chartViewUser->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartViewUser);

    chartChildReq_ = new QChart();
    QChartView* chartViewChild=new QChartView(chartChildReq_);
    chartViewChild->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartViewChild);
}

void ServerLoadView::load(const std::string& logFile)
{
    data_.loadLogFile(logFile);

    ServerLoadData::TimeRes res=ServerLoadData::SecondRes;

    QLineSeries* chSeries=new QLineSeries();
    data_.getChildReq(*chSeries,res);

    QLineSeries* usSeries=new QLineSeries();
    data_.getUserReq(*usSeries,res);

    QLineSeries* tSeries=new QLineSeries();
    data_.getTotalReq(*tSeries,res);


    build(chart_,tSeries);
    build(chartChildReq_,chSeries);
    build(chartUserReq_,usSeries);

#if 0
    //QChart *chart = new QChart();
    chart_->addSeries(tSeries);
    //chart_->addSeries(chSeries);
    //chart_->addSeries(usSeries);

    //chart_->legend()->hide();
    chart_->setTitle("Server load");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    axisX->setTitleText("Date");
    chart_->setAxisX(axisX, tSeries);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);


    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Requests per second");
    axisY->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chart_->setAxisY(axisY, tSeries);
    axisY->setMin(0.);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);

    chartU_->addSeries(chSeries);

    //chart_->legend()->hide();
    chart_->setTitle("Server load - child command");

    QDateTimeAxis *axisXU = new QDateTimeAxis;
    axisXU->setTickCount(10);
    axisXU->setFormat("HH dd/MM");
    axisXU->setTitleText("Date");
    chartU_->setAxisX(axisXU, chSeries);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);


    QValueAxis *axisYU = new QValueAxis;
    axisYU->setLabelFormat("%i");
    axisYU->setTitleText("Requests per second");
    axisYU->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chartU_->setAxisY(axisYU, chSeries);
    axisYU->setMin(0.);
    axisYU->setMax(171.);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);
#endif

}

void  ServerLoadView::build(QChart* chart,QLineSeries *series)
{
    chart->addSeries(series);

    chart->setTitle("Server load - child command");

    chart->legend()->hide();
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    axisX->setTitleText("Date");
    chart->setAxisX(axisX, series);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Requests per second");
    axisY->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chart->setAxisY(axisY, series);
    axisY->setMin(0.);
    axisY->setMax(171.);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);
}
