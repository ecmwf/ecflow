//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "IconProvider.hpp"

#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <QPainter>

static UnknownIconItem unknownIcon(":/desktop/unknown.svg");
static IconItem linkIcon(":/desktop/link.svg");
static IconItem linkBrokenIcon(":/desktop/link_broken.svg");
static IconItem lockIcon(":/viewer/padlock.svg");
static IconItem warningIcon(":/viewer/warning.svg");
static IconItem errorIcon(":/viewer/error.svg");
static IconItem bookmarkGroupIcon(":/desktop/bookmark_group.svg");
static IconItem embeddedIcon(":/desktop/embedded.svg");
static IconItem infoIcon(":/viewer/info.svg");

std::map<QString,IconItem*> IconProvider::icons_;
std::map<int,IconItem*> IconProvider::iconsById_;

static int idCnt=0;

//===========================================
//
// IconItem
//
//===========================================

IconItem::IconItem(QString path) : path_(path), id_(idCnt++)
{
}

QPixmap IconItem::pixmap(int size)
{
 	 std::map<int,QPixmap>::iterator it=pixmaps_.find(size);
	 if(it != pixmaps_.end())
	   	return it->second;
	 else
	 {
	   	QPixmap pix;
		QImageReader imgR(path_);
		if(imgR.canRead())
		{
			imgR.setScaledSize(QSize(size,size));
			QImage img=imgR.read();
			pix=QPixmap::fromImage(img);
		}
		else
		{
		  	pix=unknown(size);
		}

		pixmaps_[size]=pix;
		return pix;
	 }
	 return QPixmap();
}

QPixmap IconItem::unknown(int size)
{
	return unknownIcon.pixmap(size);
}

UnknownIconItem::UnknownIconItem(QString path) : IconItem(path)
{

}

QPixmap UnknownIconItem::unknown(int size)
{
	return QPixmap();
}


//===========================================
//
// IconProvider
//
//===========================================

IconProvider::IconProvider()
{
}

int IconProvider::add(QString path,QString name)
{
	std::map<QString,IconItem*>::iterator it=icons_.find(name);
	if(it == icons_.end())
	{
		IconItem *p=new IconItem(path);
		icons_[name]=p;
		iconsById_[p->id()]=p;
		return p->id();
	}

	return it->second->id();
}

IconItem* IconProvider::icon(QString name)
{
	std::map<QString,IconItem*>::iterator it=icons_.find(name);
	if(it != icons_.end())
		return it->second;

	return &unknownIcon;
}

IconItem* IconProvider::icon(int id)
{
	std::map<int,IconItem*>::iterator it=iconsById_.find(id);
	if(it != iconsById_.end())
		return it->second;

	return &unknownIcon;
}

QPixmap IconProvider::pixmap(QString name,int size)
{
	return icon(name)->pixmap(size);
}

QPixmap IconProvider::pixmap(int id,int size)
{
	return icon(id)->pixmap(size);
}

QPixmap IconProvider::lockPixmap(int size)
{
	 return lockIcon.pixmap(size);
}

QPixmap IconProvider::warningPixmap(int size)
{
	 return warningIcon.pixmap(size);
}

QPixmap IconProvider::errorPixmap(int size)
{
	 return errorIcon.pixmap(size);
}

QPixmap IconProvider::infoPixmap(int size)
{
	 return infoIcon.pixmap(size);
}

static IconProvider	iconProvider;
