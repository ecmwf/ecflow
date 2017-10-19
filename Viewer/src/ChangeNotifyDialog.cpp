//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyDialog.hpp"

#include "ChangeNotify.hpp"
#include "ChangeNotifyModel.hpp"
#include "MainWindow.hpp"
#include "SessionHandler.hpp"
#include "TreeView.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"
#include "WidgetNameProvider.hpp"

#include <QCloseEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVariant>

Q_DECLARE_METATYPE(QList<int>)

ChangeNotifyDialogWidget::ChangeNotifyDialogWidget(QWidget *parent) :
    QWidget(parent),
	notifier_(0)
{
	setupUi(this);
}

void ChangeNotifyDialogWidget::init(ChangeNotify* notifier)
{
	notifier_=notifier;

    tree_->setModel(notifier_->model());

	label_->hide();

	connect(notifier->data(),SIGNAL(endAppendRow()),
			this,SLOT(slotAppend()));

	connect(notifier->data(),SIGNAL(endRemoveRow(int)),
			this,SLOT(slotRemoveRow(int)));

	connect(notifier->data(),SIGNAL(endReset()),
			this,SLOT(slotReset()));

    //Selection
    connect(tree_,SIGNAL(clicked(const QModelIndex&)),
            this,SLOT(slotSelectItem(const QModelIndex&)));

    connect(tree_,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex&)));


	/*QString txt=notifier->prop()->param("description");
	label_->setText(txt);

	update(notifier);*/
}

void ChangeNotifyDialogWidget::slotAppend()
{
	Q_EMIT contentsChanged();
}

void ChangeNotifyDialogWidget::slotRemoveRow(int)
{
	Q_EMIT contentsChanged();
}

void ChangeNotifyDialogWidget::slotReset()
{
	Q_EMIT  contentsChanged();
}

void ChangeNotifyDialogWidget::update(ChangeNotify* notifier)
{

#if 0
    QColor bgCol(Qt::gray);
	if(VProperty *p=notifier->prop()->findChild("fill_colour"))
		bgCol=p->value().value<QColor>();

	QColor bgLight=bgCol.lighter(150);

	QString st="QLabel { \
					background: qlineargradient(x1 :0, y1: 0, x2: 0, y2: 1, \
					     stop: 0 " + bgLight.name() + ", stop: 1 " + bgLight.name() + "); }";

	label_->setStyleSheet(st);
#endif
}

void ChangeNotifyDialogWidget::slotSelectItem(const QModelIndex& idx)
{
    //QModelIndexList lst=tree_->selectedIndexes();
    //if(lst.count() > 0)
    //{
        VInfo_ptr info=notifier_->model()->nodeInfo(idx);
        if(info)
        {
            Q_EMIT selectionChanged(info);
        }
    //}
}

void ChangeNotifyDialogWidget::slotDoubleClickItem(const QModelIndex&)
{

}

void ChangeNotifyDialogWidget::writeSettings(QSettings& settings)
{
    QList<int> wVec;
    for(int i=0; i < tree_->model()->columnCount(QModelIndex()); i++)
    {
        wVec.push_back(tree_->columnWidth(i));
    }
    settings.setValue("colWidth",QVariant::fromValue(wVec));
}

void ChangeNotifyDialogWidget::readSettings(const QSettings& settings)
{
    QList<int> wVec;
    if(settings.contains("colWidth"))
    {
       wVec=settings.value("colWidth").value<QList<int> >();
       if(wVec.count() >0 && wVec[0] < 1)
           wVec.clear();
    }

    if(!wVec.isEmpty())
    {
        for(int i=0; i < tree_->model()->columnCount(QModelIndex()); i++)
        {
            if(wVec.count() > i && wVec[i] > 0)
                tree_->setColumnWidth(i,wVec[i]);
        }
    }
    else
    {
        if(tree_->model()->columnCount(QModelIndex()) > 1)
        {
            QFont f;
            QFontMetrics fm(f);
            tree_->setColumnWidth(0,fm.width("serverserverserver"));
            tree_->setColumnWidth(1,fm.width("/suite1/family1/family2/family3/family4/task"));
        }
    }
}

