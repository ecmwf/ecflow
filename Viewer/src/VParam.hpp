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
#include <QVariant>

class VParam
{
public:
		enum Type {NoType,
			       UnknownState,CompleteState,QueuedState,AbortedState,SubmittedState,ActiveState,SuspendedState,
			       NoAttribute,LabelAttribute,MeterAttribute,EventAttribute,RepeatAttribute,TimeAttribute,DateAttribute,
				   TriggerAttribute,VarAttribute,GenVarAttribute,LateAttribute,LimitAttribute,LimiterAttribute,
			       NoIcon,WaitIcon,RerunIcon,MessageIcon,CompleteIcon,TimeIcon,DateIcon,ZombieIcon,LateIcon};

		VParam(QString name,VParam::Type);
		VParam(QString name,const std::map<QString,QString>& attr);

		QString name() const {return name_;}
		VParam::Type type() const {return type_;}

		int number(QString) const;
		QColor colour(QString) const;
		QString text(QString) const;
		QFont font(QString) const;

		void addAttributes(const std::map<QString,QString>& attr);

		static int  toInt(VParam::Type);
		static VParam::Type  toType(int);

protected:
		QString name_;
		Type type_;

private:
		QColor toColour(QString) const;
		QFont  toFont(QString) const;
		int    toNumber(QString) const;
		bool isColour(QString) const;
		bool isFont(QString) const;
		bool isNumber(QString) const;

		std::map<QString,int> numberMap_;
		std::map<QString,QString> textMap_;
 		std::map<QString,QColor> colourMap_;
		std::map<QString,QFont> fontMap_;
};


#endif
