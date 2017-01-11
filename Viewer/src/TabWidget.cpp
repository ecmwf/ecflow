//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TabWidget.hpp"

#include <QDebug>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QStackedWidget>
#include <QStyleOptionTabV2>
#include <QStylePainter>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "UserMessage.hpp"

void IconTabBar::paintEvent(QPaintEvent *e)
{
    QStylePainter painter(this);
    for(int i = 0; i < count(); ++i)
    {
        QStyleOptionTabV2 option;
        initStyleOption(&option, i);
        qDebug() << i << option.iconSize;
        painter.drawItemPixmap(option.rect, Qt::AlignTop|Qt::AlignHCenter, option.icon.pixmap(option.iconSize));
        //painter.drawItemText(option.rect, Qt::AlignBottom|Qt::AlignHCenter, palette(), 1, option.text);

    }
}


TabWidget::TabWidget(QWidget* parent) :
		QWidget(parent),
		beingCleared_(false)
{
	//Main layout
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	//Horizontal layout for the tab bar
	QHBoxLayout* hb = new QHBoxLayout(this);
	hb->setSpacing(0);
	hb->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(hb);

	//Tab bar
    bar_ = new QTabBar(this);
    bar_->setObjectName("bar");
	hb->addWidget(bar_, 1);

    bar_->setProperty("nodePanel","1");
	bar_->setMovable(true);
    //bar_->setExpanding(true);

	//QString st=bar_->styleSheet();
	//st+="QTabBar::tab{padding: 4px;}";
	//st+="QTabBar::tab {margin-left: 4px;}";
	//st+="QTabBar::tab:selected {font: bold;}";
	//bar_->setStyleSheet(st);

	//Add tab button on the right
	addTb_ = new QToolButton(this);
    addTb_->setObjectName("addTb");
	addTb_->setAutoRaise(true);
	addTb_->setIcon(QPixmap(":/viewer/add_tab.svg"));
	addTb_->setToolTip(tr("Open a new tab"));
	hb->addWidget(addTb_);

    //Tab list menu
    tabListTb_=new QToolButton(this);
    tabListTb_->setObjectName("tabListTb");
    tabListTb_->setAutoRaise(true);
    tabListTb_->setIcon(QPixmap(":/viewer/menu_arrow_down.svg"));
    tabListTb_->setToolTip(tr("List all tabs"));
    hb->addWidget(tabListTb_);

    //Stacked widget to store the actual tab widgets
	stacked_ = new QStackedWidget(this);
    stacked_->setObjectName("stacked");
	stacked_->setMinimumHeight(1);
	stacked_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	layout->addWidget(stacked_);

	//Context menu for tha tabs
	bar_->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(bar_,SIGNAL(customContextMenuRequested(const QPoint &)),
			this,SLOT(slotContextMenu(const QPoint &)));

	connect(bar_,SIGNAL(tabMoved(int,int)),
			this,SLOT(tabMoved(int,int)));

	connect(bar_,SIGNAL(currentChanged(int)),
			this,SLOT(currentTabChanged(int)));

	connect(bar_,SIGNAL(tabCloseRequested(int)),
			this,SLOT(removeTab(int)));

	connect(addTb_,SIGNAL(clicked()),
			this,SIGNAL(newTabRequested()));

    connect(tabListTb_,SIGNAL(clicked()),
            this,SLOT(slotTabList()));
}

void TabWidget::slotContextMenu(const QPoint& pos) {

    if (pos.isNull())
		return;

    int index = bar_->tabAt(pos);
    if(index < 0 || index > bar_->count())
        return;

    QList<QAction*> lst;
    QAction *closeAc=new QAction(QPixmap(":/viewer/close.svg"),"&Close tab",this);
    lst << closeAc;

    if(QAction *ac=QMenu::exec(lst,mapToGlobal(pos),closeAc,this))
    {
        if(ac == closeAc)
            removeTab(index);
    }

    qDeleteAll(lst);


	/*MvQContextItemSet *cms = cmSet();
	if (!cms)
		return;

	int index = bar_->tabAt(pos);

	QString selection = MvQContextMenu::instance()->exec(cms->icon(),
			mapToGlobal(pos), this,
			//QString::number(bar_->count()));
			"path=" + folderPath(index));
	if (!selection.isEmpty())
		tabBarCommand(selection, index);*/
}

int TabWidget::count() const
{
	return bar_->count();
}

int TabWidget::currentIndex() const
{
	return bar_->currentIndex();
}

void TabWidget::setCurrentIndex(int index)
{
	bar_->setCurrentIndex(index);
}

QWidget* TabWidget::widget(int index) const
{
	if (index >= 0 && index < bar_->count())
	{
		return stacked_->widget(index);
	}

	return 0;
}

QWidget* TabWidget::currentWidget() const
{
	return widget(bar_->currentIndex());
}

int TabWidget::indexOfWidget(QWidget *w) const
{
	for (int i = 0; i < stacked_->count(); i++)
		if (w == stacked_->widget(i))
			return i;

	return -1;
}

void TabWidget::clear()
{
	beingCleared_=true;
	while (bar_->count() > 0) {
		removeTab(0);
	}
	beingCleared_=false;
}

