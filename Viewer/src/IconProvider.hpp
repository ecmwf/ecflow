//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ICONPROVIDER_HPP_
#define ICONPROVIDER_HPP_

#include <QPixmap>

#include <map>

class IconItem
{
public:
  	explicit IconItem(QString);
	QPixmap pixmap(int);
	int id () const {return id_;}

protected:
  	static void greyOut(QImage &);

  	QString path_;
	std::map<int,QPixmap> pixmaps_;
	int id_;
};


class IconProvider
{
public:
	IconProvider();

	static int add(QString path,QString name);

	static QPixmap pixmap(QString name,int size);
	static QPixmap pixmap(int id,int size);

	static QPixmap lockPixmap(int);
	static QPixmap warningPixmap(int);
	static QPixmap errorPixmap(int);
	static QPixmap infoPixmap(int);

private:
	static IconItem* icon(QString name);
	static IconItem* icon(int id);

	static std::map<QString,IconItem*> icons_;
	static std::map<int,IconItem*> iconsById_;
};

#endif
