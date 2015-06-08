//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Encompasses functionality for plotting server load
//============================================================================
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h> // for chmod
#include <boost/bind.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Gnuplot.hpp"
#include "File_r.hpp"
#include "Str.hpp"
#include "NodePath.hpp"
#include "Host.hpp"

using namespace std;
namespace fs = boost::filesystem;

namespace ecf {

Gnuplot::Gnuplot( const std::string& log_file,
         const std::string& host,
         const std::string& port,
         size_t no_of_suites_to_plot
         )
: log_file_(log_file), host_(host),port_(port), no_of_suites_to_plot_(no_of_suites_to_plot)
{
   if (!fs::exists(log_file)) {
      std::stringstream ss;
      ss << "Gnuplot::Gnuplot: The log file " << log_file << " does not exist\n";
      throw std::runtime_error( ss.str() );
   }
}

void Gnuplot::show_server_load() const
{
   std::string gnuplot_dat_file    = host_.prefix_host_and_port(port_,"gnuplot.dat");
   std::string gnuplot_script_file = host_.prefix_host_and_port(port_,"gnuplot.script");


   // The vector index is the order in which suites are found, this will be used to place the suite in the correct column
   std::vector<SuiteLoad> suite_vec;
   std::string gnuplot_file = create_gnuplot_file(suite_vec,gnuplot_dat_file);
   std::string gnuplot_script = create_gnuplot_script(gnuplot_file,suite_vec,no_of_suites_to_plot_,gnuplot_script_file);


   // make the gnuplot_script file executable
   if ( chmod( gnuplot_script.c_str(), 0755 ) != 0 ) {
      std::stringstream ss;
      ss << "Gnuplot::show_server_load: Could not make gnu script file " << gnuplot_script << "  executable by using chmod";
      throw std::runtime_error(ss.str());
   }

   std::string execute_gnuplot = "gnuplot " + gnuplot_script;
   ::system(execute_gnuplot.c_str());
}


std::string Gnuplot::create_gnuplot_file(
         std::vector<SuiteLoad>& suite_vec,
         const std::string& temp_file ) const
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

   /// The log file can be massive > 50Mb
   File_r log_file(log_file_);
   if ( !log_file.ok() ) throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Could not open log file " + log_file_ );


   /// Create a new file that can be used with gnuplot. This has to be column based
   std::ofstream gnuplot_file( temp_file.c_str() );
   if ( !gnuplot_file ) {
       throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Could not open output file: " + temp_file);
   }

   gnuplot_file << "#time    date     total-request child user suite_0  suite_1 suite_2  suite_3  suite_n\n";

   std::vector< std::string > new_time_stamp;
   std::vector< std::string > old_time_stamp;
   size_t child_requests_per_second = 0;
   size_t user_request_per_second = 0;
   unsigned int plot_data_line_number = 0;
   string line;
   while ( log_file.good() ) {
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
      if (line.find(Str::CHILD_CMD()) != std::string::npos) child_cmd = true;
      else if (line.find(Str::USER_CMD()) != std::string::npos)  user_cmd = true;
      if (!child_cmd && !user_cmd) continue;

      new_time_stamp.clear();
      {
         /// MSG:[HH:MM:SS D.M.YYYY] chd:fullname [+additional information] ---> HH:MM:SS D.M.YYYY
         /// EXTRACT the date
         string::size_type first_open_bracket = line.find('[');
         if ( first_open_bracket == std::string::npos) { std::cout << line << "\n"; assert(false); continue;}
         line.erase(0,first_open_bracket+1);

         string::size_type first_closed_bracket = line.find(']');
         if ( first_closed_bracket ==  std::string::npos) { std::cout << line << "\n"; assert(false); continue;}
         std::string time_stamp = line.substr(0, first_closed_bracket);

         Str::split(time_stamp, new_time_stamp);
         if (new_time_stamp.size() != 2)  continue;

         line.erase(0,first_closed_bracket+1);
      }

      // Should be just left with " chd:<child command> " or " --<user command>, since we have remove time stamp
//#ifdef DEBUG
//      std::cout << line << "\n";
//#endif

      if (old_time_stamp.empty()) {
         if (child_cmd) child_requests_per_second++;
         else           user_request_per_second++;

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if ( suite_path_found ) assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
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
         plot_data_line_number++;
         gnuplot_file << old_time_stamp[0] << " "
                      << old_time_stamp[1] << " "
                      << (child_requests_per_second + user_request_per_second) << " "
                      << child_requests_per_second << " "
                      << user_request_per_second << " ";
         for(size_t i = 0; i < suite_vec.size(); i++) { gnuplot_file << suite_vec[i].request_per_second_ << " ";}
         gnuplot_file << "\n";


         // clear request per second
         child_requests_per_second = 0;
         user_request_per_second = 0;
         for(size_t i= 0; i < suite_vec.size();i++) { suite_vec[i].request_per_second_ = 0; }

         // start of *new* time
         if (child_cmd) child_requests_per_second++;
         else           user_request_per_second++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         if ( suite_path_found ) assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }

      old_time_stamp = new_time_stamp;
   }

