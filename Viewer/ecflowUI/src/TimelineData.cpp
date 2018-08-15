//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//=============================================================

#include "TimelineData.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QStringList>

#include "TimelineData.hpp"
#include "VNState.hpp"

TimelineItem::TimelineItem(const std::string& path,unsigned char status,unsigned int time) :
    path_(path)
{
    add(status,time);
}

void TimelineItem::add(unsigned char status,unsigned int time)
{
    if(end_.size() > 0)
        end_[end_.size()-1]=time;

    status_.push_back(status);
    start_.push_back(time);
    end_.push_back(0);
}

void TimelineData::loadLogFile(const std::string& logFile,int numOfRows)
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

    //maxNumOfRows_=numOfRows;
    //numOfRows_=0;

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
        throw std::runtime_error("LogLoadData::loadLogFile: Could not open log file " + logFile );

    std::string line;

#if 0
    if(numOfRows < 0)
    {
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
        log_file.setPos(startPos_);
    }
#endif

    //A collector total counts
    //LogReqCounter total("total");

    //A collector for suite related data
    //std::vector<LogReqCounter> suite_vec;

    //A collector for uid related data
    //std::vector<LogReqCounter> uid_vec;

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;

    std::map<std::string,size_t> uidCnt;

    while ( log_file.good() )
    {
        log_file.getline(line); // default delimiter is /n

        // The log file format we are interested is :
        // 0             1         2            3
        // MSG:[HH:MM:SS D.M.YYYY] chd:fullname [path +additional information]
        // MSG:[HH:MM:SS D.M.YYYY] --begin      [args | path(optional) ]    :<user>

        //LOG:[22:45:30 21.4.2018]  complete: path

        /// We are only interested in status changes (i.e LOG:)
        if (line.empty())
            continue;

        if (line[0] != 'L')
            continue;

        std::string::size_type log_pos = line.find("LOG:");
        if (log_pos != 0)
            continue;

        /// LOG:[HH:MM:SS D.M.YYYY] status: fullname [+additional information]
        /// EXTRACT the date
        std::string::size_type first_open_bracket = line.find('[');
        if ( first_open_bracket == std::string::npos)
        {
            assert(false);
            continue;
        }
        //line.erase(0,first_open_bracket+1);

        std::string::size_type first_closed_bracket = line.find(']',first_open_bracket);
        if ( first_closed_bracket ==  std::string::npos)
        {
            //assert(false);
            continue;
        }
        std::string time_stamp = line.substr(first_open_bracket+1,first_closed_bracket-first_open_bracket-1);

        //ecf::Str::split(time_stamp, new_time_stamp);
        //if (new_time_stamp.size() != 2)
        //    continue;

        //line.erase(0,first_closed_bracket+1);

        /// EXTRACT the status
        std::string::size_type first_colon = line.find(':',first_closed_bracket);
        if(first_colon == std::string::npos)
            continue;

        std::string::size_type first_char = line.find_first_not_of(' ',first_closed_bracket+1);
        if(first_char  == std::string::npos)
            continue;

        std::string status=line.substr(first_char,first_colon-first_char);

        //get the status id
        unsigned char statusId;
        if(VNState* vn=VNState::find(status))
            statusId=vn->ucId();
        else
            continue;

        /// EXTRACT the full name
        first_char =  line.find_first_not_of(' ', first_colon+1);
        if(first_char  == std::string::npos)
              continue;

        std::string::size_type next_ws = line.find(' ', first_char+1);
        std::string name;
        if(next_ws  == std::string::npos)
        {
            name=line.substr(first_colon+1);
        }
        else
        {
            name=line.substr(first_colon+1,next_ws-first_colon);
        }

        //Convert status time into int
        unsigned int statusTime=QDateTime::fromString(QString::fromStdString(time_stamp),
                       "hh:mm:ss d.M.yyyy").toMSecsSinceEpoch()/1000;

        int idx=indexOfItem(name);
        if(idx != -1)
        {
            items_[idx].add(statusId,statusTime);
        }
        else
        {
            items_.push_back(TimelineItem(name,statusId,statusTime));
        }
    }


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


      numOfRows_++;

#if 0
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

#endif
}

int TimelineData::indexOfItem(const std::string& p)
{
    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].path() == p)
            return i;
    }
    return -1;
}


#if 0
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
#endif

#if 0
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
#endif