//===========================================================
//
// ChangeNotifyDialog
//
//===========================================================

ChangeNotifyDialog::ChangeNotifyDialog(QWidget *parent) :
	QDialog(parent),
	ignoreCurrentChange_(false)
{
	setupUi(this);

	tab_->setProperty("notify","1");

#ifdef ECFLOW_QT5
	tab_->tabBar()->setExpanding(false);
#endif

	clearOnCloseCb_->setChecked(true);

	grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
	grad_.setStart(0,0);
	grad_.setFinalStop(0,1);


    QToolButton* optionsTb=new QToolButton(this);
    //optionsTb->setAutoRaise(true);
    optionsTb->setText(tr("&Prefrences"));
    optionsTb->setIcon(QPixmap(":/viewer/configure.svg"));
    optionsTb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    optionsTb->setToolTip(tr("Configure notification options"));

    connect(optionsTb,SIGNAL(clicked()),
            this,SLOT(slotOptions()));

    tab_->setCornerWidget(optionsTb);

	readSettings();

    WidgetNameProvider::nameChildren(this);

}

ChangeNotifyDialog::~ChangeNotifyDialog()
{
	writeSettings();
}

void ChangeNotifyDialog::addTab(ChangeNotify* notifier)
{
	ChangeNotifyDialogWidget* w=new ChangeNotifyDialogWidget(this);
	w->init(notifier);

	connect(w,SIGNAL(contentsChanged()),
			this,SLOT(slotContentsChanged()));

    connect(w,SIGNAL(selectionChanged(VInfo_ptr)),
            this,SLOT(slotSelectionChanged(VInfo_ptr)));

    VProperty* prop=notifier->prop();
    Q_ASSERT(prop);

	ignoreCurrentChange_=true;
    tab_->addTab(w,prop->param("labelText"));
	ignoreCurrentChange_=false;

	tabWidgets_ << w;

	int idx=tab_->count()-1;
#if 0
    if(idx ==  tab_->currentIndex())
		updateStyleSheet(notifier->prop());
#endif

	decorateTab(idx,notifier);

    readTabSettings(tab_->count()-1);
}

void ChangeNotifyDialog::slotContentsChanged()
{
	if(ChangeNotifyDialogWidget* w=static_cast<ChangeNotifyDialogWidget*>(sender()))
	{
		int idx=tab_->indexOf(w);
		if(idx != -1)
		{
			decorateTab(idx,w->notifier());
		}
	}
}

void ChangeNotifyDialog::slotSelectionChanged(VInfo_ptr info)
{
    MainWindow::lookUpInTree(info);
}

void ChangeNotifyDialog::slotOptions()
{
    QString op="notification";
    if(ChangeNotify* notifier=tabToNtf(tab_->currentIndex()))
    {
        op+="." + QString::fromStdString(notifier->id());
    }
    MainWindow::startPreferences(op);
}

void ChangeNotifyDialog::updateStyleSheet(VProperty *currentProp)
{
#if 0
    QColor bgCol(Qt::gray);
	if(VProperty *p=currentProp->findChild("fill_colour"))
		bgCol=p->value().value<QColor>();

	QColor bgLight=bgCol.lighter(150);

	QString st="QTabBar::tab:selected { \
				background: qlineargradient(x1 :0, y1: 0, x2: 0, y2: 1, \
						stop: 0 " + bgLight.name() + ", stop: 1 " + bgCol.name() + "); }";

	tab_->setStyleSheet(st);
#endif
}

void ChangeNotifyDialog::decorateTabs()
{
    for(int i=0; i < tab_->count(); i++)
	{
		decorateTab(i,tabWidgets_.at(i)->notifier());
	}    
}