void TabWidget::addTab(QWidget *w, QPixmap pix, QString name)
{
	stacked_->addWidget(w);
	bar_->addTab(pix, name);
	bar_->setCurrentIndex(count() - 1);
	checkTabStatus();
}

void TabWidget::removeTab(int index)
{
	if (index >= 0 && index < bar_->count()) {
		QWidget *w = stacked_->widget(index);
		stacked_->removeWidget(w);
		bar_->removeTab(index);
		w->hide();
		w->deleteLater();

        Q_EMIT tabRemoved();
	}

	checkTabStatus();
}

void TabWidget::removeOtherTabs(int index)
{
	QWidget *actW = stacked_->widget(index);

	while (bar_->count() > 0) {
		if (stacked_->widget(0) != actW) {
			removeTab(0);
		} else
			break;
	}

	while (bar_->count() > 1) {
		if (stacked_->widget(1) != actW) {
			removeTab(1);
		}
	}

	checkTabStatus();
}

void TabWidget::currentTabChanged(int index)
{
	if (stacked_->count() == bar_->count()) {
		stacked_->setCurrentIndex(index);
		Q_EMIT currentIndexChanged(index);

		checkTabStatus();
	}
}

void TabWidget::tabMoved(int from, int to)
{
	QWidget *w = stacked_->widget(from);
	stacked_->removeWidget(w);
	stacked_->insertWidget(to, w);

	//bar_->setCurrentIndex(to);
	currentTabChanged(to);
}

void TabWidget::setTabText(int index, QString txt)
{
	if (index >= 0 && index < bar_->count()) {
		bar_->setTabText(index, txt);
	}
}

void TabWidget::setTabToolTip(int index, QString txt)
{
    if (index >= 0 && index < bar_->count()) {
        bar_->setTabToolTip(index, txt);
    }
}

void TabWidget::setTabWht(int index, QString txt)
{
    if (index >= 0 && index < bar_->count()) {
        bar_->setTabWhatsThis(index, txt);
    }
}

void TabWidget::setTabData(int index, QPixmap pix)
{
    if (index >= 0 && index < bar_->count()) {
        bar_->setTabData(index,QIcon(pix));
    }
}

void TabWidget::setTabIcon(int index, QPixmap pix)
{
	if (index >= 0 && index < bar_->count())
	{
        QLabel *lab=static_cast<QLabel*>(bar_->tabButton(index,QTabBar::RightSide));
        if(!lab)
        {
            lab=new QLabel();
            lab->setAlignment(Qt::AlignCenter);
        }
        else
        {
            bar_->setTabButton(index,QTabBar::RightSide,0);
        }
        lab->setPixmap(pix);
        lab->setFixedSize(pix.size());
        bar_->setTabButton(index,QTabBar::RightSide,lab);

#if 0
        QSize maxSize=maxIconSize();

        if(maxSize.width() < pix.width())
			maxSize.setWidth(pix.width());

		if(maxSize.height() < pix.height())
			maxSize.setHeight(pix.height());

		if(maxSize != bar_->iconSize())
			bar_->setIconSize(maxSize);

		bar_->setTabIcon(index, QIcon(pix));
#endif

	}
}

#if 0
QSize TabWidget::maxIconSize() const
{
	QSize maxSize(0,0);
	for(int i=0; i < bar_->count(); i++)
	{
		if(bar_->tabIcon(i).availableSizes().count() > 0)
		{
			QSize avs=bar_->tabIcon(i).availableSizes().front();
			if(maxSize.width() < avs.width())
				maxSize.setWidth(avs.width());

			if(maxSize.height() < avs.height())
				maxSize.setHeight(avs.height());
		}
	}
	return maxSize;
}
#endif

void TabWidget::checkTabStatus()
{
	if (bar_->count() > 1)
	{
		bar_->show();
        //bar_->setTabsClosable(true);
		addTb_->show();
        tabListTb_->show();
	}
	else
	{
		bar_->hide();
        //bar_->setTabsClosable(false);
        addTb_->hide();
        tabListTb_->hide();
	}

    /*
	for (int i = 0; i < bar_->count(); i++)
	{
		if (QWidget *w = bar_->tabButton(i, QTabBar::RightSide))
		{
			if (i == bar_->currentIndex())
				w->show();
			else
				w->hide();
		}
    }*/
}

void TabWidget::slotTabList()
{
    QMenu* menu=new QMenu(tabListTb_);

    for(int i=0; i < bar_->count(); i++)
    {
        QAction *ac=new QAction(menu);
        ac->setText(bar_->tabWhatsThis(i));
        ac->setIcon(bar_->tabData(i).value<QIcon>());
        ac->setData(i);
        if(i==bar_->currentIndex())
        {
            QFont font;
            font.setBold(true);
            ac->setFont(font);
        }

        menu->addAction(ac);
    }

    if(QAction *ac=menu->exec(QCursor::pos()))
    {
        int index=ac->data().toInt();
        if(index >=0 && index < count())
        {
            setCurrentIndex(index);
        }
    }

    menu->clear();
    menu->deleteLater();
}
