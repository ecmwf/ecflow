//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWCONFIG_HPP_
#define VIEWCONFIG_HPP_

#include <map>
#include <string>

#include <QColor>
#include <QFont>
#include <QVariant>

#include "DState.hpp"

/*
class StateAttribute
{
public:
		StateAttribute(QString name,QColor colour=QColor()) : name_(name), colour_(colour) {};
		QColor colour() const {return colour_;}
		QString name() const {return name_;}
		void colour(QColor c) {colour_=c;}

protected:
		QString name_;
		QColor colour_;
};
*/

class VParameter
{
public:
		VParameter(QString name,QString shortName,QString label,QString tooltip,QVariant def) :
			   name_(name), shortName_(shortName), label_(label), toolTip_(tooltip), default_(def), value_(def) {}



		QString name() const {return name_;}
		QString shortName() const {return shortName_;}
		QString toolTip() const {return toolTip_;}
		QColor toColour() const {return value_.value<QColor>();}
		QFont toFont() const {return value_.value<QFont>();}
		QString toString() const {return value_.toString();}
		int toInt() const {return value_.toInt();}

protected:
		QString name_;
		QString shortName_;
		QString label_;
		QString toolTip_;
		QVariant default_;
		QVariant value_;
};


class ViewConfig
{
public:
		static ViewConfig* Instance();

		enum PaletteItem {Unknown,Suspended,Complete,Queued,Submitted,Active,Aborted,Halted,Shutdown,Meter,Threshold,Event};
		enum FontItem {NormalFont,BoldFont,SmallFont,SmallBoldFont};

		QColor   stateColour(DState::State) const;
		QString  stateName(DState::State) const;
		QString  stateShortName(DState::State) const;
		QColor   colour(PaletteItem) const;
		QFont    font(FontItem) const;

protected:
		ViewConfig();

		QString name(PaletteItem) const;
		QString shortName(PaletteItem) const;

		static ViewConfig* instance_;
		std::map<PaletteItem,VParameter*> colour_;
		std::map<FontItem,VParameter*> font_;
		std::map<DState::State,PaletteItem> stateMap_;

};

#endif
