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
#include <QString>

class QWidget;
class InfoPanel;
class InfoProvider;
class VComboSettings;

//This is the (abstract) base class to represent one tab in the info panel.
//It cannot be inheried from QObject beacuse we would end up with double inheritance since
//all the derived calsses are inherited from QObject!!

class InfoPanelItem : public VTaskObserver, public InfoPresenter, public NodeObserver
{
friend class InfoPanel;

public:
    InfoPanelItem() :  unselectedFlags_(KeepContents) {}
	~InfoPanelItem() override;

    enum ChangeFlag {ActiveChanged=1,SelectedChanged=2,SuspendedChanged=4,FrozenChanged=8,DetachedChanged=16};
    typedef FlagSet<ChangeFlag> ChangeFlags;

    //What to do when the item is unselected
    enum UnselectedFlag {KeepContents=1,KeepActivity=2};
    typedef FlagSet<UnselectedFlag> UnselectedFlags;

	virtual void reload(VInfo_ptr info)=0;
	virtual QWidget* realWidget()=0;
	virtual void clearContents()=0;
    virtual bool hasSameContents(VInfo_ptr info);
    void setOwner(InfoPanel*);

    virtual void setActive(bool);
    void setSelected(bool,VInfo_ptr);
    void setSuspended(bool,VInfo_ptr);
	void setFrozen(bool);
	void setDetached(bool);

    bool isSuspended() const {return suspended_;}
    bool keepServerDataOnLoad() const {return keepServerDataOnLoad_;}

	//From VTaskObserver
    void taskChanged(VTask_ptr) override {}

	//From VInfoPresenter
    void infoReady(VReply*) override {}
    void infoFailed(VReply*) override {}
    void infoProgress(VReply*) override {}
    void infoProgressStart(int min,int max,const std::string& text) {}
    void infoProgress(int value,const std::string& text) {}

	//From NodeObserver
	void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) override;
	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) override {}

    virtual void writeSettings(VComboSettings* vs) {}
    virtual void readSettings(VComboSettings* vs) {}

protected:
	void adjust(VInfo_ptr);
    void linkSelected(const std::string& path);
    void linkSelected(VInfo_ptr);
    void relayInfoPanelCommand(VInfo_ptr info,QString cmd);
    void relayDashboardCommand(VInfo_ptr info,QString cmd);

    virtual void clear();
    virtual void updateState(const ChangeFlags&)=0;

	//Notifications about the server changes
	virtual void defsChanged(const std::vector<ecf::Aspect::Type>&)=0;
    virtual void connectStateChanged() {}
    virtual void suiteFilterChanged() {}
    virtual void serverSyncFinished() {}
	
	//Notifications about the node changes
	virtual void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&)=0;
	
    InfoPanel* owner_{nullptr};
    bool active_{false};
    bool selected_{false};
    bool suspended_{false};
    bool frozen_{false};
    bool detached_{false};
    UnselectedFlags unselectedFlags_;
    bool useAncestors_{false};
    bool handleAnyChange_{false};

    //do not reload the server data when a new info object is loaded. This
    //only makes sense if the this sever-related item(tab) is alawys visible
    //whatever node is selected!!!
    bool keepServerDataOnLoad_{false};
};

class InfoPanelItemFactory
{
public:
	explicit InfoPanelItemFactory(const std::string&);
	virtual ~InfoPanelItemFactory();

    virtual InfoPanelItem* make() = 0;
    static InfoPanelItem* create(const std::string& name);

private:
	explicit InfoPanelItemFactory(const InfoPanelItemFactory&) = delete;
	InfoPanelItemFactory& operator=(const InfoPanelItemFactory&) = delete;

};

template<class T>
class InfoPanelItemMaker : public InfoPanelItemFactory
{
    InfoPanelItem* make() override { return new T(); }
public:
	explicit InfoPanelItemMaker(const std::string& name) : InfoPanelItemFactory(name) {}
};

#endif
