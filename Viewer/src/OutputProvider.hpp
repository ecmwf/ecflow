//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef OUTPUTPROVIDER_HPP_
#define OUTPUTPROVIDER_HPP_

#include <QObject>

#include "VDir.hpp"
#include "VInfo.hpp"
#include "InfoProvider.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class OutputClient;

class OutputProvider : public QObject, public InfoProvider
{
Q_OBJECT

public:
	 explicit OutputProvider(InfoPresenter* owner);

	 void visit(VInfoNode*);

	 //Get the contents of the jobout directory
	 VDir_ptr directory();

	 //Get a particular jobout file
	 void file(const std::string& fileName);

	 std::string joboutFileName() const;

private Q_SLOTS:
	void slotOutputClientError(QString);
	void slotOutputClientProgress(QString);
	void slotOutputClientFinished();

private:
	 void fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout);
	 void fetchJoboutViaServer(ServerHandler *server,VNode *n,const std::string&);
	 bool fetchFileViaLogServer(VNode *n,const std::string& fileName);
	 VDir_ptr fetchDirViaLogServer(VNode *n,const std::string& fileName);
	 VDir_ptr fetchLocalDir(const std::string& path);

	 OutputClient *outClient_;
};

#endif
