//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableItemWidget.hpp"

#include <QClipboard>
#include <QDebug>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "IconProvider.hpp"
#include "LineEdit.hpp"
#include "UserMessage.hpp"
#include "VariableModel.hpp"
#include "VariableModelData.hpp"
#include "VariableSearchLine.hpp"

#define _UI_VARIABLEITEMWIDGET_DEBUG

//======================================
//
// ServerDialogChecked
//
//======================================

bool VariableDialogChecker::checkName(QString name)
{
	if(name.simplified().isEmpty())
	{
		error(QObject::tr("<b>Name</b> cannot be empty!"));
		return false;
	}
	else if(name.contains(","))
	{
		error(QObject::tr("<b>Name</b> cannot contain comma character!"));
		return false;
	}

	/*if(ServerList::instance()->find(name.toStdString()))
	{
			error(QObject::tr("The specified server already exists! Please select a different name!"));
			return false;
	}*/

	return true;
}

bool VariableDialogChecker::checkValue(QString host)
{
	if(host.simplified().isEmpty())
	{
		error(QObject::tr("<b>Host</b> cannot be empty!"));
		return false;
	}
	else if(host.contains(","))
	{
		error(QObject::tr("<b>Host</b> cannot contain comma character!"));
		return false;
	}

	return true;
}


void VariableDialogChecker::error(QString msg)
{
	QMessageBox::critical(0,QObject::tr("Server item"),errorText_ + "<br>"+ msg);
}

//======================================
//
// VariablePropDialog
//
//======================================

VariablePropDialog::VariablePropDialog(VariableModelData *data,QString name, QString value,bool genVar,bool frozen,QWidget *parent) :
   QDialog(parent),
   genVar_(genVar),
   data_(data)
{
	setupUi(this);

    typeIconLabel_->hide();
    valueEdit_->setProperty("form","1");

    if(data_->type() ==  "server")
    {
         parentLabel_->setText(QString::fromStdString(data_->name()) +" (type: server)");
    }
    else
    {
        parentLabel_->setText(QString::fromStdString(data_->fullPath()) + "(type: " +
                QString::fromStdString(data_->type()) + ")");
    }

	QString typeTxt=genVar?tr("<b>generated variable</b>"):tr("<b>user variable</b>");
	bool readOnly=data_->isReadOnly(name.toStdString());
	if(readOnly)
	{
		typeTxt+=" (read only)";
        int id=IconProvider::add(":/viewer/padlock.svg","padlock");
        typeIconLabel_->show();
        typeIconLabel_->setPixmap(IconProvider::pixmap(id,12));
	}

	typeLabel_->setText(typeTxt);
	nameEdit_->setText(name);
	valueEdit_->setPlainText(value);

	if(frozen || readOnly)
	{
		nameEdit_->setReadOnly(true);
		valueEdit_->setReadOnly(true);

		QPushButton* sb=buttonBox_->button(QDialogButtonBox::Save);
		assert(sb);
		sb->setEnabled(false);
	}

}

