//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "Palette.hpp"

#include "UserMessage.hpp"

#include <QApplication>
#include <QDebug>
#include <QMap>
#include <QPalette>
#include <QRegExp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

static QMap<std::string,QPalette::ColorRole> paletteId;

Palette::Palette()
= default;

void Palette::load(const std::string& parFile)
{

	if(paletteId.isEmpty())
	{
  		paletteId["window"]=QPalette::Window;
		paletteId["windowtext"]=QPalette::WindowText;
		paletteId["base"]=QPalette::Base;
		paletteId["alternatebase"]=QPalette::AlternateBase;
		paletteId["tooltipbase"]=QPalette::ToolTipBase;
		paletteId["tooltiptext"]=QPalette::ToolTipText;
  		paletteId["text"]=QPalette::Text;
		paletteId["button"]=QPalette::Button;
		paletteId["buttontext"]=QPalette::ButtonText;
		paletteId["brighttext"]=QPalette::BrightText;
		paletteId["light"]=QPalette::Light;
		paletteId["midlight"]=QPalette::Midlight;
		paletteId["dark"]=QPalette::Dark;
		paletteId["mid"]=QPalette::Mid;
		paletteId["shadow"]=QPalette::Shadow;
		paletteId["higlight"]=QPalette::Highlight;
		paletteId["highlightedtext"]=QPalette::HighlightedText;
		paletteId["link"]=QPalette::Link;
		paletteId["linkvisited"]=QPalette::LinkVisited;
	}

	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(parFile,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true,
				 std::string("Error! Palette::load() unable to parse definition file: " + parFile + " Message: " +errorMessage));
		 return;
	}

	QPalette palette=qApp->palette();

	for(ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
	{
		std::string name=it->first;
		ptree ptItem=it->second;

		QPalette::ColorGroup group;
		if(name == "active")
			group=QPalette::Active;
		else if(name == "inactive")
			group=QPalette::Inactive;
		else if(name == "disabled")
			group=QPalette::Disabled;
		else
		{
			 UserMessage::message(UserMessage::ERROR, true,
							 std::string("Error! Palette::load() unable to identify group: " + name));
			 continue;
		}

		for(ptree::const_iterator itItem = ptItem.begin(); itItem != ptItem.end(); ++itItem)
		{
			std::string role=itItem->first;
			std::string val=itItem->second.get_value<std::string>();

			QMap<std::string,QPalette::ColorRole>::const_iterator itP=paletteId.find(role);
			if(itP != paletteId.end())
			{
                QColor col=toColour(val);
				if(col.isValid())
				{					
					palette.setColor(group,itP.value(),col);
				}
			}
		}
	}

	qApp->setPalette(palette);
}

void Palette::statusColours(QColor bg,QColor &bgLight,QColor &border)
{
    int lighter=150;
    if(bg.value() < 235)
        bgLight=bg.lighter(130);
    else
        bgLight=bg.lighter(lighter);

    //if(bg.hsvHue() < 58 && bg.hsvHue() > 50)
    //    bgLight=bg.lighter(170);

    border=bg.darker(120); //125 150
}

QColor Palette::toColour(const std::string& name)
{
    QString qn=QString::fromStdString(name);
    QColor col;
    QRegExp rx("rgb\\((\\d+),(\\d+),(\\d+)");

    if(rx.indexIn(qn) > -1 && rx.captureCount() == 3)
    {
        col=QColor(rx.cap(1).toInt(),
                  rx.cap(2).toInt(),
                  rx.cap(3).toInt());

    }
    return col;
}
