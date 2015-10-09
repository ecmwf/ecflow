//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #70 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>

#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include <boost/token_functions.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>


#include "File.hpp"
#include "File_r.hpp"
#include "Log.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "ecflow_version.h"

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

//#define DEBUG_SERVER_PATH 1
//#define DEBUG_CLIENT_PATH 1

static std::string bjam_workspace_dir()
{
   // We need the *SAME* location so that different process find the same file. Get to the workspace directory
   boost::filesystem::path current_path = boost::filesystem::current_path();
   std::string stem = current_path.stem().string();
   int count = 0;
   while( stem.find("ecflow") == std::string::npos) {
      current_path = current_path.parent_path();
      stem = current_path.stem().string();
      count++;
      if (count == 100) {
         char* workspace = getenv("WK");
         if (workspace == NULL) {
            throw std::runtime_error("File::bjam_workspace_dir() failed to find ecflow in a directory name, up the directory tree and WK undefined");
         }
         std::string the_workspace_dir = workspace;
         return the_workspace_dir;
      }
   }
   std::string the_workspace_dir = current_path.string();  // cos string is returned by reference
   return the_workspace_dir;
}


namespace ecf {

size_t File::MAX_LINES() { return 10000; }
const std::string& File::JOB_EXTN() { static const std::string JOB_EXTN = ".job"; return JOB_EXTN; }
const std::string& File::MAN_EXTN() { static const std::string MAN_EXTN = ".man"; return MAN_EXTN; }
const std::string& File::USR_EXTN() { static const std::string USR_EXTN = ".usr"; return USR_EXTN; }
const std::string& File::ECF_EXTN() { static const std::string ECF_EXTN = ".ecf"; return ECF_EXTN; }


/// Search for the file, in $PATH return the first path that matches or an empty file, if not
std::string File::which(const std::string& file)
{
	std::string env_paths = getenv("PATH");
	if (!env_paths.empty()) {
		std::string path;
		std::vector<std::string> paths;
		Str::split(env_paths,paths,":");
		size_t path_size = paths.size();
		for(size_t i=0; i < path_size; ++i) {
			path.clear();
			path = paths[i];
			path += '/';
			path += file;
			if (fs::exists(path)) {
				return paths[i];
			}
		}
	}
	return std::string();
}

std::string File::getExt(const std::string& s)
{
	size_t i = s.rfind('.',s.length());
	if (i != std::string::npos) {
		return s.substr(i+1);
	}
	return string();
}

void File::replaceExt(std::string& file, const std::string& newExt)
{
	string::size_type i = file.rfind('.',file.length());
	if (i != string::npos)  file.replace(i+1,newExt.length(), newExt);
}

bool File::splitFileIntoLines(const std::string& filename, std::vector<std::string>& lines,bool ignoreEmptyLine)
{
   std::ifstream the_file(filename.c_str(),std::ios_base::in);
  	if ( !the_file )  return false;
	lines.reserve(lines.size() + 100);

	// Note if we use: while( getline( theEcfFile, line)), then we will miss the *last* *empty* line

//	int i = 0;
  	string line;
	while ( std::getline(the_file,line) ) {
//		i++;
//		cout << i << ": " << line << "\n";
		if (ignoreEmptyLine && line.empty()) continue;
 		lines.push_back(line);
	}

//	 METHOD1
//	 Note Another way to split lines of a file was to use a tokenizer.
//	 Much slower ~ 2.5 slower,
//	ifstream ifs(filename.c_str());
//	if (!ifs) return false;
//
//	// Note: ss.str() returns a temporary std::string.
//	// boost::tokenizer stores a reference to the string.
//	// The temporary string is destroyed and tokenizer is left with a dangling reference.
//	// hence take a copy
//	std::stringstream ss;  ss << ifs.rdbuf();  // Read the whole file into a string
//	std::string theFileAsString = ss.str();    // take a copy as ss.str() returns a temporary
//	//                                         // while tokenizer stores a reference
//	if (ignoreEmptyLine) {
//	   char_separator<char> sep("\n",0,boost::drop_empty_tokens);
//	   typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
//	   tokenizer tokens(theFileAsString, sep);
//	   std::copy(tokens.begin(), tokens.end(), back_inserter(lines));
//	}
//	else {
//	   char_separator<char> sep("\n",0,boost::keep_empty_tokens);
//	   typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
//	   tokenizer tokens(theFileAsString, sep);
//	   std::copy(tokens.begin(), tokens.end(), back_inserter(lines));
//	}

	// METHOD 2:
	// Note: The implementation below is 2.5 times slower
	//	std::ifstream ifs(filename.c_str());
	//	if (!ifs) return false;
	//
	//	std::istreambuf_iterator<char> file_iter(ifs);
	//	std::istreambuf_iterator<char> end_of_stream;
	//
	//	typedef boost::tokenizer<boost::char_separator<char>,
	//	std::istreambuf_iterator<char> > tokenizer;
	//
	//	boost::char_separator<char> sep("\n");
	//	tokenizer tokens(file_iter,end_of_stream, sep);
	// std::copy(tokens.begin(), tokens.end(), back_inserter(lines));

	// The current implementation is 2.5 times faster then method 1, and ~4 times faster than method 2
 	return true;
}


std::string File::get_last_n_lines(const std::string& filename,int last_n_lines, std::string& error_msg)
{
   if ( last_n_lines <= 0  ) return string();

   std::ifstream source( filename.c_str(), std::ios_base::in );
   if (!source) {
      error_msg = "File::get_last_n_lines: Could not open file " + filename;
      return string();
   }

   size_t const granularity = 100 * last_n_lines;
   source.seekg( 0, std::ios_base::end );
   size_t size = static_cast<size_t>( source.tellg() );
   std::vector<char> buffer;
   int newlineCount = 0;
   while ( source
           && buffer.size() != size
           && newlineCount < last_n_lines ) {
       buffer.resize( std::min( buffer.size() + granularity, size ) );
       source.seekg( -static_cast<std::streamoff>( buffer.size() ),
                     std::ios_base::end );
#if defined(HPUX) || defined(_AIX)
       source.read( &(buffer.front()), buffer.size() );
#else
       source.read( buffer.data(), buffer.size() );
#endif
       newlineCount = std::count( buffer.begin(), buffer.end(), '\n');
   }

   std::vector<char>::iterator start = buffer.begin();
   while ( newlineCount > last_n_lines ) {
       start = std::find( start, buffer.end(), '\n' ) + 1;
       -- newlineCount;
   }

   //std::vector<char>::iterator end = remove( start, buffer.end(), '\r' ); // for windows
   return std::string( start, buffer.end() );
}


std::string File::get_first_n_lines(const std::string& filename,int n_lines, std::string& error_msg)
{
   if ( n_lines <= 0  ) return string();

   std::ifstream source( filename.c_str(), std::ios_base::in );
   if (!source) {
      error_msg = "File::get_first_n_lines: Could not open file " + filename;
      return string();
   }

   std::string ret; ret.reserve(1024);
   std::string line;

   int count = 0;
   while ( std::getline(source,line) && count < n_lines) {

      ret += line;
      ret += "\n";
      count++;
   }

   return ret;
}

/// Opens the file and returns the contents
bool File::open(const std::string& filePath, std::string& contents)
{
	std::ifstream  infile( filePath.c_str() , std::ios_base::in);
 	if ( ! infile)  return false;

 	std::ostringstream temp;
 	temp << infile.rdbuf();
 	contents = temp.str();
	return true;
}

//std::string File::tmpFile(std::string& contents)
//{
//
//}


bool File::create(const std::string& filename,const std::vector<std::string>& lines, std::string& errorMsg)
{
	// For very large file. This is about 1 second quicker. Than using streams
	// See Test: TestFile.cpp:test_file_create_perf
	FILE * theFile = fopen (filename.c_str(),"w");
	if (theFile==NULL) {
		std::stringstream ss;
		ss << "Could not create file '" << filename << "'\n";
		errorMsg += ss.str();
		return false;
	}
 	size_t size = lines.size();
	for (size_t i = 0; i < size; ++i) {
		if (i != 0) {
		   if (fputs("\n",theFile) == EOF) {
		      std::stringstream ss;
		      ss << "Could not write to file '" << filename << "'\n";
		      errorMsg += ss.str();
		      fclose (theFile);
		      return false;
		   }
		}
		if (fputs(lines[i].c_str(),theFile) == EOF) {
         std::stringstream ss;
         ss << "Could not write to file '" << filename << "'\n";
         errorMsg += ss.str();
         fclose (theFile);
         return false;
		}
 	}
 	fclose (theFile);

//	std::ofstream theFile( filename.c_str() );
//	if ( !theFile ) {
//		/// Could be: [ no permissions | file system full | locked by another process ]
// 		std::stringstream ss;
//		ss << "Could not create file '" << filename << "'\n";
//		errorMsg += ss.str();
//		return false;
//	}
//	size_t size = lines.size();
//	for (size_t i = 0; i < size; ++i) {
//		if (i != 0) theFile << "\n";
//		theFile << lines[i];
//	}
//	if (!theFile.good()) {
//      std::stringstream ss;
//      ss << "Could not write to file '" << filename << "'\n";
//      errorMsg += ss.str();
//      theFile.close();
//      return false;
//	}
//	theFile.close();

	return true;
}

bool File::create(const std::string& filename, const std::string& contents, std::string& errorMsg)
{
	std::ofstream theFile( filename.c_str() );
	if ( !theFile ) {
 		std::stringstream ss;
		ss << "Could not create file '" << filename << "'\n";
		errorMsg += ss.str();
		return false;
	}

	theFile << contents;
	if (!theFile.good()) {
	   std::stringstream ss;
	   ss << "Could not write to file '" << filename << "'\n";
	   errorMsg += ss.str();
	   theFile.close();
	   return false;
	}
	theFile.close();

	// This is actually 12% slower for large file:
//	FILE * theFile = fopen (filename.c_str(),"w");
//	if (theFile==NULL) {
//		std::stringstream ss;
//		ss << "Could not create file '" << filename << "'\n";
//		errorMsg += ss.str();
//		return false;
//	}
//   if (fputs(contents.c_str(),theFile) == EOF) {
//      std::stringstream ss;
//      ss << "Could not write to file '" << filename << "'\n";
//      errorMsg += ss.str();
//      fclose (theFile);
//      return false;
//   }
//  	fclose (theFile);
	return true;
}

bool File::find(
                 const boost::filesystem::path& dir_path,    // from this directory downwards,
                 const std::string&             file_name,   // search for this name,
                 boost::filesystem::path&       path_found   // placing path here if found
			   )
{
//	std::cout << "Searching '" << dir_path << "' for  " << file_name  << "\n";
	if ( !fs::exists( dir_path ) )
		return false;

	fs::directory_iterator end_itr; // default construction yields past-the-end
	for (fs::directory_iterator itr( dir_path ); itr != end_itr; ++itr) {

		if ( fs::is_directory( itr->status() ) ) {

			if ( File::find( itr->path(), file_name, path_found ) )
				return true;
		}
	   else if ( itr->path().filename() == file_name ) // see below
		{
			path_found = itr->path();
			return true;
		}
	}
	return false;
}

void File::findAll(
                  const boost::filesystem::path& dir_path,    // from this directory downwards
                  const std::string&             file_name,   // search for this name,
                  std::vector<boost::filesystem::path>& paths_found   // placing path here if found
			    )
{
	if ( !fs::exists( dir_path ) ) return;

	fs::directory_iterator end_itr; // default construction yields past-the-end
	for (fs::directory_iterator itr( dir_path ); itr != end_itr; ++itr) {

		if ( fs::is_directory( itr->status() ) ) {

			findAll( itr->path(), file_name, paths_found ) ;
		}
      else if ( itr->path().filename() == file_name ) // see below
		{
			paths_found.push_back( itr->path() );
 		}
	}
}

std::string File::findPath(
                  const boost::filesystem::path& dir_path,    // from this directory downwards
                  const std::string&             file_name,   // search for this name,
                  const std::string&             leafDir      // path must contain this string
			    )
{
	std::vector< fs::path > paths;
	File::findAll( dir_path, file_name, paths );
	if ( !paths.empty() ) {

		// find the path that has leafDir in it.
 		BOOST_FOREACH(fs::path path, paths) {
			std::string thePath = path.string();
			if (thePath.rfind(leafDir) != std::string::npos) return thePath;
		}
 	}
	return std::string();
}


std::string File::findPath(
         const boost::filesystem::path&  dir_path,    // from this directory downwards
         const std::string&              file_name,   // search for this name,
         const std::vector<std::string>& tokens       // path must contain all these tokens
)
{
   std::vector< fs::path > paths;
   File::findAll( dir_path, file_name, paths );
   if ( !paths.empty() ) {

      // find the path that has all the tokens specified by the vector tokens
      BOOST_FOREACH(fs::path path, paths) {
         std::string thePath = path.string();
         size_t matches = 0;
         BOOST_FOREACH(const std::string& required_path_tokens, tokens) {
            if (thePath.rfind(required_path_tokens) != std::string::npos) matches++;
         }
         if (matches == tokens.size()) return thePath;
       }
   }
   return std::string();
}

//#define INTEL_DEBUG_ME 1
bool File::createMissingDirectories(const std::string& pathToFileOrDir)
{
#ifdef INTEL_DEBUG_ME
   std::cout << "File::createMissingDirectories  " << pathToFileOrDir << std::endl;
#endif
	if (pathToFileOrDir.empty()) return false;

	// Avoid making unnecessary system calls, by checking to see if directory exists first
   fs::path fs_path(pathToFileOrDir);
   if (fs_path.extension().empty()) {

#ifdef INTEL_DEBUG_ME
      std::cout << "   pure directory no extension" << std::endl;
#endif
      // pure directory
      if (fs::exists(pathToFileOrDir)) {

#ifdef INTEL_DEBUG_ME
         std::cout << "   " << pathToFileOrDir << " already exists " << std::endl;
#endif

         return true;
      }
   }
   else {

#ifdef INTEL_DEBUG_ME
      std::cout << "   pure directory *has* extension" << std::endl;
#endif
      // could be /tmp/fred/sms.job, see if /tmp/fred exists
      if (fs::exists(fs_path.parent_path())) {

#ifdef INTEL_DEBUG_ME
         std::cout << "   " << pathToFileOrDir << " already exists " << std::endl;
#endif
         return true;
      }
   }

 	std::vector<std::string> thePath;
	NodePath::split(pathToFileOrDir, thePath);
	try {
		if ( !thePath.empty() ) {

#ifdef INTEL_DEBUG_ME
         std::cout << "   last file " << thePath.back() << std::endl;
#endif

			// pathToFileOrDir is of form: /tmp/fred/sms.job
			//   we should only create directories for /tmp/fred
			if ( thePath.back().find( "." ) != std::string::npos ) {
				// assume the last token represents a file, hence dont create a directory
#ifdef INTEL_DEBUG_ME
	         std::cout << "   last file " << thePath.back() << " has a *dot* ignoring "<< std::endl;
#endif
				thePath.pop_back();
			}

			std::string pathToCreate;

			// if original path had leading slash then add it here, to preserve path
			if (pathToFileOrDir[0] == '/') pathToCreate += Str::PATH_SEPERATOR();

			for (size_t i = 0; i < thePath.size(); i++) {
				pathToCreate += thePath[i];
				if ( !fs::exists( pathToCreate ) ) {
#ifdef INTEL_DEBUG_ME
	            std::cout << "   " << pathToCreate << " does not exist, creating directory " << std::endl;
#endif
					fs::create_directory( pathToCreate );
				}
				pathToCreate += Str::PATH_SEPERATOR();
			}
		}
		else {
#ifdef INTEL_DEBUG_ME
         std::cout << "   NO path component in " << pathToFileOrDir <<  std::endl;
#endif

			if ( pathToFileOrDir.find( "." ) != std::string::npos ) {
				// assume represents a file, hence don't create a directory fred.job1
#ifdef INTEL_DEBUG_ME
	         std::cout << "   assuming " << pathToFileOrDir << " is a file, found a dot, returning without creating dir " <<  std::endl;
#endif
				return true;
			}

			// assume is a dir
#ifdef INTEL_DEBUG_ME
         std::cout << "   about to create dir " <<  pathToFileOrDir << std::endl;
#endif

			fs::create_directory( pathToFileOrDir );
		}
	}
	catch ( const std::exception & ex ) {
#ifdef INTEL_DEBUG_ME
	   std::cout << "   Exception " << ex.what() << " could not create dir " << pathToFileOrDir << std::endl;
#endif
		return false;
	}
	return true;
}


/// Create directories the boost way, with additional check to see if directories exist first
bool File::createDirectories(const std::string& pathToDir)
{
	if (pathToDir.empty()) return false;
	if (fs::exists(pathToDir)) return true;

	try {
		return fs::create_directories(pathToDir);
	}
	catch (std::exception&) {}
	return false;
}

/// Returns the difference between 2 files.
std::string File::diff(const std::string& file,
                       const std::string& file2,
                       const std::vector<std::string>& ignoreVec,
                       std::string& errorMsg,
                       bool ignoreBlanksLine)
{
	if (!fs::exists(file)) {
		errorMsg += "First argument File " + file + " does not exist";
		return std::string();
	}
	if (!fs::exists(file2)) {
		errorMsg += "Second argument File " + file2 + " does not exist";
		return std::string();
	}

	std::vector<std::string> fileLines;
	std::vector<std::string> file2Lines;

	if (!splitFileIntoLines(file, fileLines, ignoreBlanksLine)) {
		errorMsg += "First argument File " + file + " could not be opened";
		return std::string();
	}
	if (!splitFileIntoLines(file2, file2Lines, ignoreBlanksLine)) {
		errorMsg += "Second argument File " + file2 + " could not be opened";
		return std::string();
	}

	if ( fileLines  != file2Lines) {
		std::stringstream ss;
 		if (fileLines.size() != file2Lines.size())  ss << "Expected size " << file2Lines.size() << " but found " << fileLines.size() <<"\n";

		for(size_t i=0; i < fileLines.size() || i < file2Lines.size(); ++i) {

			if (i < fileLines.size() && i < file2Lines.size()) {

				if (fileLines[i] !=  file2Lines[i]) {
					bool doIgnore = false;
					BOOST_FOREACH(const std::string& ignore, ignoreVec) {
						if ( fileLines[i].find(ignore) != std::string::npos || file2Lines[i].find(ignore) != std::string::npos  ) {
							doIgnore = true; break;
						}
					}
					if (doIgnore) continue;
					ss << "Mismatch at " << i << "(" << fileLines[i] << ")   ---->   (" << file2Lines[i] << ")\n";
				}
//				else {
//					ss << "            " << i << " (" << fileLines[i] << ")   ---->   (" << file2Lines[i] << ")\n";
//				}
			}
			else {
				ss << "Mismatch at " << i ;
				if (i < fileLines.size() ) ss << " (" << fileLines[i] << ")   ";
				else                       ss << " ( ---- )   ";

				if (i < file2Lines.size()) ss << "(" << file2Lines[i] << ")\n";
				else                       ss << "( --- )\n";
			}
		}
		return ss.str();
	}
	return std::string();
}

std::string File::backwardSearch( const std::string& rootPath, const std::string& nodePath, const std::string& fileExtn )
{
	// Do a backward search of rootPath + nodePath
	// If task path if of the form /suite/family/family2/task, then we keep
	// on consuming the first path token this should leave:
	//   	<root-path>/suite/family/family2/task.ecf
	//   	<root-path>/family/family2/task.ecf
	//  	<root-path>/family2/task.ecf
	//   	<root-path>/task.ecf
	// See page 21 of SMS user guide

	vector<std::string> nodePathTokens;
	NodePath::split(nodePath, nodePathTokens);
	LOG_ASSERT(!nodePathTokens.empty(),"");

	std::string leafName; // i.e. task in the example above
	if ( !nodePathTokens.empty() )  leafName = nodePathTokens[ nodePathTokens.size() -1 ] ;

#ifdef DEBUG_TASK_LOCATION
	cout << "backwardSearch Node " << nodePath << " using root path " << rootPath << " nodePathTokens.size() = " << nodePathTokens.size() << "\n";
#endif
	while ( nodePathTokens.size() > 0 ) {

		// Reconstitute the path
	 	std::string path = NodePath::createPath( nodePathTokens ) ;
		path += fileExtn; // .ecf, .man , etc

		std::string combinedPath = rootPath;
		combinedPath += path;

		try {
			if ( fs::exists( combinedPath )) {
#ifdef DEBUG_TASK_LOCATION
				cout << "backwardSearch Node " << nodePath << " the path " << combinedPath << " exists\n";
#endif
				return combinedPath;
 			}
#ifdef DEBUG_TASK_LOCATION
			else  cout << " backwardSearch Node " << nodePath << " the path " << combinedPath << " DOEST NOT EXIST\n";
#endif
		}
		catch (fs::filesystem_error& e) {
#ifdef DEBUG_TASK_LOCATION
			std::cout << "Task::backwardSearch Caught exception for fs::exists('" << combinedPath <<"')\n" << e.what() << "\n";
#endif
		}

		nodePathTokens.erase(nodePathTokens.begin()); // consume first path token
	}

	// Look for file in the root path
 	std::string ecf_file = leafName + fileExtn;
	fs::path ecf_filePath = fs::path( fs::path(rootPath) / ecf_file );
	if (fs::exists(ecf_filePath)) {
#ifdef DEBUG_TASK_LOCATION
		std::cout << "backwardSearch Node " << leafName << " Found " << ecf_file << " in root path '" << rootPath << "'\n";
#endif
		std::string result = ecf_filePath.string(); // is returned by reference hence must take a copy
		return result ;
	}

	// failed to find file via backward search
	return string();
}

// Remove a directory recursively ****
bool File::removeDir( const boost::filesystem::path& p)
{
	try {
		fs::directory_iterator end;
		for(fs::directory_iterator it(p); it != end; ++it) {
			if (fs::is_directory(*it)) {
				if (!removeDir(*it)) {
					return false;
				}
			}
			else {
				fs::remove(*it);
			}
		}
	}
	catch (fs::filesystem_error& e) {
		return false;
	}

	// Finally remove dir itself
	fs::remove(p);

	return true;
}


static std::string find_bjam_ecf_server_path()
{
#ifdef DEBUG_SERVER_PATH
   cout << " File::find_ecf_server_path() using bjam\n";
#endif

   // bjam uses in source tree, for build, which is in the workspace dir
   std::string bin_dir = bjam_workspace_dir() + "/Server/bin/";

#ifdef DEBUG_SERVER_PATH
   cout << "  Searching under: " << bin_dir << "\n";
#endif

   // We need to take into account that on linux, we may have the GNU and CLANG executables
   // Hence we need to distinguish between them.
   std::vector<std::string> required_path_tokens;

   // We have 3 variants debug,release,profile
#ifdef DEBUG

   required_path_tokens.push_back(std::string("debug"));
#if defined(__clang__)
   required_path_tokens.push_back(std::string("clang"));
#endif

   return File::findPath( bin_dir, Ecf::SERVER_NAME(), required_path_tokens );

#else

   required_path_tokens.push_back(std::string("release"));
#if defined(__clang__)
   required_path_tokens.push_back(std::string("clang"));
#endif

   std::string path = File::findPath( bin_dir, Ecf::SERVER_NAME(), required_path_tokens );
   if (path.empty()) {

      required_path_tokens.clear();
      required_path_tokens.push_back(std::string("profile"));
#if defined(__clang__)
      required_path_tokens.push_back(std::string("clang"));
#endif

      path = File::findPath( bin_dir, Ecf::SERVER_NAME(), required_path_tokens );
   }
   return path;
#endif
}


std::string File::find_ecf_server_path()
{
   if (File::cmake_build()) {

      std::string path = CMAKE_ECFLOW_BUILD_DIR;
      path += "/bin/";
      path += Ecf::SERVER_NAME();

#ifdef DEBUG_SERVER_PATH
      cout << " File::find_ecf_server_path() path = " << path << "\n";
#endif
      return path;
   }

   return find_bjam_ecf_server_path();
}


static std::string find_bjam_ecf_client_path()
{
#ifdef DEBUG_CLIENT_PATH
   cout << " find_bjam_ecf_client_path \n";
#endif

   // Bjam uses, in source build
   std::string binDir = bjam_workspace_dir() + "/Client/bin/";

   // We need to take into account that on linux, we may have the GNU and CLANG executables
   // Hence we need to distinguish between them.
   std::vector<std::string> required_path_tokens;

   // We have 3 variants debug,release,profile
#ifdef DEBUG

   required_path_tokens.push_back(std::string("debug"));
#if defined(__clang__)
   required_path_tokens.push_back(std::string("clang"));
#endif

   return File::findPath( binDir, Ecf::CLIENT_NAME(), required_path_tokens );

#else

   required_path_tokens.push_back(std::string("release"));
#if defined(__clang__)
   required_path_tokens.push_back(std::string("clang"));
#endif

   std::string path = File::findPath( binDir, Ecf::CLIENT_NAME(), required_path_tokens );
   if (path.empty()) {

      required_path_tokens.clear();
      required_path_tokens.push_back(std::string("profile"));
#if defined(__clang__)
      required_path_tokens.push_back(std::string("clang"));
#endif

      path = File::findPath( binDir, Ecf::CLIENT_NAME(), required_path_tokens );
   }
   return path;
#endif
}

std::string File::find_ecf_client_path()
{
   if (File::cmake_build()) {

      std::string path = CMAKE_ECFLOW_BUILD_DIR;
      path += "/bin/";
      path += Ecf::CLIENT_NAME();

#ifdef DEBUG_CLIENT_PATH
      cout << " File::find_ecf_client_path() returning path " << path << "\n";
#endif
      return path;
   }

#ifdef DEBUG_CLIENT_PATH
   cout << " File::find_ecf_server_path() using bjam\n";
#endif
   return find_bjam_ecf_client_path();
}


std::string File::test_data(const std::string& rel_path, const std::string& dir)
{
   std::string test_file;
   char* work_space = getenv("WK"); // for ecbuild
   if (work_space != NULL ) {
      test_file = std::string(work_space);
      if (!rel_path.empty() && rel_path[0] != '/' ) test_file += "/";
      test_file += rel_path;
   }
   else {
      std::string work_space = root_source_dir();
      if (!work_space.empty()) {
         test_file = work_space;
         if (!rel_path.empty() && rel_path[0] != '/' ) test_file += "/";
         test_file += rel_path;
      }
      else {
         fs::path current_path = fs::current_path();
         if (current_path.stem() == dir ) {

            // remove first path, expecting "dir/path/path1" remove dir
            std::string::size_type pos = rel_path.find("/",1); // skip over any leading /
            if (pos != std::string::npos) {
               test_file += rel_path.substr(pos+1); // skip over '/' to be left with path/path1, making it relative
            }
            else {
               test_file += rel_path;
            }
         }
         else {
            test_file += rel_path;
         }
      }
   }
   return test_file;
}


bool File::cmake_build()
{
   fs::path current_path = fs::current_path();
   std::string the_current_path = current_path.string();
   std::string cmakecache_txt = the_current_path + "/CTestTestfile.cmake";
   if (fs::exists(cmakecache_txt)) {
      return true;
   }
   return false;
}

std::string File::root_source_dir()
{
   // bjam
   fs::path current_path = fs::current_path();
   std::string the_current_path = current_path.string();
   std::string version_cmake = the_current_path + "/VERSION.cmake";
   if (fs::exists(version_cmake)) {
      return the_current_path;
   }

   std::string stem = current_path.stem().string();
   int count = 0;
   while( stem.find("ecflow") == std::string::npos) {
      current_path = current_path.parent_path();

      // bjam
      std::string version_cmake = std::string(current_path.string()) + "/VERSION.cmake";
      if (fs::exists(version_cmake)) {
         std::string the_root_source_dir = current_path.string();  // cos current_path.string() is returned by reference
         return the_root_source_dir;
      }

      stem = current_path.stem().string();
      count++;
      if (count == 10000) break;
   }

   // cmake
   return CMAKE_ECFLOW_SOURCE_DIR;
}

std::string File::root_build_dir()
{
   fs::path current_path = fs::current_path();
   std::string the_current_path = current_path.string();

   // bjam
   std::string version_cmake = the_current_path + "/VERSION.cmake";
   if (fs::exists(version_cmake))  return the_current_path;

   // cmake
   std::string cmake_cache = the_current_path + "/CMakeCache.txt";
   if (fs::exists(cmake_cache))  return the_current_path;


   // bjam or cmake
   std::string stem = current_path.stem().string();
   int count = 0;
   while( stem.find("ecflow") == std::string::npos) {
      current_path = current_path.parent_path();

      the_current_path = current_path.string(); // cos current_path.string() is returned by reference

      // bjam
      std::string version_cmake = the_current_path + "/VERSION.cmake";
      if (fs::exists(version_cmake))  return the_current_path;

      // cmake
      std::string cmake_cache = the_current_path + "/CMakeCache.txt";
      if (fs::exists(cmake_cache))  return the_current_path;

      stem = current_path.stem().string();
      count++;
      if (count == 1000) throw std::runtime_error("File::root_build_dir() failed to find ecflow in a directory name, up the directory tree");
   }

   throw std::runtime_error("File::root_build_dir() failed to find root build directory");
   return std::string();
}


}
