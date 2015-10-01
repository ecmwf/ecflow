//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CustomTabWidget.hpp"

#include <QVariant>


CustomTabWidget::CustomTabWidget(QWidget* parent) : QTabWidget(parent)
{
	setProperty("change","1");
}

void CustomTabWidget::setCustomIcon(int index, QPixmap pix)
{
	if (index >= 0 && index < count())
	{
		QSize maxSize=maxIconSize();

		if(maxSize.width() < pix.width())
			maxSize.setWidth(pix.width());

		if(maxSize.height() < pix.height())
			maxSize.setHeight(pix.height());

		if(maxSize != iconSize())
			setIconSize(maxSize);

		setTabIcon(index, QIcon(pix));
	}
}

QSize CustomTabWidget::maxIconSize() const
{
	QSize maxSize(0,0);
	for(int i=0; i < count(); i++)
	{
		if(tabIcon(i).availableSizes().count() > 0)
		{
			QSize avs=tabIcon(i).availableSizes().front();
			if(maxSize.width() < avs.width())
				maxSize.setWidth(avs.width());

			if(maxSize.height() < avs.height())
				maxSize.setHeight(avs.height());
		}
	}
	return maxSize;
}