   if (plot_data_line_number < 3) {
      throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Log file empty or not enough data for plot\n");
   }
   return temp_file;
}

std::string Gnuplot::create_gnuplot_script(
         const std::string& path_to_file,
         const std::vector<SuiteLoad>& suite_vec,
         size_t no_of_suites_to_plot,
         const std::string& script) const
{
   /// Create the gnuplot script file for rendering the graph
   std::ofstream gnuplot_script( script.c_str() );
   if ( !gnuplot_script ) {
       throw std::runtime_error( "Gnuplot::create_gnuplot_script: Could not open output file: " + script);
   }

   gnuplot_script << "set term png\n";
   gnuplot_script << "set output \"" << host_.name() << "." << port_ << ".png\"\n";


   gnuplot_script << "set autoscale                          # scale axes automatically\n";
   gnuplot_script << "set xtic auto rotate                   # set xtics automatically\n";
   gnuplot_script << "set ytic auto                          # set ytics automatically\n";
//   gnuplot_script << "set origin 0,0.08                      # offset y, so that rotated xtics don't truncate, However cause title to disappear\n";
   gnuplot_script << "set title \"Server request per second\"\n";
   gnuplot_script << "set x2label \"time/min\" textcolor lt 3\n";
   gnuplot_script << "set ylabel \"requests\"\n";
   gnuplot_script << "set xdata time\n";
   gnuplot_script << "set grid                             # show grid\n";
   gnuplot_script << "set timefmt \"%H:%M:%S %d.%m.%Y\"\n";

   //# LINE COLORS, STYLES
   //# type 'test' to see the colors and point types available.
   //# Differs from x11 to postscript
   //# lt chooses a particular line type: -1=black 1=red 2=grn 3=blue 4=purple 5=aqua 6=brn 7=orange 8=light-brn
   //# lt must be specified before pt for colored points
   //# for postscipt -1=normal, 1=grey, 2=dashed, 3=hashed, 4=dot, 5=dot-dash
   //# lw chooses a line width 1=normal, can use 0.8, 0.3, 1.5, 3, etc.
   //# ls chooses a line style

   ///    1         2         3             4              5            6        7       8         9       n
   ///  HH:MM:SS D.M.YYYY total_request child_request  users_requests suite_0 suite_1  suite_2  suite_3  suite_n

   // determine which suite columns to plot based on server load
   std::vector<SuiteLoad> suite_vec_copy = suite_vec;

//   cout << "sort vector according to load\n";
   std::sort(suite_vec_copy.begin(),suite_vec_copy.end(),
            boost::bind(std::greater<int>(),
               boost::bind(&SuiteLoad::total_request_per_second_, _1),
               boost::bind(&SuiteLoad::total_request_per_second_, _2)));
//   for(size_t i = 0; i < suite_vec_copy.size(); i++) {
//      cout << " " << suite_vec_copy[i].first << " " << suite_vec_copy[i].second << "\n";
//   }

//   cout << "get top loaded suites\n";
   std::vector<std::string> suites;
   for(size_t i = 0; i < suite_vec_copy.size() && i < no_of_suites_to_plot; i++) {
      suites.push_back(suite_vec_copy[i].suite_name_);
   }
//   std::copy(suites.begin(), suites.end(), std::ostream_iterator <std::string> (std::cout, "\n"));


//   cout << "now determine which columns the top suites belong to, **THIS** time <int> indicates column\n";
   std::vector< std::pair<std::string,int> > ordered_suites;
   for(size_t column = 0; column < suite_vec.size(); column++) {
      for(size_t j = 0; j < suites.size(); j++) {
         if (suites[j] == suite_vec[column].suite_name_) {
            ordered_suites.push_back ( std::make_pair(suites[j], column));
         }
      }
   }
//   for(size_t i = 0; i < ordered_suites.size(); i++) {
//      cout << " " << ordered_suites[i].first << " " << ordered_suites[i].second << "\n";
//   }


   gnuplot_script << "plot \""
                  << path_to_file << "\" using 1:4 title \"child\" with lines, \""
                  << path_to_file << "\" using 1:5 title \"user\" with lines, \""
                  << path_to_file << "\" using 1:3 smooth bezier title \"total-load\" with lines lt 3";
   if (!ordered_suites.empty()) gnuplot_script << ",";
   else                         gnuplot_script << "\n";
   for(size_t i = 0; i < ordered_suites.size(); i++) {
      gnuplot_script << "\""
                     << path_to_file
                     << "\" using 1:" << (6 + ordered_suites[i].second)
                     << " smooth bezier title \"" << ordered_suites[i].first << "\" with lines";
      if (i == ordered_suites.size() -1) gnuplot_script << "\n";
      else                               gnuplot_script << ",";
   }

//   gnuplot_script << "pause -1 \"Hit any key to continue\"\n\n";

   return script;
}


