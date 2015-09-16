//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef CHANGENOTIFYHANDLER_HPP_
#define CHANGENOTIFYHANDLER_HPP_

#include <map>
#include <string>

class ChangeNotifyDialog;
class ChangeNotifyModel;
class ChangeNotifyWidget;
class VProperty;
class VNode;
class VNodeList;

class ChangeNotify
{
public:
	ChangeNotify(const std::string& id);

	const std::string& id() const {return id_;}
	VNodeList* data() const {return data_;};
	VProperty* prop() const {return prop_;}
	ChangeNotifyModel* model() const {return model_;}
	bool isEnabled() const {return enabled_;}

	static void add(const std::string&,VNode*,bool,bool);
	static void init();
	static void setEnabled(const std::string&,bool);
	static void populate(ChangeNotifyWidget* w);
	static void showDialog(const std::string& id);
	static void clearData(const std::string& id);

	//Called from VConfigLoader
	static void load(VProperty* group);

protected:
	void make();
	void add(VNode*,bool,bool);
	void setEnabled(bool);
	static ChangeNotify* find(const std::string&);
	static ChangeNotifyDialog* dialog();

	std::string id_;
	bool enabled_;
	VNodeList* data_;
	ChangeNotifyModel* model_;
	VProperty* prop_;
	static ChangeNotifyDialog* dialog_;
};

class AbortedNotify : public ChangeNotify
{
public:
	AbortedNotify(const std::string& id) : ChangeNotify(id) {}
};

class RestartedNotify : public ChangeNotify
{
public:
	RestartedNotify(const std::string& id) : ChangeNotify(id) {}
};





#endif
