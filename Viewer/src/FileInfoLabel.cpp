//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FileInfoLabel.hpp"

#include <QVariant>

#include "VFileInfo.hpp"
#include "VReply.hpp"

FileInfoLabel::FileInfoLabel(QWidget* parent) : QLabel(parent)
{
	//Define id for the css
	setProperty("fileInfo","1");

    //Set size policy
	/*QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);
    //setMinimumSize(QSize(0, 60));
    //setMaximumSize(QSize(16777215, 45));*/

	setMargin(2);
	setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	//Other settings
    setAutoFillBackground(true);

    setFrameShape(QFrame::StyledPanel);
    setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

}

void FileInfoLabel::update(VReply* reply)
{
	if(!reply)
		clear();

	QString labelText;
	QString ttText;
	QString s;

	QColor col(Qt::black);
	QColor colText("#000010");
	QColor colSize(0,0,255);
	QColor colErr(255,0,0);

	QString fileName=QString::fromStdString(reply->fileName());

	if(fileName.isEmpty())
	{
		s="<b><font color=" + col.name() + ">File: </font></b>";
		s+="<font color=" + colErr.name() + "> ??? </font>";
		setText(s);
		setToolTip(QString());
		return;
	}

	//Name
	labelText="<b><font color=" + col.name() + ">File: </font></b>";
	labelText+="<font color=" +colText.name() + ">" + fileName + "</font>";

	VFileInfo f(fileName);

	s="";

	//Local read
	if(reply->fileReadMode() == VReply::LocalReadMode)
	{
		VFileInfo f(fileName);
		if(f.exists())
		{
			//s+="<br>";

			labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
			labelText+="<font color=" + colSize.name() + "> " + f.formatSize() + "</font>";

			s+="<b><font color=" + col.name() + "> Permissions: </font></b>";
			s+="<font color=" + colText.name() + ">" + f.formatPermissions() + "</font>";

			s+="<b><font color=" + col.name() + "> Owner: </font></b>";
			s+="<font color=" + colText.name() + ">" + f.owner() + "</font>";

			s+="<b><font color=" + col.name() + "> Group: </font></b>";
			s+="<font color=" + colText.name() + ">" + f.group() + "</font>";

			s+="<b><font color=" + col.name() + "> Modified: </font></b>";
			s+="<font color=" + colText.name() + ">" + f.formatModDate() + "</font>";

			s+="<br>";
			s+="<b><font color=" + col.name() + "> Access method: </font></b>";
			s+="<font color=" + colText.name() + "> read from disk</font>";
		}
	}
	else if(reply->fileReadMode() == VReply::ServerReadMode)
	{
		//s+="<br>";
		s+="<b><font color=" + col.name() + "> Access method: </font></b>";
		int rowLimit=10000;
		s+="<font color=" + colText.name() + "> through server (first " + QString::number(rowLimit) + "lines)</font>";
	}

	else if(reply->fileReadMode() == VReply::LogServerReadMode)
	{
		VFile_ptr tmp=reply->tmpFile();
		if(tmp && tmp.get())
		{
			VFileInfo f(QString::fromStdString(tmp->path()));
			if(f.exists())
			{
				//s+="<br>";
				labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
				labelText+="<font color=" + colSize.name() + "> " + f.formatSize() + "</font>";
			}
		}
		s+="<br>";
		s+="<b><font color=" + col.name() + "> Access method: </font></b>";
		s+="<font color=" + colText.name() + "> " + QString::fromStdString(reply->fileReadMethod()) + "</font>";
	}

	ttText=s;

	setText(labelText);
	setToolTip(ttText);
}

void DirInfoLabel::update(VDir_ptr dir)
{
	if(!dir)
		clear();

	QString s;
	QColor col(Qt::black);
	QColor colText("#000010");
	QColor colSize(0,0,255);
	QColor colErr(255,0,0);

	QString dirName=QString::fromStdString(dir->path());

	if(dirName.isEmpty())
	{
		s="<b><font color=" + col.name() + ">Directory: </font></b>";
		s+="<font color=" + colErr.name() + "> ??? </font>";
		setText(s);
		return;
	}

	//Name
	s="<b><font color=" + col.name() + ">Directory: </font></b>";
	s+="<font color=" +colText.name() + ">" + dirName + "</font>";

	//Where
	QString where=QString::fromStdString(dir->where());
	if(where.isEmpty())
		where="???";

	s+="<br>";
	s+="<b><font color=" + col.name() + ">Host: </font></b>";
	s+="<font color=" +colText.name() + ">" + where + "</font>";

	setText(s);
}
