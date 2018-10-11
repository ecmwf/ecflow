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
#include <QStringList>

static int REQCNT=0;

//=======================================================
//
// LogReqCounter
//
//=======================================================

//Helper structure for data collection
LogReqCounter::LogReqCounter(const std::string& name) :
    name_(name), childReq_(0),userReq_(0)
{
    LogLoadDataItem::buildSubReq(subReq_);
}

void LogReqCounter::clear()
{
    childReq_=0;
    userReq_=0;

    for(std::vector<LogRequestItem>::iterator it=subReq_.begin(); it != subReq_.end(); ++it)
        (*it).counter_=0;
}

void LogReqCounter::add(bool childCmd,const std::string& line)
{
    if(childCmd)
    {
        childReq_++;
        /*for(std::vector<LogRequestItem>::iterator it=childSubReq_.begin(); it != childSubReq_.end(); ++it)
        {
            if(line.find((*it).pattern_) != std::string::npos)
            {
                (*it).counter_++;
                return;
            }
        }*/
    }
    else
    {
        userReq_++;
        /*for(size_t i=0; i < userSubReq_.size(); i++)
        {
            if(line.find(userSubReq_[i].pattern_) != std::string::npos)
            {
                userSubReq_[i].counter_++;
                return;
            }
        }*/
    }

    for(std::vector<LogRequestItem>::iterator it=subReq_.begin(); it != subReq_.end(); ++it)
    {
        if(line.find((*it).pattern_) != std::string::npos)
        {
            (*it).counter_++;
            return;
        }
    }
}

//=======================================================
//
// LogRequestItem
//
//=======================================================

void LogRequestItem::add(size_t index,size_t v)
{
    //stat_.sumTotal_+=v;
    index_.push_back(index);
    req_.push_back(v);

    //if(stat_.maxTotal_ < v)
    //    stat_.maxTotal_ = v;
}

