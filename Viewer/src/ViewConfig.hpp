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

class VParameter
{
public:
		VParameter(QString name,const std::map<QString,QString>& attr);

		QString name() const {return name_;}
		QString attribute(QString);
		QVariant value() const {return value_;}

		QColor toColour() const {return value_.value<QColor>();}
		QFont toFont() const {return value_.value<QFont>();}
		QString toString() const {return value_.toString();}
		int toInt() const {return value_.toInt();}

protected:
		QColor toColour(QString);

		QString name_;
		std::map<QString,QString> attr_;
		QVariant default_;
		QVariant value_;
};


class ViewConfig
{
public:
		static ViewConfig* Instance();

		QColor   stateColour(DState::State) const;
		QString  stateName(DState::State) const;
		QString  stateShortName(DState::State) const;
		QColor   colour(QString) const;

		const std::string& configDir() const {return configDir_;}
		const std::string& rcDir() const {return rcDir_;}

protected:
		ViewConfig();

		void readParams(const std::string&);

		static ViewConfig* instance_;

		std::string configDir_;
		std::string rcDir_;

		std::map<DState::State,VParameter*> stateParams_;
		std::map<QString,VParameter*> params_;

};

#endif
