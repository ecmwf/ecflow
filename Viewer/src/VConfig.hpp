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
class VNStateSet;
class AttributeFilter;
class IconFilter;
class VParamSet;
class VSettings;

class NodeFilter;


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
	virtual void notifyConfigChanged(VNStateSet*)=0;
	virtual void notifyConfigChanged(AttributeFilter*)=0;
	virtual void notifyConfigChanged(IconFilter*)=0;

	virtual void notifyServerSetChanged()=0;
	virtual void notifyIconSetChanged()=0;
	virtual void notifyNodeFilterChanged()=0;
};

class VConfig
{
public:

	VConfig();
	~VConfig();

	enum Type {Server,State,Attribute,Icon};

	void addServerSet(ServerFilter* server=0);

	ServerFilter* serverSet() const {return server_;}
	//NodeFilter* filter() const { return filter_;}
	VParamSet*  iconSet() const {return icon_;}

	/*VParamSet* stateFilter() const {return state_;}
	VParamSet* attributeFilter() const {return attr_;}
	VParamSet* iconFilter() const {return icon_;}*/

	void addObserver(VConfigObserver*);
	void removeObserver(VConfigObserver*);

	void changed(ServerFilter*) {};
	void changed(IconFilter*) {};
	void changed(NodeFilter*) {};

	/*void changed(StateFilter*);
	void changed(AttributeFilter*);
	void changed(IconFilter*);*/


	void reloaded();

	void writeSettings(VSettings*);
	void readSettings(VSettings*);

protected:


	std::vector<VConfigObserver*> observers_;

private:
	ServerFilter* server_;
	//VParamSet* state_;
	//VParamSet* attr_;
	VParamSet* icon_;
	//NodeFilter* filter_;
};

#endif
