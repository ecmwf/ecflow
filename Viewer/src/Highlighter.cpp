//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "Highlighter.hpp"
#include "UserMessage.hpp"
#include "VParam.hpp"

#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextLayout>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

std::string Highlighter::parFile_;

#include "VProperty.hpp"


Highlighter::Highlighter(QTextDocument *parent,QString id)
     : QSyntaxHighlighter(parent)
{
	load(id);
}

void Highlighter::addRule(QString pattern,QTextCharFormat format)
{
	HighlightingRule rule;
	rule.pattern = QRegExp(pattern,Qt::CaseSensitive);
	rule.format = format;
	rules_.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
	Q_FOREACH(HighlightingRule rule, rules_)
	{
		QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0)
        {
             int length = expression.matchedLength();
             setFormat(index, length, rule.format);
             index = text.indexOf(expression, index + length);
         }
     }
     setCurrentBlockState(0);
}

void Highlighter::init(const std::string& parFile)
{
	parFile_=parFile;
}

void Highlighter::load(QString id)
{
	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(parFile_,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true,
				 std::string("Error! Highlighter::load() unable to parse definition file: " + parFile_ + " Message: " +errorMessage));
		 return;
	}

	ptree::const_assoc_iterator itTop=pt.find(id.toStdString());
	if(itTop == pt.not_found())
	{
		return;
	}

	//For each parameter
	for(ptree::const_iterator itRule = itTop->second.begin(); itRule != itTop->second.end(); ++itRule)
	{
		QString pattern;
		QTextCharFormat format;

		ptree::const_assoc_iterator itPar;
		ptree ptPar=itRule->second;

		if((itPar=ptPar.find("pattern")) !=ptPar.not_found())
		{
			pattern=QString::fromStdString(itPar->second.get_value<std::string>());
		}
		if((itPar=ptPar.find("colour")) !=ptPar.not_found())
		{
			format.setForeground(VProperty::toColour(itPar->second.get_value<std::string>()));
		}
		if((itPar=ptPar.find("bold")) !=ptPar.not_found())
		{
			if(itPar->second.get_value<std::string>() == "true")
				format.setFontWeight(QFont::Bold);
		}
		if((itPar=ptPar.find("italic")) !=ptPar.not_found())
		{
			if(itPar->second.get_value<std::string>() == "true")
				format.setFontItalic(true);
		}
		addRule(pattern,format);
	}
}

void Highlighter::toHtml(QString& html)
{
    // Create a new document from all the selected text document.
    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);

    QTextDocument* tmpDoc(new QTextDocument());
    Q_ASSERT(tmpDoc);
    QTextCursor tmpCursor(tmpDoc);
    tmpCursor.insertFragment(cursor.selection());
    tmpCursor.select(QTextCursor::Document);

    // Set the default foreground for the inserted characters.
    //QTextCharFormat textfmt = tmpCursor.charFormat();
    //textfmt.setForeground(Qt::black);
    //tmpCursor.setCharFormat(textfmt);

    // Apply the additional formats set by the syntax highlighter
    QTextBlock start = document()->findBlock(cursor.selectionStart());
    QTextBlock end = document()->findBlock(cursor.selectionEnd());
    end = end.next();

    const int selectionStart = cursor.selectionStart();
    const int endOfDocument = tmpDoc->characterCount() - 1;
    for(QTextBlock current = start; current.isValid() and current not_eq end; current = current.next())
    {
        const QTextLayout* layout(current.layout());

        Q_FOREACH(const QTextLayout::FormatRange &range, layout->additionalFormats())
        {
            const int start = current.position() + range.start - selectionStart;
            const int end = start + range.length;
            if(end <= 0 or start >= endOfDocument)
                continue;
            tmpCursor.setPosition(qMax(start, 0));
            tmpCursor.setPosition(qMin(end, endOfDocument), QTextCursor::KeepAnchor);
            tmpCursor.setCharFormat(range.format);
        }
    }

    // Reset the user states since they are not interesting
    for(QTextBlock block = tmpDoc->begin(); block.isValid(); block = block.next())
        block.setUserState(-1);

    // Make sure the text appears pre-formatted, and set the background we want.
    tmpCursor.select(QTextCursor::Document);
    QTextBlockFormat blockFormat = tmpCursor.blockFormat();
    blockFormat.setNonBreakableLines(true);
    blockFormat.setBackground(Qt::black);
    tmpCursor.setBlockFormat(blockFormat);

    // Finally retreive the syntax higlighted and formatted html.
    html = tmpCursor.selection().toHtml();
    delete tmpDoc;
}
