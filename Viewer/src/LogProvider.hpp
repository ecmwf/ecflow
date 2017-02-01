//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#ifndef VIEWER_SRC_LOGPROVIDER_HPP_
#define VIEWER_SRC_LOGPROVIDER_HPP_

#include <QObject>
#include <QStringList>

#include "VDir.hpp"
#include "VInfo.hpp"
#include "InfoProvider.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class FileWatcher;

class LogProvider : public QObject, public InfoProvider
{
  Q_OBJECT

public:
	 LogProvider(InfoPresenter* owner,QObject* parent=0);

	 void visit(VInfoServer*);
	 void clear();
     void setAutoUpdate(bool);

public Q_SLOTS:
	void slotLinesAppend(QStringList);

private:
	void fetchFile();
	void fetchFile(ServerHandler *server,const std::string& fileName);
	void watchFile(const std::string&,size_t);
	void stopWatchFile();
	std::string readLastLines(const std::string& filename,int last_n_lines,size_t& size,std::string& error_msg);

    FileWatcher* fileWatcher_;

};

#endif
