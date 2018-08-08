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
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "VConfig.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"
#include "WidgetNameProvider.hpp"
#include "WmWorkspaceHandler.hpp"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVariant>

Q_DECLARE_METATYPE(QList<int>)

//===========================================================
//
// ChangeNotifyDialogButton
//
//===========================================================

ChangeNotifyDialogButton::ChangeNotifyDialogButton(QWidget* parent) :
    QToolButton(parent),
    notifier_(nullptr)
{
    setProperty("notify","1");
    setAutoRaise(true);
    setIconSize(QSize(20,20));
    setCheckable(true);
}

void ChangeNotifyDialogButton::setNotifier(ChangeNotify* notifier)
{
    notifier_=notifier;

    setText(notifier_->widgetText());
    setToolTip(notifier_->toolTip());

    connect(notifier_->data(),SIGNAL(endAppendRow()),
            this,SLOT(slotAppend()));

    connect(notifier_->data(),SIGNAL(endRemoveRow(int)),
                    this,SLOT(slotRemoveRow(int)));

    connect(notifier_->data(),SIGNAL(endReset()),
                this,SLOT(slotReset()));

    updateSettings();
}

void ChangeNotifyDialogButton::slotAppend()
{
    updateSettings();
}

void ChangeNotifyDialogButton::slotRemoveRow(int)
{
    updateSettings();
}

void ChangeNotifyDialogButton::slotReset()
{
    updateSettings();
}

void ChangeNotifyDialogButton::updateSettings()
{
    setEnabled(notifier_->isEnabled());

#if 0

    QString text;
    QString numText;

    if(notifier_->prop())
    {
        text=notifier_->prop()->param("widgetText");
    }

    int num=0;
    if(notifier_->data())
    {
        num=notifier_->data()->size();
        if(num > 0 && num < 10)
            numText=QString::number(num);
        else if(num > 10)
            numText="9+";

    }
#endif
}

//===========================================================
//
// ChangeNotifyDialogWidget
//
//===========================================================

ChangeNotifyDialogWidget::ChangeNotifyDialogWidget(QWidget *parent) :
    QWidget(parent),
	notifier_(nullptr)
{
	setupUi(this);
}

void ChangeNotifyDialogWidget::init(ChangeNotify* notifier)
{
	notifier_=notifier;

    tree_->setModel(notifier_->model());  
    label_->setText(notifier_->widgetText());


#if 0
	connect(notifier->data(),SIGNAL(endAppendRow()),
			this,SLOT(slotAppend()));

	connect(notifier->data(),SIGNAL(endRemoveRow(int)),
			this,SLOT(slotRemoveRow(int)));

	connect(notifier->data(),SIGNAL(endReset()),
			this,SLOT(slotReset()));
#endif

    //Selection
    connect(tree_,SIGNAL(clicked(const QModelIndex&)),
            this,SLOT(slotSelectItem(const QModelIndex&)));

    connect(tree_,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex&)));

    updateSettings();
}

#if 0
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
    Q_EMIT contentsChanged();
}
#endif

