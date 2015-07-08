//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANELITEM_HPP_
#define INFOPANELITEM_HPP_

#include "NodeObserver.hpp"
#include "VInfo.hpp"
#include "InfoPresenter.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

#include <string>

class QWidget;
class InfoPanel;
class InfoProvider;

//This is the (abstract) base class to represent one tab in the info panel.

class InfoPanelItem : public VTaskObserver, public InfoPresenter, public NodeObserver
{
friend class InfoPanel;

public:
	InfoPanelItem() : loaded_(false), useAncestors_(false),
	                  frozen_(false), detached_(false) {};
	virtual ~InfoPanelItem(){};

	bool loaded() const {return loaded_;}
	virtual void reload(VInfo_ptr info)=0;
	virtual QWidget* realWidget()=0;
	virtual void clearContents()=0;
	void setFrozen(bool);
	void setDetached(bool);

	//From VTaskObserver
	void taskChanged(VTask_ptr) {};

	//From VInfoPresenter
	void infoReady(VReply*) {};
	void infoFailed(VReply*) {};
	void infoProgress(VReply*) {};

	//From NodeObserver
	void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) {}

protected:
	void adjust(VInfo_ptr);
	void clear();

	//Notifications about the server changes
	virtual void defsChanged(const std::vector<ecf::Aspect::Type>&)=0;
	virtual void connectStateChanged() {};
	virtual void suiteFilterChanged() {};
	
	//Notifications about the node changes
	virtual void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&)=0;
	
	bool loaded_;
    bool useAncestors_;
    bool frozen_;
    bool detached_;
};

class InfoPanelItemFactory
{
public:
	explicit InfoPanelItemFactory(const std::string&);
	virtual ~InfoPanelItemFactory();

	virtual InfoPanelItem* make() = 0;
	static InfoPanelItem* create(const std::string& name);

private:
	explicit InfoPanelItemFactory(const InfoPanelItemFactory&);
	InfoPanelItemFactory& operator=(const InfoPanelItemFactory&);

};

template<class T>
class InfoPanelItemMaker : public InfoPanelItemFactory
{
	InfoPanelItem* make() { return new T(); }
public:
	explicit InfoPanelItemMaker(const std::string& name) : InfoPanelItemFactory(name) {}
};

#endif
