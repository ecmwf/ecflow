//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LogLoadData.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDateTime>
#include <QFileInfo>

//=======================================================
//
// LogReqCounter
//
//=======================================================

//Helper structure for data collection
LogReqCounter::LogReqCounter(const std::string& name) :
    name_(name), childReq_(0),userReq_(0)
{
    LogLoadDataItem::buildSubReq(childSubReq_,userSubReq_);
}

void LogReqCounter::clear()
{
    childReq_=0;
    userReq_=0;

    for(std::vector<LogRequestItem>::iterator it=childSubReq_.begin(); it != childSubReq_.end(); ++it)
        (*it).counter_=0;

    for(std::vector<LogRequestItem>::iterator it=userSubReq_.begin(); it != userSubReq_.end(); ++it)
        (*it).counter_=0;
}

void LogReqCounter::add(bool childCmd,const std::string& line)
{
    if(childCmd)
    {
        childReq_++;
        for(std::vector<LogRequestItem>::iterator it=childSubReq_.begin(); it != childSubReq_.end(); ++it)
        {
            if(line.find((*it).pattern_) != std::string::npos)
            {
                (*it).counter_++;
                return;
            }
        }
    }
    else
    {
        userReq_++;
        for(std::vector<LogRequestItem>::iterator it=userSubReq_.begin(); it != userSubReq_.end(); ++it)
        {
            if(line.find((*it).pattern_) != std::string::npos)
            {
                (*it).counter_++;
                return;
            }
        }
    }
}


void LogRequestItem::add(size_t v)
{
    sumTotal_+=v;
    req_.push_back(v);

    if(maxTotal_ < v)
        maxTotal_ = v;
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

LogLoadDataItem::LogLoadDataItem(const std::string& name) :
    subReqMax_(0), sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.), name_(name)
{
    buildSubReq();
}

LogLoadDataItem::LogLoadDataItem() :
    subReqMax_(0), sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.)
{
    buildSubReq();
}

void LogLoadDataItem::buildSubReq()
{
    buildSubReq(childSubReq_,userSubReq_);
}

void LogLoadDataItem::buildSubReq(std::vector<LogRequestItem>& childSubReq,
                                  std::vector<LogRequestItem>& userSubReq)
{
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildAbortType,"abort","chd:abort"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildCompleteType,"complete","chd:complete"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildEventType,"event","chd:event"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildInitType,"init","chd:init"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildLabelType,"label","chd:label"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildMeterType,"meter","chd:meter"));
    childSubReq.push_back(LogRequestItem(LogRequestItem::ChildWaitType,"wait","chd:wait"));

    userSubReq.push_back(LogRequestItem(LogRequestItem::UserAlterType,"alter","--alter="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserDeleteType,"delete","--delete="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserForceType,"force","--force="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserNewsType,"news","--news=",true));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserRequeueType,"requeue","--requeue="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserResumeType,"resume","--resume="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserSuspendType,"suspend","--suspend="));
    userSubReq.push_back(LogRequestItem(LogRequestItem::UserSyncType,"sync","--sync=",true));
}

