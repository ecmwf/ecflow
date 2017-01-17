//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VPARAM_HPP_
#define VPARAM_HPP_

#include <QColor>

#include "VProperty.hpp"

class VParam : public VPropertyObserver
{
public:
		explicit VParam(const std::string& name);
        ~VParam();

		QString name() const {return qName_;}
		const std::string& strName() const {return name_;}

		QString label() const {return label_;}
		QColor colour() const {return colour_;}
		QColor fontColour() const {return fontColour_;}
        QColor typeColour() const {return typeColour_;}

		void setProperty(VProperty*);
		
		void notifyChange(VProperty*);

		/*
		int number(const std::string&) const;
		QColor colour(const std::string&) const;
		std::string text(const std::string&) const;
		QFont font(const std::string&) const;

		static QColor toColour(const std::string&) ;
		static QFont  toFont(const std::string&);
		static int    toNumber(const std::string&);
		static bool isColour(const std::string&);
		static bool isFont(const std::string&);
		static bool isNumber(const std::string&);*/

protected:
		//void addAttributes(const std::map<std::string,std::string>& attr);
		//static void init(const std::string& parFile,const std::string id,std::map<std::string,std::map<std::string,std::string> >& vals);

		std::string name_;
		QString qName_;
		QString label_;

		//Cached information
		QColor colour_;
		QColor fontColour_;
        QColor typeColour_;

        /*std::map<std::string,int> numberMap_;
		std::map<std::string,std::string> textMap_;
 		std::map<std::string,QColor> colourMap_;
		std::map<std::string,QFont> fontMap_;*/

		VProperty* prop_;
		QString colourPropName_;
        QString fontColourPropName_;
        QString typeColourPropName_;
};

#endif