void ChangeNotifyDialog::decorateTab(int tabIdx,ChangeNotify* notifier)
{
    if(tabIdx == -1 || !notifier)
        return;

    VProperty *prop=notifier->prop();
    QString numText;
    if(notifier->data())
    {
        int num=notifier->data()->size();
        if(num > 0)
            numText=" (" + QString::number(num) + ")";
    }
    QString labelText=prop->param("labelText")+ numText;

    tab_->setTabText(tabIdx,labelText);


#if 0
    if(tabIdx == -1 || !notifier)
		return;

	VProperty *prop=notifier->prop();

	QString numText;
	if(notifier->data())
	{
		int num=notifier->data()->size();
		if(num > 0)
			numText=" (" + QString::number(num) + ")";
	}


	//Create icon for tab
	QFont f;
	QFontMetrics fm(f);
	QString labelText=prop->param("labelText")+ numText;
	int textH=fm.height();
	int textW=fm.width(labelText);
	int margin=3;

	QColor bgCol(Qt::gray);
	if(VProperty *p=prop->findChild("fill_colour"))
		bgCol=p->value().value<QColor>();

	QColor fgCol(Qt::black);
	if(VProperty *p=prop->findChild("text_colour"))
		fgCol=p->value().value<QColor>();

	QColor countBgCol(58,126,194);
	if(VProperty *p=prop->findChild("count_fill_colour"))
		countBgCol=p->value().value<QColor>();

	QColor countFgCol(Qt::white);
	if(VProperty *p=prop->findChild("count_text_colour"))
		countFgCol=p->value().value<QColor>();

	QColor bgLight=bgCol.lighter(150);
	grad_.setColorAt(0,bgLight);
	grad_.setColorAt(1,bgCol);
	QBrush bgBrush(grad_);

	QFont numF;
	numF.setBold(true);
	numF.setPointSize(f.pointSize()-1);
	QFontMetrics numFm(numF);

	int h=2*margin+textH+4;
    int w=2*margin+textW;

	QPixmap pix(w,h);
	pix.fill(Qt::transparent);

	QPainter painter(&pix);

	/*	QFont f;
		f.setBold(true);
		f.setPointSize(f.pointSize()+1);
		QFontMetrics fm(f);
		int w;
		if(!numText.isEmpty())
			w=fm.width(text) + 6 + fm.width(numText) + 2;
		else
			w=fm.width(text) + 6;

		int h=fm.height()+6;

		QPixmap pix(w,h);
		pix.fill(QColor(255,255,255,0));
		QPainter painter(&pix);
		painter.setRenderHint(QPainter::Antialiasing,true);
		painter.setRenderHint(QPainter::TextAntialiasing,true);

		QRect textRect(0,0,fm.width(text)+6,h);

		QColor bgLight=bgCol.lighter(150);
		grad_.setColorAt(0,bgLight);
		grad_.setColorAt(1,bgCol);

		painter.setBrush(QBrush(grad_));
		painter.setPen(border);
		painter.drawRoundedRect(textRect,2,2);
		painter.setPen(fgCol);
		painter.setFont(f);
		painter.drawText(textRect,Qt::AlignHCenter|Qt::AlignVCenter,text);

		if(!numText.isEmpty())
		{
			QRect numRect(textRect.right()-1,0,fm.width(numText)+4,fm.ascent()+4);
			painter.setBrush(countBgCol);
			painter.setPen(countFgCol);
			painter.drawRoundedRect(numRect,4,4);
			painter.setFont(f);
			painter.drawText(numRect,Qt::AlignHCenter|Qt::AlignVCenter,numText);
		}

		setIconSize(QSize(w,h));
		setIcon(pix);
	*/

	pix.fill(Qt::transparent);

	QRect textRect=QRect(margin,0,textW,pix.height());
	painter.setPen(fgCol);
	painter.drawText(textRect,Qt::AlignVCenter|Qt::AlignHCenter,labelText);


    if(tabIdx != tab_->currentIndex())
	{
		QRect lineRect(textRect.left(),pix.height()/2+textH/2+1,
					   textRect.width(),3);

		painter.fillRect(lineRect,bgCol);
	}

	/*if(!numText.isEmpty())
	{
		painter.setRenderHint(QPainter::Antialiasing,true);
		painter.setRenderHint(QPainter::TextAntialiasing,true);

		QRect numRect(textRect.right()+4,1,fm.width(numText)+4,fm.ascent()+4);
		painter.setBrush(countBgCol);
		painter.setPen(Qt::NoPen);
		painter.drawRoundedRect(numRect,4,4);
		painter.setFont(numF);
		painter.setPen(countFgCol);
		painter.drawText(numRect,Qt::AlignHCenter|Qt::AlignVCenter,numText);
	}*/

    tab_->setCustomIcon(tabIdx,pix);
#endif
}