void LogLoadDataItem::clear()
{
    sumTotal_=0;
    maxTotal_=0;
    rank_=-1;
    childReq_.clear();
    userReq_.clear();
    name_.clear();
    childSubReq_.clear();
    userSubReq_.clear();
    subReqMax_=0;

    buildSubReq();
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

        for(size_t i=0; i < childSubReq_.size(); i++)
        {
            childSubReq_[i].req_=std::vector<int>();
        }
        for(size_t i=0; i < userSubReq_.size(); i++)
        {
            userSubReq_[i].req_=std::vector<int>();
        }
    }
    else
    {
        childReq_=std::vector<int>(num,0);
        userReq_=std::vector<int>(num,0);

        for(size_t i=0; i < childSubReq_.size(); i++)
        {
            childSubReq_[i].req_=std::vector<int>(num,0);
        }
        for(size_t i=0; i < userSubReq_.size(); i++)
        {
            userSubReq_[i].req_=std::vector<int>(num,0);
        }
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

void LogLoadDataItem::add(const LogReqCounter& req)
{
    childReq_.push_back(static_cast<int>(req.childReq_));
    userReq_.push_back(static_cast<int>(req.userReq_));

    size_t tot=req.childReq_ + req.userReq_;
    sumTotal_+=tot;
    if(maxTotal_ < tot)
        maxTotal_ = tot;

    for(size_t i=0; i < childSubReq_.size(); i++)
    {
        childSubReq_[i].add(req.childSubReq_[i].counter_);
    }

    for(size_t i=0; i < userSubReq_.size(); i++)
    {
        userSubReq_[i].add(req.userSubReq_[i].counter_);
    }
}

void LogLoadDataItem::procSubReq(std::vector<LogRequestItem>& subReq)
{
    if(subReq.size() == 0)
        return;

    if(subReq.size() == 1)
    {
        subReq[0].rank_=0;
        subReq[0].percentage_=100.;
        return;
    }

    std::vector<size_t> sumVec;
    std::vector<std::pair<size_t,size_t> > sortVec;
    size_t sum=0;
    for(size_t i = 0; i < subReq.size(); i++)
    {
        sum+=subReq[i].sumTotal_;
        sumVec.push_back(subReq[i].sumTotal_);
        sortVec.push_back(std::make_pair(i,subReq[i].sumTotal_));
    }

    if(sum <=0 )
        return;

    assert(sumVec.size() == subReq.size());
    std::sort(sortVec.begin(), sortVec.end(),sortVecFunction);

    for(size_t i = 0; i < sortVec.size(); i++)
    {
        int idx=sortVec[i].first;
        subReq[idx].rank_=sortVec.size()-i-1;
        subReq[idx].percentage_=static_cast<float>(subReq[idx].sumTotal_*100)/static_cast<float>(sum);
    }
}

void LogLoadDataItem::postProc()
{
    procSubReq(childSubReq_);
    procSubReq(userSubReq_);

    subReqMax_=0;

    for(size_t i=0; i < childSubReq_.size(); i++)
    {
        if(subReqMax_ < childSubReq_[i].maxTotal_)
            subReqMax_ = childSubReq_[i].maxTotal_;
    }

    for(size_t i=0; i < userSubReq_.size(); i++)
    {
        if(subReqMax_ < userSubReq_[i].maxTotal_)
            subReqMax_ = userSubReq_[i].maxTotal_;
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
    total_.clear();
    suiteData_.clear();
    suites_.clear();
}

QString LogLoadData::childSubReqName(int idx) const
{
    return QString::fromStdString(total_.childSubReq()[idx].name_);
}

QString LogLoadData::userSubReqName(int idx) const
{
    return QString::fromStdString(total_.userSubReq()[idx].name_);
}

size_t LogLoadData::subReqMax() const
{
    return total_.subReqMax();
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
bool LogLoadData::indexOfTime(qint64 t,size_t& idx,size_t startIdx,qint64 tolerance) const
{
    idx=0;
    if(t < 0)
        return false;

    size_t num=time_.size();
    if(num == 0)
        return false;

    if(startIdx > num-1)
        startIdx=0;

    if(startIdx >= num)
        return false;

    if(t >= time_[startIdx])
    {
        if(tolerance <=0)
            tolerance=10*1000; //ms

        for(size_t i=startIdx; i < num; i++)
        {
            if(time_[i] >= t)
            {
                qint64 nextDelta=time_[i]-t;
                qint64 prevDelta=(i > 0)?(t-time_[i-1]):(nextDelta+1);
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

        for(size_t i=startIdx; i >=0; i--)
        {
            if(time_[i] <= t)
            {
                qint64 nextDelta=t-time_[i];
                qint64 prevDelta=(i < startIdx)?(time_[i+1]-t):(nextDelta+1);
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

void LogLoadData::getSeries(QLineSeries& series,const std::vector<int>& vals,int& maxVal)
{
    UI_ASSERT(time_.size() == vals.size(), "time_.size()=" << time_.size() << " vals.size()=" << vals.size());

    maxVal=0;
    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
        {
            if(vals[i] > maxVal) maxVal=vals[i];
            series.append(time_[i], vals[i]);
        }
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
                {
                    if(sum > maxVal) maxVal=sum;
                    series.append(time_[i],sum);
                }

                currentMinute=minute;
                sum=0;
            }
        }
    }
    else if(timeRes_ == HourResolution)
    {
        qint64 currentHour=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 hour=time_[i]/3600000;
            sum+=vals[i];
            if(currentHour != hour)
            {
                if(currentHour >0 )
                {
                    if(sum > maxVal) maxVal=sum;
                    series.append(time_[i],sum);
                }

                currentHour=hour;
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
    else if(timeRes_ == HourResolution)
    {
        maxVal=0;
        qint64 currentHour=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 hour=time_[i]/3600000;
            sum+=vals1[i] + vals2[i];
            if(currentHour != hour)
            {
                if(currentHour >0 )
                {
                    series.append(time_[i],sum);
                    if(maxVal < sum)
                        maxVal=sum;
                }
                currentHour=hour;
                sum=0;
            }
        }
    }
}

void LogLoadData::getChildReq(QLineSeries& series)
{
    int maxVal=0;
    getSeries(series,total_.childReq(),maxVal);
}

void LogLoadData::getUserReq(QLineSeries& series)
{
    int maxVal=0;
    getSeries(series,total_.userReq(),maxVal);
}

void LogLoadData::getTotalReq(QLineSeries& series,int& maxVal)
{
    getSeries(series,total_.childReq(),total_.userReq(),maxVal);
    if(timeRes_ == SecondResolution)
        maxVal=total_.maxTotal();
}

void LogLoadData::getSuiteChildReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[idx].childReq(),maxVal);
    }
}

void LogLoadData::getSuiteUserReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[idx].userReq(),maxVal);
    }
}

void LogLoadData::getSuiteTotalReq(size_t idx,QLineSeries& series)
{
    if(idx >=0 && idx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[idx].childReq(),suiteData_[idx].userReq(),maxVal);
    }
}

