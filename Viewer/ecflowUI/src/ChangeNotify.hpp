//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include "VProperty.hpp"

class ChangeNotifyDialog;
class ChangeNotifyModel;
class ChangeNotifyWidget;
class VProperty;
class VNode;
class VNodeList;

class QAbstractItemModel;
class QSortFilterProxyModel;

class ChangeNotify : public VPropertyObserver
{
public:
	explicit ChangeNotify(const std::string& id);

	const std::string& id() const {return id_;}
    VNodeList* data() const {return data_;}
	VProperty* prop() const {return prop_;}
    ChangeNotifyModel* model() const;
	QSortFilterProxyModel* proxyModel() const {return proxyModel_;}
	bool isEnabled() const {return enabled_;}
	void clearData();

    virtual QColor fillColour() const;
    virtual QColor textColour() const;
    QColor countFillColour() const;
    QColor countTextColour() const;
    QString toolTip() const;
    QString widgetText() const;

    static void showDialog(ChangeNotify* notifier=0);

	//Form VPropertyObserver
	void notifyChange(VProperty*);

	static void add(const std::string&,VNode*,bool,bool);
	static void remove(const std::string&,VNode*);
	static void populate(ChangeNotifyWidget* w);
    static void updateNotificationStateFromServer(const std::string& id,bool hasEnabledServer);

	//Called from VConfigLoader
	static void load(VProperty* group);

protected:
	void add(VNode*,bool,bool);
	void remove(VNode*);
	void setEnabled(bool);
    void updateNotificationState(bool hasEnabledServer);
    void setProperty(VProperty* prop);
	void loadServerSettings();

	static ChangeNotify* find(const std::string&);
	static ChangeNotifyDialog* dialog();

	std::string id_;
	bool enabled_;
	VNodeList* data_;
	ChangeNotifyModel* model_;
	QSortFilterProxyModel* proxyModel_;
	VProperty* prop_;
    VProperty* propEnabled_; //central settings in config GUI
	static ChangeNotifyDialog* dialog_;
};

class AbortedNotify : public ChangeNotify
{
public:
	explicit AbortedNotify(const std::string& id) : ChangeNotify(id) {}
    QColor fillColour() const;
    QColor textColour() const;
};

#endif