bool Gnuplot::extract_suite_path(
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
   if ( forward_slash != std::string::npos) {

      std::string path;
      if (child_cmd) {

         // For labels ignore paths in the label part
         // MSG:[14:55:04 17.10.2013] chd:label progress 'core/nodeattr/nodeAParser' /suite/build/cray/cray_gnu/build_release/test
         if (line.find("chd:label") != std::string::npos) {
            size_t last_tick = line.rfind("'");
            if ( last_tick != std::string::npos ) {
               size_t the_forward_slash = line.find('/',last_tick);
               if (the_forward_slash != std::string::npos) {
                  forward_slash = the_forward_slash;
               }
            }
         }
         path = line.substr(forward_slash);
      }
      else {
         // Ignore the --news command, they dont have a path, hence i.e to ignore line like:
         //  MSG:[09:36:05 22.10.2013] --news=1 36506 6  :ma0 [server handle(36508,7) server(36508,7)
         //                     : *Large* scale changes (new handle or suites added/removed) :NEWS]
         //   the /removed was being interpreted as a suite
         if (line.find("--news") != std::string::npos) return false;
      }

      // find the space after the path
      size_t space_pos = line.find(" ",forward_slash);
      if (space_pos != std::string::npos &&  space_pos > forward_slash) {
         path = line.substr(forward_slash,space_pos-forward_slash);
      }

      if (!path.empty()) {

         std::vector<std::string> theNodeNames; theNodeNames.reserve(4);
         NodePath::split(path,theNodeNames);
         if (!theNodeNames.empty()) {
            for(size_t n = 0; n < suite_vec.size(); n++) {
               if (suite_vec[n].suite_name_ == theNodeNames[0] ) {
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


}