void LogLoadData::getSuiteChildSubReq(size_t suiteIdx,size_t subIdx,QLineSeries& series)
{
    if(suiteIdx >=0 && suiteIdx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[suiteIdx].childSubReq()[subIdx].req_,maxVal);
    }
}

void LogLoadData::getChildSubReq(size_t subIdx,QLineSeries& series,int& maxVal)
{
     getSeries(series,total_.childSubReq()[subIdx].req_,maxVal);
}

void LogLoadData::getUserSubReq(size_t subIdx,QLineSeries& series,int& maxVal)
{
    getSeries(series,total_.userSubReq()[subIdx].req_,maxVal);
}

void LogLoadData::getSuiteUserSubReq(size_t suiteIdx,size_t subIdx,QLineSeries& series)
{
    if(suiteIdx >=0 && suiteIdx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[suiteIdx].userSubReq()[subIdx].req_,maxVal);
    }
}

void LogLoadData::add(std::vector<std::string> time_stamp,const LogReqCounter& total,
                  const std::vector<LogReqCounter>& suite_vec)
{
    if(time_stamp.size() < 2)
        return;

    QString s=QString::fromStdString(time_stamp[0]) + " " +
            QString::fromStdString(time_stamp[1]);
    time_.push_back(QDateTime::fromString(s,"HH:mm:ss d.M.yyyy").toMSecsSinceEpoch());

    //all data
    //data_.add(total.childReq_,total.userReq_);
    total_.add(total);

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
        //suiteData_[i].add(suite_vec[i].childReq_,
        //                  suite_vec[i].userReq_);
        suiteData_[i].add(suite_vec[i]);
    }
}

void LogLoadData::processSuites()
{
    total_.postProc();

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
        suiteData_[idx].setRank(sortVec.size()-i-1);
        suiteData_[idx].setPercentage(static_cast<float>(suiteData_[idx].sumTotal()*100)/static_cast<float>(sum));
    }

    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        suiteData_[i].postProc();
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

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
        throw std::runtime_error("LogLoadData::loadLogFile: Could not open log file " + logFile );

    //A collector total counts
    LogReqCounter total("total");

    //A collector for suite related data
    std::vector<LogReqCounter> suite_vec;

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;
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
         total.add(child_cmd,line);

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }
      else if (old_time_stamp[0] == new_time_stamp[0])
      {
         // HH:MM:SS == HH:MM:SS
         total.add(child_cmd,line);

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

         //Add collected data to the storage objects
         add(old_time_stamp,total,suite_vec);

         // clear request per second
         total.clear();

         for(size_t i= 0; i < suite_vec.size();i++)
         {
            suite_vec[i].clear();
         }

         // start of *new* time
         total.add(child_cmd,line);

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
         std::vector<LogReqCounter>& suite_vec,
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
                        suite_vec[n].add(child_cmd,line);
                        column_index = n;
                        return true;
                    }
                }

                suite_vec.push_back( LogReqCounter(theNodeNames[0]) );
                column_index = suite_vec.size() - 1;
                suite_vec[column_index].add(child_cmd,line);
                return true;
         }
      }
   }
   return false;
}
