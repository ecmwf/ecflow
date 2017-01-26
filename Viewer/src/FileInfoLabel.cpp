//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FileInfoLabel.hpp"

#include <QDateTime>
#include <QVariant>

#include "VFileInfo.hpp"
#include "VReply.hpp"

FileInfoLabel::FileInfoLabel(QWidget* parent) : QLabel(parent)
{
	//Define id for the css
	setProperty("fileInfo","1");
	setWordWrap(true);

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

void FileInfoLabel::update(VReply* reply,QString extraText)
{
	if(!reply)
		clear();

	QString labelText;
	QString ttText;
	QString s;

	QColor col(39,49,101);
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

	//VFileInfo f(fileName);

	s="";

	//Local read
	if(reply->fileReadMode() == VReply::LocalReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            VFileInfo fInfo(QString::fromStdString(f->path()));
            if(fInfo.exists())
            {
                labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                labelText+="<font color=" + fileSizeColour(fInfo.size()).name() + "> " + fInfo.formatSize() + "</font>";
                s+="<b><font color=" + col.name() + "> Modified: </font></b>";
                s+="<font color=" + colText.name() + ">" + fInfo.formatModDate() + "</font>";

            }
        }
        else
        {
            VFileInfo f(fileName);
            if(f.exists())
            {
                labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                labelText+="<font color=" + fileSizeColour(f.size()).name() + "> " + f.formatSize() + "</font>";

                s+="<b><font color=" + col.name() + "> Modified: </font></b>";
                s+="<font color=" + colText.name() + ">" + f.formatModDate() + "</font>";
            }
         }

         s+="<br>";
         s+="<b><font color=" + col.name() + "> Source: </font></b>";
         s+="<font color=" + colText.name() + "> read from disk</font>";

         if(f)
         {
             QString dt=f->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
             s+="<b><font color=" + col.name() + "> at </font></b>";
             s+="<font color=" + colText.name() + ">" + dt +  + "</font>";
         }

	}
	else if(reply->fileReadMode() == VReply::ServerReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
        {
            if(f->storageMode() == VFile::MemoryStorage)
            {
                labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                labelText+="<font color=" + fileSizeColour(f->dataSize()).name() + "> " + VFileInfo::formatSize(f->dataSize()) + "</font>";
            }
            else
            {
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
                {
                    //s+="<br>";
                    labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                    labelText+="<font color=" + fileSizeColour(fInfo.size()).name() + "> " + fInfo.formatSize() + "</font>";
                }
            }

            s+="<br>";
            s+="<b><font color=" + col.name() + "> Source: </font></b>";
            s+="<font color=" + colText.name() + "> " + QString::fromStdString(f->fetchModeStr()) + "</font>";

            QString dt=f->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
            s+="<b><font color=" + col.name() + "> at </font></b>";
            s+="<font color=" + colText.name() + ">" + dt +  + "</font>";

            int rowLimit=f->truncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }
            s+="</font>";
        }
        else if(reply->status() == VReply::TaskDone)
        {
            QString dt=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            //s+="<b><font color=" + col.name() + "> Fetched: </font></b>";
            //s+="<font color=" + colText.name() + ">" + dt +  + "</font>";

            s+="<br>";
            s+="<b><font color=" + col.name() + "> Source: </font></b><font color=" + colText.name() + "> fetched from server </font>" +
			//"<font color=" + colHighlight.name() +	"server </font>" +
			" <font color=" + col.name() + "> <b>at</b> </font><font color=" + colText.name() + ">" + dt + "</font>";

            int rowLimit=reply->readTruncatedTo();
            if(rowLimit >= 0)
            {
                s+=" (<i>text truncated to last " + QString::number(rowLimit) + " lines</i>)";
            }
            s+="</font>";
        }
        else
        {
            QString dt=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            s+="<br>Fetch attempted from server<font color=" + col.name() + "> <b>at</b> </font>" +  dt;
        }
	}

	else if(reply->fileReadMode() == VReply::LogServerReadMode)
	{
        VFile_ptr f=reply->tmpFile();
        if(f)
		{         
            if(f->storageMode() == VFile::MemoryStorage)
			{
				labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                labelText+="<font color=" + fileSizeColour(f->dataSize()).name() + "> " + VFileInfo::formatSize(f->dataSize()) + "</font>";
			}
			else
			{
                VFileInfo fInfo(QString::fromStdString(f->path()));
                if(fInfo.exists())
				{
					//s+="<br>";
					labelText+="<b><font color=" + col.name() + "> Size: </font></b>";
                    labelText+="<font color=" + fileSizeColour(fInfo.size()).name() + "> " + fInfo.formatSize() + "</font>";
				}
			}

			s+="<br>";
			s+="<b><font color=" + col.name() + "> Source: </font></b>";
            s+="<font color=" + colText.name() + "> " + QString::fromStdString(f->fetchModeStr()) + "</font>";

			//s+=" (took <font color=" + colSize.name() + "> " + QString::number(static_cast<float>(tmp->transferDuration())/1000.,'f',1) + " s </font>)";
            s+=" (took " + QString::number(static_cast<float>(f->transferDuration())/1000.,'f',1) + " s)";

            QString dt=f->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
			s+="<b><font color=" + col.name() + "> at </font></b>";
			s+="<font color=" + colText.name() + ">" + dt +  + "</font>";
            if(f->cached())
            {
                s+=" (<b> read from cache</b>)";
            }
		}
	}

	ttText=s;
	labelText += ttText;
	if(!extraText.isEmpty())
	{
		labelText +=" <i>" + extraText + "</i>";
	}

	setText(labelText);
    //setToolTip(ttText);
}

QColor FileInfoLabel::fileSizeColour(qint64 size) const
{
	QColor col(0,0,255);
	if(size > 10*1024*1024)
		col=QColor(Qt::red);
	/*else if( size > 10*1024*1024)
		col=QColor(255,166,0);*/

	return col;
}

void DirInfoLabel::update(VReply* reply) //VDir_ptr dir)
{
    VDir_ptr dir=reply->directory();

    if(!dir)
		clear();

	QString s;

    QColor col(39,49,101);
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

#if 0
	//Name
    s="<b><font color=" + col.name() + ">Dir: </font></b>";
	s+="<font color=" +colText.name() + ">" + dirName + "</font>";
#endif
    //Local read
    if(dir->fetchMode() == VDir::LocalFetchMode)
    {
        s+="<b><font color=" + col.name() + "> Directory: </font></b>";
        s+="<font color=" + colText.name() + "> read from disk</font>";

        QString dt=dir->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
        s+="<b><font color=" + col.name() + "> at </font></b>";
        s+="<font color=" + colText.name() + ">" + dt +  + "</font>";

    }
    else if(dir->fetchMode() == VDir::LogServerFetchMode)
    {
        s+="<b><font color=" + col.name() + "> Directory: </font></b>";
        s+="<font color=" + colText.name() + "> " + QString::fromStdString(dir->fetchModeStr()) + "</font>";

        QString dt=dir->fetchDate().toString("yyyy-MM-dd HH:mm:ss");
        s+="<b><font color=" + col.name() + "> at </font></b>";
        s+="<font color=" + colText.name() + ">" + dt +  + "</font>";

    }

	setText(s);
}