void ChangeNotifyDialog::setCurrentTab(ChangeNotify *ntf)
{
	int tabIdx=ntfToTab(ntf);
	if(tabIdx != -1)
	{
		tab_->setCurrentIndex(tabIdx);
	}
}

void ChangeNotifyDialog::setEnabledTab(ChangeNotify* ntf,bool b)
{
	int tabIdx=ntfToTab(ntf);
	if(tabIdx != -1)
	{
		tab_->setTabEnabled(tabIdx,b);
	}
}

void ChangeNotifyDialog::on_tab__currentChanged(int idx)
{
	if(ignoreCurrentChange_)
		return;

	if(ChangeNotify* notifier=tabToNtf(idx))
	{
		updateStyleSheet(notifier->prop());
		decorateTabs();
	}
}

void ChangeNotifyDialog::on_closePb__clicked(bool b)
{
	hide();

	if(clearOnCloseCb_->isChecked())
	{
		int idx=tab_->currentIndex();
		if(idx != -1)
		{
			if(ChangeNotify *ntf=tabToNtf(idx))
				ntf->clearData();
		}
	}

	writeSettings();
}

void ChangeNotifyDialog::on_clearPb__clicked(bool b)
{
	int idx=tab_->currentIndex();
	if(idx != -1)
	{
		if(ChangeNotify *ntf=tabToNtf(idx))
			ntf->clearData();
	}
}

ChangeNotify* ChangeNotifyDialog::tabToNtf(int idx)
{
	if(idx >=0 && idx < tab_->count())
	{
		return tabWidgets_.at(idx)->notifier();
	}

	return 0;
}

int ChangeNotifyDialog::ntfToTab(ChangeNotify* ntf)
{
	for(int i=0; i < tab_->count(); i++)
	{
		if(tabWidgets_.at(i)->notifier() == ntf)
			return i;
	}

	return -1;
}

void ChangeNotifyDialog::updateSettings(ChangeNotify* notifier)
{
	int idx=ntfToTab(notifier);
	if(idx != -1)
	{
		if(tab_->isTabEnabled(idx))
		{
			if(idx == tab_->currentIndex())
			{
				updateStyleSheet(notifier->prop());
			}
			decorateTab(idx,notifier);
		}
	}
}

void ChangeNotifyDialog::closeEvent(QCloseEvent* e)
{
	writeSettings();
	e->accept();
}

void ChangeNotifyDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ChangeNotifyDialog")),
                       QSettings::NativeFormat);

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.setValue("clearOnClose",clearOnCloseCb_->isChecked());
    settings.endGroup();

    for(int i=0; i < tab_->count(); i++)
    {
        settings.beginGroup("tab_" + QString::number(i));
        tabWidgets_[i]->writeSettings(settings);
        settings.endGroup();
    }
}

void ChangeNotifyDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ChangeNotifyDialog")),
                       QSettings::NativeFormat);

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
        resize(QSize(540,460));
	}

	if(settings.contains("clearOnClose"))
	{
		clearOnCloseCb_->setChecked(settings.value("clearOnClose").toBool());
	}

	settings.endGroup();

    //The tab settings are read when the actual tabs are created later.
}

void ChangeNotifyDialog::readTabSettings(int tabIndex)
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ChangeNotifyDialog")),
                   QSettings::NativeFormat);

    settings.beginGroup("tab_" + QString::number(tabIndex));
    Q_ASSERT(tab_->count() > tabIndex);
    tabWidgets_[tabIndex]->readSettings(settings);
    settings.endGroup();
}
