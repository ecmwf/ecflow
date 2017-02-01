//============================================================================
// Copyright 2009-2017 ECMWF.
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
  caseSensitive_(false),
  wholeWords_(false)
 {
	if(VProperty *p=VConfig::instance()->find("panel.search.highlightColour"))
	{
		bgColour_=p->value().value<QColor>();
	}

	format_.setBackground(Qt::yellow);

	rx_=QRegExp("(server)");
 }

//from QRegExp
bool TextPagerSearchHighlighter::isWordCharacter(const QChar& ch) const
{
	return ch.isLetterOrNumber() || ch.isMark() || ch == QLatin1Char('_');
}

void TextPagerSearchHighlighter::highlightBlock(const QString &string)
{
	if(string.simplified().isEmpty())
		return;

	if(mode_ == RegexpMode)
	{
        if(rx_.isEmpty()) return;
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
        if(text_.isEmpty()) return;
        int index=0;
		while((index = string.indexOf(text_,index,
			   caseSensitive_?Qt::CaseSensitive:Qt::CaseInsensitive)) != -1)
		{
			bool found=true;
			if(wholeWords_)
			{
				if(index>0)
					found=!isWordCharacter(string.at(index-1));
				if(found && index + text_.size() < string.size())
					found=!isWordCharacter(string.at(index+text_.size()));
			}
			if(found)
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

	bool ww=mode & TextPagerDocument::FindWholeWords;
	if(ww != wholeWords_)
	{
		wholeWords_=ww;
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

void TextPagerSearchHighlighter::clear()
{
    rx_=QRegExp();
    text_.clear();
    rehighlight();
}


