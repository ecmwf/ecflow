//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TextPagerSearchHighlighter.hpp"

#include "VConfig.hpp"
#include "VProperty.hpp"

#include <QDebug>

QColor TextPagerSearchHighlighter::bgColour_=QColor(200, 255, 200);

TextPagerSearchHighlighter::TextPagerSearchHighlighter(QObject *parent) :
  SyntaxHighlighter(parent),
  mode_(NoMode),
  caseSensitive_(false)
 {
	if(VProperty *p=VConfig::instance()->find("panel.search.highlightColour"))
	{
		bgColour_=p->value().value<QColor>();
	}

	format_.setBackground(Qt::yellow);

	rx_=QRegExp("(server)");
 }

void TextPagerSearchHighlighter::highlightBlock(const QString &string)
{
	if(string.simplified().isEmpty())
		return;

	if(mode_ == RegexpMode)
	{
		int index=0;
		while((index = rx_.indexIn(string, index)) != -1) {

			if(rx_.matchedLength() == 0)
				return;
			setFormat(index, rx_.matchedLength(), format_);
			index+=rx_.matchedLength();
		}
	}
	else if(mode_ == TextMode)
	{
		int index=0;
		while((index = string.indexOf(text_,index,
			   caseSensitive_?Qt::CaseSensitive:Qt::CaseInsensitive)) != -1)
		{
			setFormat(index, text_.size(), format_);
			index+=string.size();
		}
	}
}

void TextPagerSearchHighlighter::reset(QString txt,TextPagerDocument::FindMode mode,bool apply)
{
	bool changed=false;
	if(mode_ != TextMode)
	{
		mode_=TextMode;
		rx_=QRegExp();
		changed=true;
	}

	if(txt != text_)
	{
		text_=txt;
		changed=true;
	}

	bool cs=mode & TextPagerDocument::FindCaseSensitively;
	if(cs != caseSensitive_)
	{
		caseSensitive_=cs;
		changed=true;
	}

	if(changed && apply)
		rehighlight();

}

void TextPagerSearchHighlighter::reset(QRegExp rx,TextPagerDocument::FindMode mode,bool apply)
{
	bool changed=false;
	if(mode_ != RegexpMode)
	{
		mode_=RegexpMode;
		text_.clear();
		changed=true;
	}

	if(rx_.pattern() != rx.pattern() || rx_.caseSensitivity() != rx.caseSensitivity() ||
	   rx_.patternSyntax() !=  rx.patternSyntax())
	{
		rx_=rx;
		changed=true;
	}

	if(changed && apply)
		rehighlight();
}



