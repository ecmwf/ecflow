//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableItemWidget.hpp"

#include <QDebug>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "VariableModel.hpp"
#include "VariableModelData.hpp"

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
// VariableEditDialog
//
//======================================

VariableEditDialog::VariableEditDialog(QString name, QString value,bool genVar,QWidget *parent) :
   QDialog(parent),
   genVar_(genVar)
{
	setupUi(this);

	nameLabel_->setText(name);
	valueEdit_->setText(value);
}

void VariableEditDialog::accept()
{
	QString value=valueEdit_->text();

	if(genVar_)
	{
		if(QMessageBox::Ok!=
				QMessageBox::question(0,QObject::tr("Confirm: change variable"),
						"You are about to modify a <b>generated variable</b>.<br>Do you want to proceed?"),
						QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel)
		{
			QDialog::reject();
			return;
		}
	}

	//if(!checkValue(name))
	//	return;

	QDialog::accept();
}

QString VariableEditDialog::name() const
{
	return nameLabel_->text();
}

QString VariableEditDialog::value() const
{
	return valueEdit_->text();
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
			"<b>" + QString::fromStdString(data->type()) + "</b>: " +
			QString::fromStdString(data->name()));
}

VariableAddDialog::VariableAddDialog(VariableModelData *data,QString name, QString value,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

	label_->setText(tr("Add new variable for ") +
			"<b>" + QString::fromStdString(data->type()) + "</b>: " +
			QString::fromStdString(data->name()));

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
								tr("This variable is <b>already defined<b>. A new variable will be created \
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

	model_=new VariableModel(data_,this);
	sortModel_= new VariableSortModel(model_,this);
    
    //!!!!We need to do it because:
    //The background colour between the tree view's left border and the items in the first column cannot be
    //controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    //colour from the palette. Here we set this to transparent so that Qt could leave
    //this area empty and we will fill it appropriately in our delegate.
    QPalette pal=varView->palette();
    pal.setColor(QPalette::Highlight,Qt::transparent);
    varView->setPalette(pal);
	
    //varView->setProperty("var","1");
	//varView->setIndentation(16);
	varView->setModel(sortModel_);

	//filterLine->setDecoration(QPixmap(":/viewer/filter_decor.svg"));
	filterLine->hide();
	filterTb->hide();

	//searchLine_->hide();

    //Initialise the search widget
    searchLine_->setView(varView);
    
	/*varView->setRootIsDecorated(false);
	varView->setAllColumnsShowFocus(true);
	varView->setUniformRowHeights(true);
	varView->setSortingEnabled(true);*/
	//varView->setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(varView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));


	//Add context menu actions to the view
	QAction* sep1=new QAction(this);
	sep1->setSeparator(true);
	QAction* sep2=new QAction(this);
	sep2->setSeparator(true);

	varView->addAction(actionAdd);
	varView->addAction(sep1);
	varView->addAction(actionDuplicate);
	varView->addAction(actionEdit);
	varView->addAction(sep2);
	varView->addAction(actionDelete);

	//Add actions for the toolbuttons
	addTb->setDefaultAction(actionAdd);
	deleteTb->setDefaultAction(actionDelete);
	editTb->setDefaultAction(actionEdit);

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
	adjust(info);
	data_->reload(info);
	varView->expandAll();
	varView->resizeColumnToContents(0);
	//varView->reload(info);
	loaded_=true;
}

void VariableItemWidget::clearContents()
{
	loaded_=false;
}


void VariableItemWidget::slotItemSelected(const QModelIndex& idx,const QModelIndex& /*prevIdx*/)
{
	checkActionState();
}

void VariableItemWidget::checkActionState()
{
	QModelIndex vIndex=varView->currentIndex();
	QModelIndex index=sortModel_->mapToSource(vIndex);

	//The index is invalid (no selection)
	if(!index.isValid())
	{
		actionAdd->setEnabled(false);
		actionEdit->setEnabled(false);
		actionDuplicate->setEnabled(false);
		actionDelete->setEnabled(false);
	}
	else
	{
		//Variables
		if(model_->isVariable(index))
		{
			actionAdd->setEnabled(true);
			actionEdit->setEnabled(true);
			actionDuplicate->setEnabled(true);
			actionDelete->setEnabled(true);
		}
		//Server or nodes
		else
		{
			actionAdd->setEnabled(true);
			actionEdit->setEnabled(false);
			actionDuplicate->setEnabled(false);
			actionDelete->setEnabled(false);
		}
	}
}

void VariableItemWidget::editItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	//Get the data from the model
	if(model_->variable(vIndex,name,value,genVar))
	{
		//Start edit dialog
		VariableEditDialog d(name,value,genVar,this);

		if(d.exec()== QDialog::Accepted)
		{
			model_->setVariable(vIndex,name,d.value());
		}
	}
}

void VariableItemWidget::duplicateItem(const QModelIndex& index)
{
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
			data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::addItem(const QModelIndex& index)
{
	QModelIndex vIndex=sortModel_->mapToSource(index);

	if(VariableModelData* data=model_->indexToData(vIndex))
	{
		//Start add dialog
		VariableAddDialog d(data,this);

		if(d.exec() == QDialog::Accepted)
		{
			data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::removeItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	//Get the data from the model
	if(model_->variable(vIndex,name,value,genVar))
	{
		if(QMessageBox::question(0,tr("Confirm: delete variable"),
						tr("Are you sure that you want to delete variable <b>") + name + "</b>?",
					    QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Ok)
		{
			model_->removeVariable(vIndex,name,value);
		}
	}
}


void VariableItemWidget::on_varView_doubleClicked(const QModelIndex& index)
{
	editItem(index);
}

void VariableItemWidget::on_actionEdit_triggered()
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

void VariableItemWidget::on_actionDuplicate_triggered()
{
	QModelIndex index=varView->currentIndex();
	duplicateItem(index);
}

void VariableItemWidget::on_filterTb_toggled(bool b)
{
	sortModel_->enableFilter(b);
	//searchLine_->setFilter(b);
}

void VariableItemWidget::on_filterLine_textChanged(QString text)
{
	sortModel_->setFilterText(text);
	//searchLine_->setFilter(b);
}


void VariableItemWidget::nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& aspect)
{
	data_->nodeChanged(node,aspect);
}

static InfoPanelItemMaker<VariableItemWidget> maker1("variable");



