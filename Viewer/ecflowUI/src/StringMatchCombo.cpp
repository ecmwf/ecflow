//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "StringMatchCombo.hpp"

#include <QMenu>
#include <QVariant>

StringMatchTb::StringMatchTb(QWidget* parent) : QToolButton(parent)
{
   QIcon ic(QPixmap(":/viewer/edit.svg")); 
   setIcon(ic);
   setAutoRaise(true);
   QMenu *menu=new QMenu(this);
   menu->addAction("Contains"); //,StringMatchMode::ContainsMatch);
   menu->addAction("Matches"); //,StringMatchMode::WildcardMatch);
   menu->addAction("Regexp"); //,StringMatchMode::RegexpMatch);
   setMenu(menu);
   setPopupMode(QToolButton::InstantPopup);
}
    
StringMatchCombo::StringMatchCombo(QWidget* parent) : QComboBox(parent)
{
    addItem("contains",StringMatchMode::ContainsMatch);
    addItem("matches",StringMatchMode::WildcardMatch);
    addItem("regexp",StringMatchMode::RegexpMatch);

	setCurrentIndex(1);
}

StringMatchMode::Mode StringMatchCombo::matchMode(int index) const
{
	if(index >=0 && index < count())
	{
		return static_cast<StringMatchMode::Mode>(itemData(index).toInt());
	}
	return StringMatchMode::WildcardMatch;
}

StringMatchMode::Mode StringMatchCombo::currentMatchMode() const
{
	return matchMode(currentIndex());
}

void StringMatchCombo::setMatchMode(const StringMatchMode& mode)
{
	int im=mode.toInt();
	for(int i=0; i < count(); i++)
	{
		if(itemData(i).toInt() == im)
		{
			setCurrentIndex(i);
		}
	}
}
