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

#include <map>
#include <set>
#include <vector>
#include <string>

#include "VParam.hpp"

#include <QPixmap>
#include <QVariant>

class VNode;
class VParamSet;
class VProperty;

class VIcon : public VParam
{
public:
	VIcon(const std::string& name);
	virtual ~VIcon();

	static std::vector<VParam*> filterItems();
	static QVariantList pixmapList(VNode *vnode,VParamSet *filter);
	static VIcon* find(const std::string& name);

	void setProperty(VProperty* prop);

	//Called from VConfigLoader
	static void load(VProperty* group);

protected:
	QPixmap* pixmap(int size);
	virtual bool show(VNode*)=0;

	//QApplication makes some initializations of Qt itself, before which no "complex" Qt objects can be created.
	//Since QPixmap is a complex object it cannot be used in static initialiaztion unless we make it a pointer.
	QPixmap* pix_;
	VProperty* prop_;

	static std::map<std::string,VIcon*> items_;
};

#endif
