//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEMODELDATA_H
#define NODEMODELDATA_H

#include <vector>

class Node;

class NodeFilter;
class ServerHandler;
class VFilter;

class NodeModelData
{
friend class NodeModelDataHandler;

public:
    NodeModelData(ServerHandler *server,NodeFilter*);
    ~NodeModelData();

    int nodeNum() const;
    void filter(VFilter* stateFilter);

    /*bool isFiltered(Node*) const;

    void resetFilter(VFilter* stateFilter);
    int nodeNum() const;*/

protected:
    //bool filterState(node_ptr node,VFilter* stateFilter);

    /*ServerHandler *server_;
	QSet<Node*> nodeFilter_;
	Node* rootNode_;
	mutable int nodeNum_;*/

	ServerHandler *server_;
	NodeFilter* filter_;
	mutable int nodeNum_;
};


class NodeModelDataHandler
{
public:
	NodeModelDataHandler() {}

	void add(ServerHandler*,NodeFilter*);
	void clear();

	ServerHandler* server(int) const;
	ServerHandler* server(void*) const;
	int indexOf(ServerHandler* s) const;
	int count() const {return static_cast<int>(data_.size());}

	int numOfNodes(int) const;
	int numOfFiltered(int index) const;
	bool isFiltered(Node *node) const;
	void filter(VFilter* stateFilter);
	Node* getNodeFromFilter(int totalRow);

	/*ServerHandler* server(int) const;
	ServerHandler* server(void*) const;
	void add(ServerHandler *);
	int indexOf(ServerHandler* s) const;
	int count() const {return data_.count();}
	void clear();
	void clearFilter();
	void nodeFilter(int i,QSet<Node*>);
	bool isFiltered(Node *node) const;
	void resetFilter(VFilter* stateFilter);*/

protected:
	NodeModelData* data(int) const;

	std::vector<NodeModelData*> data_;
};

#endif
