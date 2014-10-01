//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VPARAM_HPP_
#define VPARAM_HPP_

#include <map>

#include <QColor>
#include <QFont>
#include <QMap>

class VParam
{
public:

		VParam(const std::string& name);

		QString qName() const {return qName_;}
		std::string name() const {return name_;}

		int number(const std::string&) const;
		QColor colour(const std::string&) const;
		std::string text(const std::string&) const;
		QFont font(const std::string&) const;

protected:
		void addAttributes(const std::map<std::string,std::string>& attr);
		static void init(const std::string& parFile,const std::string id,std::map<std::string,std::map<std::string,std::string> >& vals);

		std::string name_;
		QString qName_;

private:
		QColor toColour(const std::string&) const;
		QFont  toFont(const std::string&) const;
		int    toNumber(const std::string&) const;
		bool isColour(const std::string&) const;
		bool isFont(const std::string&) const;
		bool isNumber(const std::string&) const;

		std::map<std::string,int> numberMap_;
		std::map<std::string,std::string> textMap_;
 		std::map<std::string,QColor> colourMap_;
		std::map<std::string,QFont> fontMap_;
};

#endif
