//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WhyItemWidget.hpp"

#include <QDebug>

#include "Node.hpp"

#include "VConfig.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

//========================================================
//
// WhyItemWidget
//
//========================================================

WhyItemWidget::WhyItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
	messageLabel_->hide();
	fileLabel_->hide();
	textEdit_->setShowLineNumbers(false);
    textEdit_->setHyperlinkEnabled(true);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.why.font"));

    //Set css for the text formatting
    QString cssDoc="a:link { text-decoration:none; color: #0645AD;}";
    textEdit_->document()->setDefaultStyleSheet(cssDoc);


    connect(textEdit_,SIGNAL(hyperlinkActivated(QString)),
            this,SLOT(anchorClicked(QString)));
}

WhyItemWidget::~WhyItemWidget()
{
}


QWidget* WhyItemWidget::realWidget()
{
	return this;
}

void WhyItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
	info_=info;

    if(info_)
	{
        //textEdit_->setPlainText(why());
        textEdit_->appendHtml(why());
	}
}

void WhyItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
}

QString WhyItemWidget::why() const
{
	QString s;

	std::vector<std::string> theReasonWhy;

	if(info_ && info_.get())
	{
		if(info_->isServer())
		{
			info_->node()->why(theReasonWhy);
		}
		else if(info_->isNode() && info_->node())
		{
			info_->node()->why(theReasonWhy);
		}
	}

    s=makeHtml(theReasonWhy);
    return s;
}


QString WhyItemWidget::makeHtml(const std::vector<std::string>& rawTxt) const
{
    QString s;
    for(std::vector<std::string>::const_iterator it=rawTxt.begin(); it != rawTxt.end(); ++it)
    {
        QString line=QString::fromStdString(*it);
        QRegExp rx("'\\S+:(\\S+)'");
        if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
        {
            QString path=rx.cap(1);
            rx=QRegExp("'(\\S+):");
            QString type,typeOri;
            if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
            {
                typeOri=rx.cap(1);
                type=typeOri.toLower();
            }

            QString anchor=QString::fromStdString(VItemPathParser::encode(path.toStdString(),type.toStdString()));
            line.replace("\'" + typeOri + ":"," " + typeOri + " ");
            line.replace(path + "'","<a href=\'" + anchor  + "\'>" + path +"</a>");

        }
        else
        {
            rx=QRegExp("\\s+(/\\S+)\\b");
            if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
            {
                QString path=rx.cap(1);
                rx=QRegExp("(SUITE|FAMILY|TASK|ALIAS)");
                QString type;
                if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
                {
                    type=rx.cap(1);
                }

                QString anchor=QString::fromStdString(VItemPathParser::encode(path.toStdString(),type.toStdString()));
                line.replace(path,"<a href=\'" + anchor  + "\'>" + path +"</a>");
            }
        }



        s+=line+"<br>";
    }
    return s;
}

void WhyItemWidget::anchorClicked(QString link)
{
    linkSelected(link.toStdString());
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");

