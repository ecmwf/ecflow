//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VICON_HPP_
#define VICON_HPP_

#include <set>
#include <vector>
#include <string>

#include "VParam.hpp"

#include <QPixmap>

class Node;
class VFilter;

class VIcon : public VParam
{
public:
	typedef bool (*TestProc)(Node*);

	VIcon(QString name,VParam::Type type,TestProc testProc);
	~VIcon();

	static std::vector<VParam*> filterItems();
	static QVariantList pixmapList(Node *node,VFilter *filter);
	static VIcon* find(VParam::Type t);
	static VIcon* find(const std::string& name);
	static void init();

	static bool  testWait(Node *n);
	static bool  testRerun(Node *n);
	static bool  testMessage(Node *n);
	static bool  testComplete(Node *n);
	static bool  testTime(Node *n);
	static bool  testDate(Node *n);
	static bool  testZombie(Node *n);
	static bool  testLate(Node *n);

protected:
	QPixmap* pixmap(int size);

	//QApplication makes some initializations of Qt itself, before which no "complex" Qt objects can be created.
	//Since QPixmap is a complex object it cannot be used in static initialiaztion unless we make it a pointer.
	QPixmap* pix_;

	TestProc testProc_;
	static std::vector<VIcon*> items_;
};

#endif