void LogRequestItem::computeStat(size_t startIndex,size_t endIndex)
{
    int max=0;
    size_t sum=0;
    for(size_t i=0; i < index_.size(); i++)
    {
        if(index_[i] >= startIndex && index_[i] <= endIndex)
        {
            sum+=req_[i];
            if(req_[i] > max)
                max=req_[i];
        }
    }

    periodStat_.maxTotal_=max;
    periodStat_.sumTotal_=sum;
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
   name_(name)
{
    buildSubReq(subReq_);
}

LogLoadDataItem::LogLoadDataItem()
{
    buildSubReq(subReq_);
}

void LogLoadDataItem::buildSubReq(std::vector<LogRequestItem>& subReq)
{
    //clientOptionsDescriptions_ = new po::options_description("help", po::options_description::m_default_line_length + 80);

    QStringList chCmd;
    chCmd << "abort" << "complete" << "event" << "init" << "label" << "meter" << "wait";

    Q_FOREACH(QString s,chCmd)
    {
         subReq.push_back(LogRequestItem(s.toStdString(),LogRequestItem::ChildCmd,
                                         "chd:" + s.toStdString()));
    }

    QStringList usCmd;
    usCmd <<   "alter"          << "--alter " <<
               "begin"          << "--begin=" <<
               "ch_add"         << "--ch_add=" <<
               "ch_auto_add"    << "--ch_auto_add=" <<
               "ch_drop_user"   << "--ch_drop_user=" <<
               "ch_register"    << "--ch_register=" <<
               "ch_rem"         << "--ch_red=" <<
               "ch_suites"      << "--ch_suites=" <<
               "check"          << "--check=" <<
               "checkJobGenOnly" << "--checkJobGenOnly" <<
               "check_pt"       << "svr:check_pt " <<
               "debug"          << "--debug" <<
               "debug_server_off" << "--debug_server_off" <<
               "debug_server_on"  << "--debug_server_on" <<
               "delete"         << "--delete " <<
               "edit_history"   << "--edit_history" <<
               "edit_script"    << "--edit_script=" <<
               "file"           << "--file=" <<
               "force"          << "--force=" <<
               "force-dep-eval" << "--force-dep-eval" <<
               "free-dep"       << "--free-dep" <<
               "get"            << "--get" <<
               "get_state"      << "--get_state" <<
               "group"          << "--group=" <<
               "halt"           << "--halt" <<
               "help"           << "--help" <<
               "host"           << "--host " <<
               "job_gen"        << "--job_gen" <<
               "kill"           << "--kill " <<
               "load"           << "--load=" <<
               "log"            << "--log=" <<
               "migrate"        << "--migrate" <<
               "msg"            << "--msg=" <<
               "news"           << "--news=" <<
               "order"          << "--order=" <<
               "ping"           << "--ping " <<
               "plug"           << "--plug=" <<
               "port"           << "--port " <<
               "reloadwsfile"   << "--reloadwsfile " <<
               "replace"        << "--replace=" <<
               "requeue"        << "--requeue " <<
               "restart"        << "--restart " <<
               "restore_from_checkpt" << "--restore_from_checkpt " <<
               "resume"         << "--resume " <<
               "rid"            << "--rid" <<
               "run"            << "--run " <<
               "server_load"    << "--server_load" <<
               "server_version" << "--server_version " <<
               "shutdown"       << "--shutdown" <<
               "stats"          << "--stats " <<
               "stats_reset"    << "--stats_reset" <<
               "status"         << "--status=" <<
               "suites"         << "--suites " <<
               "suspend"        << "--suspend=" <<
               "sync"           << "--sync=" <<
               "sync_full"      << "--sync_full=" <<
               "terminate"      << "--terminate" <<
               "version"        << "--version" <<
               "zombie_adopt"   << "--zombie_adopt=" <<
               "zombie_block"   << "--zombie_block=" <<
               "zombie_fail"    << "--zombie_fail=" <<
               "zombie_fob"     << "--zombie_fob=" <<
               "zombie_get"     << "--zombie_get=" <<
               "zombie_kill"    << "--zombie_kill=" <<
               "zombie_remove"  << "--zombie_remove=" ;

    Q_ASSERT(usCmd.count() % 2 == 0);
    for(int i=0; i < usCmd.count() ; i+=2)
    {
         subReq.push_back(LogRequestItem(usCmd[i].toStdString(),
                                             LogRequestItem::UserCmd,
                                             usCmd[i+1].toStdString()));
    }
}

void LogLoadDataItem::clear()
{
    childReq_.clear();
    userReq_.clear();
    name_.clear();
    subReq_.clear();
    periodStat_.clear();
    fullStat_.clear();
    subReqMax_=0;

    buildSubReq(subReq_);
}

void LogLoadDataItem::init(size_t num)
{
    if(num==0)
    {
       childReq_=std::vector<int>();
       userReq_=std::vector<int>();

#if 0
        for(size_t i=0; i < childSubReq_.size(); i++)
        {
            childSubReq_[i].req_=std::vector<int>();
        }
        for(size_t i=0; i < userSubReq_.size(); i++)
        {
            userSubReq_[i].req_=std::vector<int>();
        }
#endif
    }
    else
    {
        childReq_=std::vector<int>(num,0);
        userReq_=std::vector<int>(num,0);
#if 0
        for(size_t i=0; i < childSubReq_.size(); i++)
        {
            childSubReq_[i].req_=std::vector<int>(num,0);
        }
        for(size_t i=0; i < userSubReq_.size(); i++)
        {
            userSubReq_[i].req_=std::vector<int>(num,0);
        }
#endif
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

void LogLoadDataItem::add(size_t index,const LogReqCounter& req)
{
    childReq_.push_back(static_cast<int>(req.childReq_));
    userReq_.push_back(static_cast<int>(req.userReq_));

    //size_t tot=req.childReq_ + req.userReq_;
    //stat_.sumTotal_+=tot;
    //if(stat_.maxTotal_ < tot)
    //    stat_.maxTotal_ = tot;

    for(size_t i=0; i < subReq_.size(); i++)
    {
        if(req.subReq_[i].counter_ > 0)
            subReq_[i].add(index,req.subReq_[i].counter_);
    }
}

void LogLoadDataItem::computeSubReqStat(std::vector<LogRequestItem>& subReq,
                                 size_t startIndex,size_t endIndex)
{
    for(size_t i = 0; i < subReq.size(); i++)
    {
        subReq[i].computeStat(startIndex,endIndex);
    }

    if(subReq.size() == 0)
        return;

    if(subReq.size() == 1)
    {
        //subReq[0].rank_=0;
        subReq[0].periodStat_.percentage_=100.;
        return;
    }

    //std::vector<size_t> sumVec;
    std::vector<std::pair<size_t,size_t> > sortVec;
    std::vector<size_t> sumVec;
    size_t sum=0;
    for(size_t i = 0; i < subReq.size(); i++)
    {
        sum+=subReq[i].periodStat_.sumTotal_;
        sumVec.push_back(subReq[i].periodStat_.sumTotal_);
        sortVec.push_back(std::make_pair(i,subReq[i].periodStat_.sumTotal_));
    }

    if(sum <=0 )
        return;

    assert(sumVec.size() == subReq.size());
    std::sort(sortVec.begin(), sortVec.end(),sortVecFunction);

    for(size_t i = 0; i < subReq.size(); i++)
    {     
        int idx=sortVec[i].first;
        subReq[idx].periodStat_.rank_=sortVec.size()-i-1;
        subReq[idx].periodStat_.percentage_=static_cast<float>(subReq[idx].periodStat_.sumTotal_*100)/static_cast<float>(sum);
    }
}

void LogLoadDataItem::saveFullStat()
{
    fullStat_=periodStat_;
    for(size_t i=0; i < subReq_.size(); i++)
    {
        subReq_[i].fullStat_=subReq_[i].periodStat_;
    }
}

void LogLoadDataItem::useFullStat()
{
    periodStat_=fullStat_;
    for(size_t i=0; i < subReq_.size(); i++)
    {
        subReq_[i].periodStat_=subReq_[i].fullStat_;
    }

    computeSubReqMax();
}

void LogLoadDataItem::computeStat(size_t startIndex,size_t endIndex)
{   
    size_t max=0;
    size_t sum=0;

    if(childReq_.size() != 0)
    {
        for(size_t i=startIndex; i <= endIndex; i++)
        {
            size_t tot=childReq_[i] + userReq_[i];
            sum+=tot;
            if(max < tot)
                max = tot;
        }
    }

    periodStat_.sumTotal_=sum;
    periodStat_.maxTotal_=max;

    computeSubReqStat(subReq_,startIndex,endIndex);

    computeSubReqMax();
}

void LogLoadDataItem::computeSubReqMax()
{
    subReqMax_=0;

    for(size_t i=0; i < subReq_.size(); i++)
    {
        if(subReqMax_ < subReq_[i].periodStat_.maxTotal_)
            subReqMax_ = subReq_[i].periodStat_.maxTotal_;
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
    uidData_.clear();
    fullStatComputed_=false;
    maxNumOfRows_=0;
    numOfRows_=0;
    startPos_=0;
}

QString LogLoadData::subReqName(int idx) const
{
    return QString::fromStdString(total_.subReq()[idx].name_);
}

QString LogLoadData::uidName(int idx) const
{
    return QString::fromStdString(uidData_[idx].name());
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

QDateTime LogLoadData::startTime() const
{
    return (time_.empty())?QDateTime():QDateTime::fromMSecsSinceEpoch(time_[0]);
}

QDateTime LogLoadData::endTime() const
{
    return (time_.empty())?QDateTime():QDateTime::fromMSecsSinceEpoch(time_[time_.size()-1]);
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

void LogLoadData::getSeries(QLineSeries& series,const LogRequestItem& item,int& maxVal)
{
    UI_ASSERT(item.index_.size() == item.req_.size(), "item.index_.size()=" << item.index_.size() <<
              "  item.req_.size()=" <<  item.req_.size());

    maxVal=0;
    if(timeRes_ == SecondResolution)
    {
        size_t itemIndex=0;
        QVector<size_t> vals(time_.size(),0);
        for(size_t i=0; i < item.index_.size() ; i++)
        {
            vals[item.index_[i]]=item.req_[i];
            if(maxVal < item.req_[i])
                maxVal=item.req_[i];
        }
        for(size_t i=0; i < time_.size(); i++)
        {
            series.append(time_[i], vals[i]);
        }
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        size_t idx=0;
        size_t itemSize=item.index_.size();
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;

            if(itemSize > 0  && item.index_[idx] == i)
            {
                sum+=item.req_[idx];
                idx++;
            }

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
        size_t idx=0;
        size_t itemSize=item.index_.size();
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 hour=time_[i]/3600000;

            if(itemSize > 0 && item.index_[idx] == i)
            {
                sum+=item.req_[idx];
                idx++;
            }

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


void LogLoadData::getSuiteSubReq(size_t suiteIdx,size_t subIdx,QLineSeries& series)
{
    if(suiteIdx >=0 && suiteIdx < suites().size())
    {
        int maxVal=0;
        getSeries(series,suiteData_[suiteIdx].subReq()[subIdx],maxVal);
    }
}

void LogLoadData::getSubReq(size_t subIdx,QLineSeries& series,int& maxVal)
{
     getSeries(series,total_.subReq()[subIdx],maxVal);
}

void LogLoadData::getUidTotalReq(size_t uidIdx,QLineSeries& series,int& maxVal)
{
    if(uidIdx >=0 && uidIdx < uidData().size())
    {
        //the childreq  = 0 for the uidData!
        maxVal=0;
        getSeries(series,uidData_[uidIdx].userReq(),maxVal);
    }
}

void LogLoadData::getUidSubReq(size_t uidIdx,size_t subIdx,QLineSeries& series)
{
    if(uidIdx >=0 && uidIdx < uidData().size())
    {
        int maxVal=0;
        getSeries(series,uidData_[uidIdx].subReq()[subIdx],maxVal);
    }
}

void LogLoadData::add(std::vector<std::string> time_stamp,const LogReqCounter& total,
                  const std::vector<LogReqCounter>& suite_vec,
                  const std::vector<LogReqCounter>& uid_vec)
{
    if(time_stamp.size() < 2)
        return;

    QString s=QString::fromStdString(time_stamp[0]) + " " +
            QString::fromStdString(time_stamp[1]);
    time_.push_back(QDateTime::fromString(s,"HH:mm:ss d.M.yyyy").toMSecsSinceEpoch());

    size_t index=time_.size()-1;

    //all data
    //data_.add(total.childReq_,total.userReq_);
    total_.add(index,total);

    //---------------------------
    // uid specific data
    //---------------------------
    for(size_t i = uidData_.size(); i < uid_vec.size(); i++)
    {
        uidData_.push_back(LogLoadDataItem(uid_vec[i].name_));
        uidData_.back().init(time_.size()-1);
    }

    assert(uidData_.size() == uid_vec.size());
    for(size_t i = 0; i < uidData_.size(); i++)
    {
        uidData_[i].add(index,uid_vec[i]);
    }

    //---------------------------
    //suite specific data
    //---------------------------
    for(size_t i = suiteData_.size(); i < suite_vec.size(); i++)
    {
        suiteData_.push_back(LogLoadDataItem(suite_vec[i].name_));
        suiteData_.back().init(time_.size()-1);
    }

    assert(suiteData_.size() == suite_vec.size());
    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        //suiteData_[i].add(suite_vec[i].childReq_,
        //                  suite_vec[i].userReq_);
        suiteData_[i].add(index,suite_vec[i]);
    }
}

//Compute the initial stat
void LogLoadData::computeInitialStat()
{
    size_t endIndex=(time_.empty())?0:(time_.size()-1);
    computeStat(0,endIndex);
    fullStatComputed_=true;
}

void LogLoadData::computeStat()
{
    size_t endIndex=(time_.empty())?-1:(time_.size()-1);
    computeStat(0,endIndex);
}

void LogLoadData::computeStat(size_t startIndex,size_t endIndex)
{
    bool fullPeriod=(startIndex == 0 && (time_.size() ==0 || endIndex == time_.size()-1));

    //Compute stats for total
    if(fullPeriod && fullStatComputed_)
    {
        total_.useFullStat();
    }
    else
    {
        total_.computeStat(startIndex,endIndex);
        if(!fullStatComputed_)
            total_.saveFullStat();
    }

    //Suites
    computeStat(suiteData_,startIndex,endIndex,fullPeriod);

    //Uids
    computeStat(uidData_,startIndex,endIndex,fullPeriod);
}

void LogLoadData::computeStat(std::vector<LogLoadDataItem>& items,size_t startIndex,size_t endIndex,
                              bool fullPeriod)
{
    if(fullPeriod && fullStatComputed_)
    {
        for(size_t i = 0; i < items.size(); i++)
        {
            items[i].useFullStat();
        }
        return;
    }

    for(size_t i = 0; i < items.size(); i++)
    {
        items[i].computeStat(startIndex,endIndex);
    }

    if(items.size() == 0)
        return;

    if(items.size() == 1)
    {
        items[0].setRank(0);
        items[0].setPercentage(100.);
        return;
    }

    std::vector<size_t> sumVec;
    std::vector<std::pair<size_t,size_t> > sortVec;
    size_t sum=0;
    for(size_t i = 0; i < items.size(); i++)
    {
        sum+=items[i].sumTotal();
        sumVec.push_back(items[i].sumTotal());
        sortVec.push_back(std::make_pair(i,items[i].sumTotal()));
    }

    if(sum <=0 )
        return;

    assert(sumVec.size() == items.size());
    std::sort(sortVec.begin(), sortVec.end(),sortVecFunction);

    for(size_t i = 0; i < sortVec.size(); i++)
    {
        int idx=sortVec[i].first;
        items[idx].setRank(sortVec.size()-i-1);
        items[idx].setPercentage(static_cast<float>(items[idx].sumTotal()*100.)/static_cast<float>(sum));
    }

    if(fullPeriod && !fullStatComputed_)
    {
        for(size_t i = 0; i < items.size(); i++)
        {
            items[i].saveFullStat();
        }
        return;
    }
}

void LogLoadData::loadLogFile(const std::string& logFile,int numOfRows)
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

    maxNumOfRows_=numOfRows;
    numOfRows_=0;
    startPos_=getStartPos(logFile,numOfRows);

#if 0
    if(numOfRows < 0)
    {
        /// The log file can be massive > 50Mb
        ecf::File_r log_file(logFile);
        if( !log_file.ok() )
            throw std::runtime_error("LogLoadData::loadLogFile: Could not open log file " + logFile );

        int maxNum=-numOfRows;
        int cnt=0;
        std::vector<std::streamoff> posVec(maxNum,0);
        startPos_=0;
        while(log_file.good())
        {
            std::streamoff currentPos=log_file.pos();

            log_file.getline(line); // default delimiter is /n

            /// We are only interested in Commands (i.e MSG:), and not state changes
            if (line.empty())
                continue;

            if (line[0] != 'M')
                continue;

            std::string::size_type msg_pos = line.find("SG:");
            if (msg_pos != 1)
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

            posVec[cnt % maxNum]=currentPos;
            cnt++;
        }

        startPos_=posVec[cnt % maxNum];
        //log_file.setPos(startPos_);
    }
#endif

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
    {
        UiLog().warn() << "LogLoadData::loadLogFile: Could not open log file " <<  logFile;
        return;
    }

    //A collector total counts
    LogReqCounter total("total");

    //A collector for suite related data
    std::vector<LogReqCounter> suite_vec;

    //A collector for uid related data
    std::vector<LogReqCounter> uid_vec;

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;

    std::map<std::string,size_t> uidCnt;
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

#if 0
        std::string uid;
        if(user_cmd)
        {
            std::string::size_type uidStart = line.find(" :");
            if ( uidStart != std::string::npos)
            {
                std::string::size_type uidEnd = line.find(" ",uidStart+2);
                if(uidEnd != std::string::npos)
                {
                    uid=line.substr(uidStart+2,uidEnd-uidStart+1);
                }
                else
                {
                    uid = line.substr(uidStart+2);
                }
            }
        }
#endif

        new_time_stamp.clear();
        {
            /// MSG:[HH:MM:SS D.M.YYYY] chd:fullname [+additional information] ---> HH:MM:SS D.M.YYYY
            /// EXTRACT the date
            std::string::size_type first_open_bracket = line.find('[');
            if ( first_open_bracket == std::string::npos)
            {
                //std::cout << line << "\n";
                //assert(false);
                continue;
            }
            line.erase(0,first_open_bracket+1);

            std::string::size_type first_closed_bracket = line.find(']');
            if ( first_closed_bracket ==  std::string::npos)
            {
                //std::cout << line << "\n";
                //assert(false);
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

      numOfRows_++;

      if (old_time_stamp.empty())
      {
         total.add(child_cmd,line);

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if(user_cmd)
         {
            extract_uid_data(line,uid_vec);
         }
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }
      else if (old_time_stamp[0] == new_time_stamp[0])
      {
         // HH:MM:SS == HH:MM:SS
         total.add(child_cmd,line);

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if(user_cmd)
         {
            extract_uid_data(line,uid_vec);
         }
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
         add(old_time_stamp,total,suite_vec,uid_vec);

         // clear request per second
         total.clear();

         for(size_t i= 0; i < suite_vec.size();i++)
         {
            suite_vec[i].clear();
         }
         for(size_t i= 0; i < uid_vec.size();i++)
         {
            uid_vec[i].clear();
         }

         // start of *new* time
         total.add(child_cmd,line); //?

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
         if(user_cmd)
         {
            extract_uid_data(line,uid_vec);
         }

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

   computeInitialStat();

   UiLog().dbg() << "REQCNT " << REQCNT;
}

std::streamoff LogLoadData::getStartPos(const std::string& logFile, int numOfRows)
{
    std::streamoff startPos=0;
    std::string line;

    if(numOfRows < 0)
    {
        /// The log file can be massive > 50Mb
        ecf::File_r log_file(logFile);
        if( !log_file.ok() )
        {
            UiLog().warn() << "LogLoadData::getStartPos: Could not open log file " <<  logFile;
            return startPos;
        }

        int maxNum=-numOfRows;
        int cnt=0;
        std::vector<std::streamoff> posVec(maxNum,0);
        while(log_file.good())
        {
            std::streamoff currentPos=log_file.pos();

            log_file.getline(line); // default delimiter is /n

            /// We are only interested in Commands (i.e MSG:), and not state changes
            if (line.size() < 4)
                continue;

            if (line[0] != 'M' || line[1] != 'S' || line[2] != 'G' || line[3] != ':')
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

            posVec[cnt % maxNum]=currentPos;
            cnt++;
        }
        startPos=posVec[cnt % maxNum];
    }

    return startPos;
}


bool LogLoadData::extract_uid_data(const std::string& line,std::vector<LogReqCounter>& uid_vec)
{
   std::string uid;
   std::string::size_type uidStart = line.find(" :");

   bool hasIt=false;
   if(line.find(":newops") != std::string::npos)
   {
       hasIt=true;
       REQCNT++;
   }

   if ( uidStart != std::string::npos)
   {
       uidStart+=2;
       for(size_t i=uidStart; i < line.size() ; i++)
       {
            if(line[i] == ' ')
            {
                if(i > uidStart+1)
                {
                    uid=line.substr(uidStart,i-uidStart);
                }
                break;
            }
            else if(line[i] == ';')
            {
               if(i > uidStart+1)
               {
                   uid=line.substr(uidStart,i-uidStart);
               }
               break;
            }
            else if(line[i] == ':')
            {
                if(i >= uidStart+5 && line.substr(i-3,4) == "ERR:")
                {
                    uid=line.substr(uidStart,i-uidStart-3);
                }
                break;
            }
       }

       if(uid.empty() && line.size()-uidStart >= 2)
       {
           uid=line.substr(uidStart);
       }

       /*std::string::size_type uidEnd = line.find(" ",uidStart+2);
        if(uidEnd != std::string::npos)
        {
            uid=line.substr(uidStart+2,uidEnd-uidStart-2);
        }
        else
        {
            uid = line.substr(uidStart+2);
        }*/
    }
   //if(uid == "newops")
   //    REQCNT++;

    if(hasIt && uid.empty())
    {
        hasIt=false;
    }

    //collect uid based stats for user requests!
    if(!uid.empty())
    {
        for(size_t i=0; i < uid_vec.size(); i++)
        {
            if(uid_vec[i].name_ ==  uid)
            {
                uid_vec[i].add(false,line);
                return true;
            }
        }

        uid_vec.push_back(LogReqCounter(uid));
        uid_vec.back().add(false,line);
        return true;
    }

    return false;
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
