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

#include "ViewNodeInfo.hpp"
#include <string>

class QWidget;

class InfoPanelItem
{
public:
	InfoPanelItem() : loaded_(false) {};
	virtual ~InfoPanelItem(){};

	bool loaded() const {return loaded_;}
	virtual void reload(ViewNodeInfo_ptr node)=0;
	virtual QWidget* realWidget()=0;
	virtual void clearContents()=0;

protected:
	bool loaded_;

};


class InfoPanelItemFactory
{
public:
	InfoPanelItemFactory(const std::string&);
	virtual ~InfoPanelItemFactory();

	virtual InfoPanelItem* make() = 0;
	static InfoPanelItem* create(const std::string& name);

private:
	InfoPanelItemFactory(const InfoPanelItemFactory&);
	InfoPanelItemFactory& operator=(const InfoPanelItemFactory&);

};

template<class T>
class InfoPanelItemMaker : public InfoPanelItemFactory
{
	InfoPanelItem* make() { return new T(); }
public:
	InfoPanelItemMaker(const std::string& name) : InfoPanelItemFactory(name) {}
};

#endif