void ChangeNotifyDialogWidget::updateSettings()
{
    Q_ASSERT(notifier_);
    QColor bgCol=notifier_->fillColour();
    QColor textCol=notifier_->textColour();
    QColor bgLight=bgCol.lighter(105);

	QString st="QLabel { \
					background: qlineargradient(x1 :0, y1: 0, x2: 0, y2: 1, \
                         stop: 0 " + bgLight.name() + ", stop: 1 " + bgCol.name() + "); color: " +
        textCol.name() + "; padding: 4px; border: 1px solid rgb(170,170,170);}";

    UiLog().dbg() << bgCol << " " << textCol;
    UiLog().dbg() << st;

	label_->setStyleSheet(st);
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
    ignoreCurrentChange_(false),
    switchWsProp_(nullptr)
{
    setupUi(this);

    buttonHb_= new QHBoxLayout(buttonW_);
    buttonHb_->setContentsMargins(0,0,0,0);
    buttonHb_->setSpacing(2);

    buttonGroup_=new QButtonGroup(this);
    connect(buttonGroup_,SIGNAL(buttonToggled(int,bool)),
            this,SLOT(slotButtonToggled(int,bool)));

	clearOnCloseCb_->setChecked(true);

    connect(optionsTb_,SIGNAL(clicked()),
            this,SLOT(slotOptions()));

    switchWsProp_=VConfig::instance()->find("notification.settings.switch_desktop");

    readSettings();

    WidgetNameProvider::nameChildren(this);   
}

ChangeNotifyDialog::~ChangeNotifyDialog()
{
	writeSettings();
}

void ChangeNotifyDialog::add(ChangeNotify* notifier)
{
    auto* w=new ChangeNotifyDialogWidget(this);
    w->init(notifier);

    connect(w,SIGNAL(selectionChanged(VInfo_ptr)),
            this,SLOT(slotSelectionChanged(VInfo_ptr)));

    ignoreCurrentChange_=true;
    stacked_->addWidget(w);
    ignoreCurrentChange_=false;
    ntfWidgets_ << w;



    auto *bw=new ChangeNotifyDialogButton(this);
    bw->setNotifier(notifier);
    buttonHb_->addWidget(bw);
    int buttonId=buttonGroup_->buttons().count();
    buttonGroup_->addButton(bw,buttonId);
    ntfButtons_ << bw;

    UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
             "stacked_->count()=" << stacked_->count() <<
             " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());

    readNtfWidgetSettings(stacked_->count()-1);
}

void ChangeNotifyDialog::slotButtonToggled(int,bool)
{
    int idx=buttonGroup_->checkedId();
    if(idx != -1)
    {
        stacked_->setCurrentIndex(idx);
    }
}

void ChangeNotifyDialog::slotSelectionChanged(VInfo_ptr info)
{
    //Moves the dialogue to the virtual workspace of the first
    //mainwindow and then switches the workspace
    if(switchWsProp_ && switchWsProp_->value().toBool())
    {
        if(WmWorkspaceHandler::switchTo(this,MainWindow::firstWindow()))
            raise();
    }

    MainWindow::lookUpInTree(info);
}

void ChangeNotifyDialog::slotOptions()
{
    QString op="notification";
    if(ChangeNotify* notifier=indexToNtf(stacked_->currentIndex()))
    {
        op+="." + QString::fromStdString(notifier->id());
    }
    MainWindow::startPreferences(op);
}

void ChangeNotifyDialog::setCurrent(ChangeNotify *ntf)
{
    int idx=ntfToIndex(ntf);
    if(idx != -1)
	{
        UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
                 "stacked_->count()=" << stacked_->count() <<
                 " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());
        UI_ASSERT(idx < stacked_->count(),"idx=" << idx << " stacked_->count()=" << stacked_->count());
        UI_ASSERT(idx >=0,"idx=" << idx);

        buttonGroup_->button(idx)->setChecked(true);
	}
}

void ChangeNotifyDialog::setEnabled(ChangeNotify* ntf,bool b)
{
    int idx=ntfToIndex(ntf);
    if(idx != -1)
	{
        UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
                 "stacked_->count()=" << stacked_->count() <<
                 " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());
        UI_ASSERT(idx < stacked_->count(),"idx=" << idx << " stacked_->count()=" << stacked_->count());
        UI_ASSERT(idx >=0,"idx=" << idx);

        buttonGroup_->button(idx)->setEnabled(b);
        stacked_->widget(idx)->setEnabled(b);
	}
}

void ChangeNotifyDialog::clearCurrentData()
{
    UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
             "stacked_->count()=" << stacked_->count() <<
             " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());

    int idx=buttonGroup_->checkedId();
    if(idx != -1)
    {
        UI_ASSERT(idx < stacked_->count(),"idx=" << idx << " stacked_->count()=" << stacked_->count());
        UI_ASSERT(idx >=0,"idx=" << idx);
        if(ChangeNotify *ntf=indexToNtf(idx))
            ntf->clearData();
    }
}

void ChangeNotifyDialog::on_closePb__clicked(bool)
{
	hide();

	if(clearOnCloseCb_->isChecked())
	{
        clearCurrentData();
    }

	writeSettings();
}

void ChangeNotifyDialog::on_clearPb__clicked(bool)
{
    clearCurrentData();
}

ChangeNotify* ChangeNotifyDialog::indexToNtf(int idx)
{
    UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
             "stacked_->count()=" << stacked_->count() <<
             " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());

    if(idx >=0 && idx < stacked_->count())
    {
        UI_ASSERT(idx < stacked_->count(),"idx=" << idx << " stacked_->count()=" << stacked_->count());
        UI_ASSERT(idx >=0,"idx=" << idx);
        return ntfWidgets_[idx]->notifier();
    }

    return nullptr;
}

int ChangeNotifyDialog::ntfToIndex(ChangeNotify* ntf)
{
    UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
             "stacked_->count()=" << stacked_->count() <<
             " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());

    for(int i=0; i < stacked_->count(); i++)
    {
        if(ntfWidgets_[i]->notifier() == ntf)
            return i;
    }

    return -1;
}

void ChangeNotifyDialog::updateSettings(ChangeNotify* notifier)
{
    UI_ASSERT(stacked_->count() == buttonGroup_->buttons().count(),
             "stacked_->count()=" << stacked_->count() <<
             " buttonGroup_->buttons().count()=" << buttonGroup_->buttons().count());

    int idx=ntfToIndex(notifier);
	if(idx != -1)
	{
        UI_ASSERT(idx < stacked_->count(),"idx=" << idx << " stacked_->count()=" << stacked_->count());
        UI_ASSERT(idx >=0,"idx=" << idx);
        //if(stacked_->widget(idx)->isEnabled())
        //{
        ntfWidgets_[idx]->updateSettings();
        ntfButtons_[idx]->updateSettings();
        //}
	}
}

void ChangeNotifyDialog::closeEvent(QCloseEvent* e)
{
    if(clearOnCloseCb_->isChecked())
    {
        clearCurrentData();
    }
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

    for(int i=0; i < stacked_->count(); i++)
    {
        settings.beginGroup("tab_" + QString::number(i));
        ntfWidgets_[i]->writeSettings(settings);
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

void ChangeNotifyDialog::readNtfWidgetSettings(int idx)
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ChangeNotifyDialog")),
                   QSettings::NativeFormat);

    settings.beginGroup("tab_" + QString::number(idx));
    UI_ASSERT(stacked_->count() > idx,"stacked_->count()=" << stacked_->count() << " idx=" << idx);
    ntfWidgets_[idx]->readSettings(settings);
    settings.endGroup();
}