void VariablePropDialog::accept()
{
    QString name=nameEdit_->text();
	QString value=valueEdit_->toPlainText();

	if(!data_->hasName(name.toStdString()))
	{
		if(QMessageBox::question(0,tr("Confirm: create new variable"),
									tr("You are about to create a <b>new</b> variable<br>Do you want to proceed?"),
						    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
			{
				QDialog::reject();
				return;
			}
	}

	if(genVar_)
	{
		if(QMessageBox::question(0,QObject::tr("Confirm: change variable"),
						tr("You are about to modify a <b>generated variable</b>.<br>Do you want to proceed?"),
					QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel)  == QMessageBox::Cancel)
		{
			QDialog::reject();
			return;
		}
	}

	QDialog::accept();
}

QString VariablePropDialog::name() const
{
	return nameEdit_->text();
}

QString VariablePropDialog::value() const
{
	return valueEdit_->toPlainText();
}

//======================================
//
// VariableAddDialog
//
//======================================

VariableAddDialog::VariableAddDialog(VariableModelData *data,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

	label_->setText(tr("Add new variable for ") +
			QString::fromStdString(data->type()) + ":<b> " +
			QString::fromStdString(data->name()) + "</b>");
}

VariableAddDialog::VariableAddDialog(VariableModelData *data,QString name, QString value,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

	label_->setText(tr("Add new variable for ") +
			QString::fromStdString(data->type()) + ":<<b> " +
			QString::fromStdString(data->name()) + "</b>");

	nameEdit_->setText(name + "_copy");
	valueEdit_->setText(value);
}

void VariableAddDialog::accept()
{
	QString name=nameEdit_->text();
	QString value=valueEdit_->text();

	if(data_->hasName(name.toStdString()))
	{
		if(QMessageBox::question(0,tr("Confirm: overwrite variable"),
								tr("This variable is <b>already defined</b>. A new variable will be created \
								for the selected node and hide the previous one.<br>Do you want to proceed?"),
					    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
		{
			QDialog::reject();
			return;
		}
	}

	QDialog::accept();
}

QString VariableAddDialog::name() const
{
	return nameEdit_->text();
}

QString VariableAddDialog::value() const
{
	return valueEdit_->text();
}

//========================================================
//
// VariableItemWidget
//
//========================================================

VariableItemWidget::VariableItemWidget(QWidget *parent)
{
	//This item displays all the ancestors of the info object
    useAncestors_=true;
    
    setupUi(this);

	data_=new VariableModelDataHandler();

	//The model and the sort-filter model
	model_=new VariableModel(data_,this);
	sortModel_= new VariableSortModel(model_,this);
    
    //Set the model on the view
    varView->setModel(sortModel_);

    varView->setSelectionBehavior(QAbstractItemView::SelectRows);

    //Search and filter interface: we have a menu attached to a toolbutton and a
    //stackedwidget connected up.

    //Populate the toolbuttons menu
    findModeTb->addAction(actionFilter);
	findModeTb->addAction(actionSearch);

    //The filter line editor
    filterLine_=new LineEdit;
    stackedWidget->addWidget(filterLine_);

    //The search line editor. Its a custom widget handling its own signals and slots.
    searchLine_=new VariableSearchLine(this);
    stackedWidget->addWidget(searchLine_);
    searchLine_->setView(varView);
    
    //The filter editor changes
    connect(filterLine_,SIGNAL(textChanged(QString)),
        		this,SLOT(slotFilterTextChanged(QString)));

    //The selection changes in the view
	connect(varView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

	//Init the find mode selection
	if(sortModel_->matchMode() == VariableSortModel::FilterMode)
	{
		actionFilter->trigger();
	}
	else
	{
		actionSearch->trigger();
	}

	//Add context menu actions to the view
	QAction* sep1=new QAction(this);
	sep1->setSeparator(true);
	QAction* sep2=new QAction(this);
	sep2->setSeparator(true);
	QAction* sep3=new QAction(this);
	sep3->setSeparator(true);

	//Build context menu
	varView->addAction(actionAdd);
	varView->addAction(sep1);
    varView->addAction(actionCopy);
    varView->addAction(actionCopyFull);
    //varView->addAction(actionPaste);
    varView->addAction(sep2);
	varView->addAction(actionDelete);
	varView->addAction(sep3);
	varView->addAction(actionProp);

	//Add actions for the toolbuttons
	addTb->setDefaultAction(actionAdd);
	deleteTb->setDefaultAction(actionDelete);
	propTb->setDefaultAction(actionProp);
	exportTb->setDefaultAction(actionExport);

	//TODO: implemet it
	actionExport->setEnabled(false);

	//Initialise action state (it depends on the selection)
	checkActionState();
}

VariableItemWidget::~VariableItemWidget()
{

}

QWidget* VariableItemWidget::realWidget()
{
	return this;
}

//A new info object is set
void VariableItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
       return;

    clearContents();
	adjust(info);

	data_->reload(info);
	varView->expandAll();
	varView->resizeColumnToContents(0);
	//varView->reload(info);
}

void VariableItemWidget::clearContents()
{
	InfoPanelItem::clear();
	data_->clear();
}


void VariableItemWidget::slotItemSelected(const QModelIndex& idx,const QModelIndex& prevIdx)
{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "VariableItemWidget::slotItemSelected";
    qDebug() << "  current:" << idx << "prev:" << prevIdx;
    qDebug() << "  in view:" << varView->currentIndex();
#endif

    checkActionState();
}

void VariableItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    checkActionState();
}

void VariableItemWidget::checkActionState()
{
	QModelIndex vIndex=varView->currentIndex();
	QModelIndex index=sortModel_->mapToSource(vIndex);

    if(suspended_)
    {
         actionAdd->setEnabled(false);
         actionProp->setEnabled(false);
         actionDelete->setEnabled(false);
         actionCopy->setEnabled(false);
         actionCopyFull->setEnabled(false);
         return;
    }

	//The index is invalid (no selection)
	if(!index.isValid())
	{
		actionAdd->setEnabled(false);
		actionProp->setEnabled(false);
        actionDelete->setEnabled(false);
        actionCopy->setEnabled(false);
        actionCopyFull->setEnabled(false);
	}
	else
	{
		//Variables
		if(model_->isVariable(index))
		{
			if(frozen_)
			{
				actionAdd->setEnabled(false);
				actionDelete->setEnabled(false);
			}
			else
			{
				actionAdd->setEnabled(true);
				actionDelete->setEnabled(true);
			}
            actionProp->setEnabled(true);
            actionCopy->setEnabled(true);
            actionCopyFull->setEnabled(true);
		}
		//Server or nodes
		else
		{
			if(frozen_)
			{
				actionAdd->setEnabled(false);
				actionDelete->setEnabled(false);
			}
			else
			{
				actionAdd->setEnabled(true);
				actionDelete->setEnabled(false);
			}
            actionProp->setEnabled(false);
            actionCopy->setEnabled(false);
            actionCopyFull->setEnabled(false);
		}
	}
}

void VariableItemWidget::editItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("VariableItemWidget::editItem -->");
    qDebug() << "   index:" << index;
#endif

	QModelIndex vIndex=sortModel_->mapToSource(index);

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "   vIndex:" << vIndex;
#endif

	VariableModelData* data=model_->indexToData(vIndex);

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "  data:" << data;
#endif

    //Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        qDebug() << "selected before:" <<   varView->currentIndex();
#endif
        //Start edit dialog
		VariablePropDialog d(data,name,value,genVar,frozen_,this);

		if(d.exec()== QDialog::Accepted && !frozen_)
		{
            //data might have been deleted while the dialog was open
            //so we alter it via the model that can properly lookup the
            //data object            
            model_->alterVariable(vIndex,d.name(),d.value());
        }

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        qDebug() << "selected after:" <<   varView->currentIndex();
#endif
	}

    //Having finished editing we need to reselect the current row. See issue ECFLOW-613.
    varView->setCurrentIndex(varView->currentIndex());

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("<-- VariableItemWidget::editItem");
#endif

}

void VariableItemWidget::duplicateItem(const QModelIndex& index)
{
	if(frozen_)
			return;

	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	VariableModelData* data=model_->indexToData(vIndex);

	//Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
		//Start add dialog
		VariableAddDialog d(data,name,value,this);

		if(d.exec() == QDialog::Accepted)
		{
            data->alter(d.name().toStdString(),d.value().toStdString());
            //data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::addItem(const QModelIndex& index)
{
	if(frozen_)
		return;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	if(VariableModelData* data=model_->indexToData(vIndex))
	{
		//Start add dialog
		VariableAddDialog d(data,this);

		if(d.exec() == QDialog::Accepted)
        {
            data->alter(d.name().toStdString(),d.value().toStdString());
            //data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::removeItem(const QModelIndex& index)
{
	if(frozen_)
		return;

	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	VariableModelData* data=model_->indexToData(vIndex);

	//Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
		if(QMessageBox::question(0,tr("Confirm: delete variable"),
						tr("Are you sure that you want to delete variable <b>") + name + "</b> from " +
						QString::fromStdString(data->type()) + " <b>" + QString::fromStdString(data->name()) +  "</b>?",
					    QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Ok)
		{
            //data might have deleted while the dialog was open
            //so we alter it via the model that can properly lookup the
            //data object
            model_->removeVariable(vIndex,name,value);
		}
	}
}

void VariableItemWidget::on_varView_doubleClicked(const QModelIndex& index)
{
    if(!suspended_)
       editItem(index);
}

void VariableItemWidget::on_actionProp_triggered()
{
	QModelIndex index=varView->currentIndex();
	editItem(index);
}

void VariableItemWidget::on_actionAdd_triggered()
{
	QModelIndex index=varView->currentIndex();
	addItem(index);
}

void VariableItemWidget::on_actionDelete_triggered()
{
	QModelIndex index=varView->currentIndex();
	removeItem(index);
}

void VariableItemWidget::on_actionFilter_triggered()
{
	findModeTb->setIcon(actionFilter->icon());
	sortModel_->setMatchMode(VariableSortModel::FilterMode);
	filterLine_->clear();
	searchLine_->clear();

	//Notify stackedwidget
	stackedWidget->setCurrentIndex(0);
}

void VariableItemWidget::on_actionSearch_triggered()
{
	findModeTb->setIcon(actionSearch->icon());
	sortModel_->setMatchMode(VariableSortModel::SearchMode);
	filterLine_->clear();
	searchLine_->clear();

	//Notify stackedwidget
	stackedWidget->setCurrentIndex(1);

}

void VariableItemWidget::on_actionCopy_triggered()
{
   QModelIndex idx=sortModel_->mapToSource(varView->currentIndex());
   QString name, val;
   bool gen;

   if(model_->variable(idx,name,val,gen))
   {
        QString txt;
        if(idx.column() == 0)
            toClipboard(name);
        else if(idx.column() == 1)
            toClipboard(val);
    }
}

void VariableItemWidget::on_actionCopyFull_triggered()
{
   QModelIndex idx=sortModel_->mapToSource(varView->currentIndex());
   QString name, val;
   bool gen;

   if(model_->variable(idx,name,val,gen))
   {
        toClipboard(name + "=" + val);
   }
}

void VariableItemWidget::toClipboard(QString txt) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QClipboard* cb=QGuiApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#else
    QClipboard* cb=QApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#endif
}


void VariableItemWidget::slotFilterTextChanged(QString text)
{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("VariableItemWidget::slotFilterTextChanged -->");
    qDebug() << "selected before:" <<   varView->currentIndex();
#endif
    sortModel_->setMatchText(text);
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "selected after:" <<   varView->currentIndex();
    qDebug() << sortModel_->data(varView->currentIndex());
    //UserMessage::debug("<-- VariableItemWidget::slotFilterTextChanged");
#endif
}


void VariableItemWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("VariableItemWidget::nodeChanged -->");
    qDebug() << "selected before:" <<   varView->currentIndex();
#endif
    if(data_->nodeChanged(node,aspect))
    {
        //After any change we need to reselect the current row. See issue ECFLOW-613.
        varView->setCurrentIndex(varView->currentIndex());
    }
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "selected after:" <<   varView->currentIndex();
    UserMessage::debug("<-- VariableItemWidget::nodeChanged");
#endif
}

void VariableItemWidget::defsChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
	data_->defsChanged(aspect);
}

//Register at the factory
static InfoPanelItemMaker<VariableItemWidget> maker1("variable");
