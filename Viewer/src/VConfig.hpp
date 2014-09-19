//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VCONFIG_HPP_
#define VCONFIG_HPP_

#include <vector>

class VConfig;

class ServerFilter;
class StateFilter;
class AttributeFilter;
class IconFilter;
class VFilter;

class VConfigItem
{
friend class VConfig;

public:
	VConfigItem(VConfig* owner) : owner_(owner) {};
	virtual ~VConfigItem() {};

	virtual void notifyOwner()=0;
	VConfig* owner_;
};

class VConfigObserver
{
public:
	VConfigObserver(){};
	virtual ~VConfigObserver(){};
	virtual void notifyConfigChanged(ServerFilter*)=0;
	virtual void notifyConfigChanged(StateFilter*)=0;
	virtual void notifyConfigChanged(AttributeFilter*)=0;
	virtual void notifyConfigChanged(IconFilter*)=0;
};

class VConfig
{
public:

	VConfig();
	~VConfig();

	enum Type {Server,State,Attribute,Icon};

	ServerFilter* serverFilter() const {return server_;}
	VFilter* stateFilter() const {return state_;}
	VFilter* attributeFilter() const {return attr_;}
	VFilter* iconFilter() const {return icon_;}

	void addObserver(VConfigObserver*);
	void removeObserver(VConfigObserver*);

	void changed(ServerFilter*);
	void changed(StateFilter*);
	void changed(AttributeFilter*);
	void changed(IconFilter*);
	void reloaded();

protected:


	std::vector<VConfigObserver*> observers_;

private:
	ServerFilter* server_;
	VFilter* state_;
	VFilter* attr_;
	VFilter* icon_;
};

#endif
