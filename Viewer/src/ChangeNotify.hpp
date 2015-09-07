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
class VProperty;
class VNode;
class VNodeList;
/*
class ChangeNotifyConf  {
public:
	//ChangeNotifyConf(QColor headerBg,QColor headerFg,QString title,QString header) :
	//	headerBg_(headerBg), headerFg_(headerFg), title_(title), header_(header) {}

	ChangeNotifyConf()  {};

	//Called from VConfigLoader
		static void load(VProperty* group);


	QColor headerBg_;
	QColor headerFg_;
	QString title_;
	QString headertext_;
};
*/
class ChangeNotify
{
public:
	ChangeNotify(const std::string& id);

	static void add(const std::string&,VNode*);
	void add(VNode*);

	VNodeList* data() const {return data_;};
	ChangeNotifyDialog* dialog() const {return dialog_;}
	ChangeNotifyModel* model() const {return model_;}

	//Called from VConfigLoader
	static void load(VProperty* group);

protected:
	void make();
	static ChangeNotify* find(const std::string&);

	std::string id_;
	VNodeList* data_;
	ChangeNotifyDialog* dialog_;
	ChangeNotifyModel* model_;

	VProperty* prop_;
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
