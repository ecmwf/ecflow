//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANELITEM_HPP_
#define INFOPANELITEM_HPP_

#include "FlagSet.hpp"
#include "NodeObserver.hpp"
#include "VInfo.hpp"
#include "InfoPresenter.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

#include <string>

class QWidget;
class InfoPanel;
class InfoProvider;
class VSettings;

//This is the (abstract) base class to represent one tab in the info panel.

class InfoPanelItem : public VTaskObserver, public InfoPresenter, public NodeObserver
{
friend class InfoPanel;

public:
    InfoPanelItem() : owner_(0), active_(false), selected_(false), suspended_(false),
                      frozen_(false), detached_(false), unselectedFlags_(KeepContents),
                      useAncestors_(false) {}
	virtual ~InfoPanelItem();

    enum ChangeFlag {ActiveChanged=1,SelectedChanged=2,SuspendedChanged=4,FrozenChanged=8,DetachedChanged=16};
    typedef FlagSet<ChangeFlag> ChangeFlags;

    //What to do when the item is unselected
    enum UnselectedFlag {KeepContents=1,KeepActivity=2};
    typedef FlagSet<UnselectedFlag> UnselectedFlags;

	virtual void reload(VInfo_ptr info)=0;
	virtual QWidget* realWidget()=0;
	virtual void clearContents()=0;

    void setOwner(InfoPanel*);

    virtual void setActive(bool);
    void setSelected(bool,VInfo_ptr);
    void setSuspended(bool,VInfo_ptr);
	void setFrozen(bool);
	void setDetached(bool);

    bool isSuspended() const {return suspended_;}

	//From VTaskObserver
    void taskChanged(VTask_ptr) {}

	//From VInfoPresenter
    void infoReady(VReply*) {}
    void infoFailed(VReply*) {}
    void infoProgress(VReply*) {}
    void infoProgressStart(int min,int max,const std::string& text) {}
    void infoProgress(int value,const std::string& text) {}

	//From NodeObserver
	void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) {}

    virtual void writeSettings(VSettings* vs) {}
    virtual void readSettings(VSettings* vs) {}

protected:
	void adjust(VInfo_ptr);
    virtual void clear();
    virtual void updateState(const ChangeFlags&)=0;
    void linkSelected(const std::string& path);

	//Notifications about the server changes
	virtual void defsChanged(const std::vector<ecf::Aspect::Type>&)=0;
    virtual void connectStateChanged() {}
    virtual void suiteFilterChanged() {}
    virtual void serverSyncFinished() {}
	
	//Notifications about the node changes
	virtual void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&)=0;
	
    InfoPanel* owner_;
    bool active_;
    bool selected_;
    bool suspended_;
    bool frozen_;
    bool detached_;
    UnselectedFlags unselectedFlags_;
    bool useAncestors_;
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
