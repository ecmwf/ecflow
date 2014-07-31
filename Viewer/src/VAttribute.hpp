//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTE_HPP_
#define VATTRIBUTE_HPP_

#include <set>
#include <vector>
#include <string>

#include "VParam.hpp"

class Node;

class VAttribute : public VParam
{
public:
	VAttribute(QString name,VParam::Type type);

	static std::vector<VParam*> filterItems();
	static bool getData(Node *node,int row,VParam::Type &type,QStringList& data);
	static int totalNum(Node *node);
	static void init();

private:
	static bool getData(Node *node,int row,int& totalRow,VParam::Type type,QStringList& data);
	static int num(Node* node,VParam::Type type);

	static std::vector<VAttribute*> items_;
};

#endif
