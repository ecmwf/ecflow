//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_VNODELIST_HPP_
#define VIEWER_SRC_VNODELIST_HPP_

#include "ServerObserver.hpp"
#include "NodeObserver.hpp"

#include <QObject>

#include <assert.h>
#include <vector>

class VNodeList : public QObject, public ServerObserver, public NodeObserver
{
 Q_OBJECT

public:
 	explicit VNodeList(QObject* parent=0);
 	~VNodeList();

 	int size() const {return data_.size();}
 	VNode* nodeAt(int i) const {assert(i>=0 && i < data_.size()); return data_.at(i);}
 	void add(VNode*);
 	void clear();

    //From ServerObserver
 	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {};
 	void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server);
 	void notifyEndServerClear(ServerHandler* server);
 	void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {};
    void notifyEndServerScan(ServerHandler* server) {};

 	//From NodeObserver
    void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
 	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);

Q_SIGNALS:
     void beginAppendRow();
     void endAppendRow();
     void beginReset();
     void endReset();

protected:
     void clear(ServerHandler*);

     std::vector<VNode*> data_;
};

#endif /* VIEWER_SRC_VNODELIST_HPP_ */
