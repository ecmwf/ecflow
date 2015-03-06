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


//========================================================
//
// VariableItemWidget
//
//========================================================

VariableItemWidget::VariableItemWidget(QWidget *parent)
{
	setupUi(this);

	data_=new VariableModelDataHandler();

	model_=new VariableModel(data_,this);
	sortModel_= new VariableSortModel(model_,this);
    
    //!!!!We need to do it because:
    //The background colour between the tree view's left border and the items in the first column be
    //controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    //colour from the palette. Here we set this to transparent so that Qt could leave
    //this area empty and we will fill it appropriately in our delegate.
    QPalette pal=varView->palette();
    pal.setColor(QPalette::Highlight,Qt::transparent);
    varView->setPalette(pal);
	
    //varView->setProperty("var","1");
	//varView->setIndentation(16);
	varView->setModel(sortModel_);

	filterLine->setDecoration(QPixmap(":/viewer/filter_decor.svg"));


	searchLine_->hide();

    //Initialise the search widget
    //searchLine_->setView(varView);
    
	/*varView->setRootIsDecorated(false);
	varView->setAllColumnsShowFocus(true);
	varView->setUniformRowHeights(true);
	varView->setSortingEnabled(true);*/
	//varView->setSelectionMode(QAbstractItemView::ExtendedSelection);

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
}

VariableItemWidget::~VariableItemWidget()
{

}

QWidget* VariableItemWidget::realWidget()
{
	return this;
}

void VariableItemWidget::reload(VInfo_ptr info)
{
	adjust(info);
	data_->reload(info);
	varView->expandAll();
	varView->resizeColumnToContents(0);
	//varView->reload(info);
}

void VariableItemWidget::clearContents()
{
	loaded_=false;
}


void VariableItemWidget::editItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	//Get the data from the model
	if(model_->data(vIndex,name,value,genVar))
	{
		//Start edit dialog
		VariableEditDialog d(name,value,genVar,this);

		//The dialog checks the name and valueS
		if(d.exec()== QDialog::Accepted)
		{
			model_->setData(vIndex,name,d.value());
		}
	}
}

void VariableItemWidget::duplicateItem(const QModelIndex& index)
{
	std::string name;
	std::string value;
	/*
	sortModel_->data(index,name,value);



	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index)))
	{
		std::string dname=ServerList::instance()->uniqueName(item->name());

		VariableEditDialog d(QString::fromStdString(dname),
						   QString::fromStdString(item->host()),
						   QString::fromStdString(item->port()),this);

		//The dialog checks the name, host and port!
		if(d.exec() == QDialog::Accepted)
		{
			model_->dataIsAboutToChange();
			ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString());
			model_->dataChangeFinished();
		}
	}*/
}

void VariableItemWidget::addItem()
{
	QString name,value;
	VariableEditDialog d(name,value,this);

	//The dialog checks the name, host and port!
	if(d.exec() == QDialog::Accepted)
	{
		/*model_->setData();
		model_->dataIsAboutToChange();
		//ServerItem* item=ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString());
		model_->dataChangeFinished();
		if(d.addToView() && filter_)
		{
			filter_->addServer(item);
		}*/
	}
}

void VariableItemWidget::removeItem(const QModelIndex& index)
{
	/*ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index));

	if(!item)
		return;

	QString str=tr("Are you sure that you want to delete server: <b>");
	str+=QString::fromStdString(item->name()) ;
	str+="</b>?";
	str+=tr("<br>It will be removed from all the existing views!");

	QMessageBox msgBox;
	msgBox.setWindowTitle(tr("Delete server"));
	msgBox.setText(str);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret=msgBox.exec();

	switch (ret)
	{
   	case QMessageBox::Yes:
   		model_->dataIsAboutToChange();
   		//It will delete item as well!
   		ServerList::instance()->remove(item);
   		model_->dataChangeFinished();
      	break;
   	case QMessageBox::Cancel:
       	// Cancel was clicked
       	break;
   	default:
       	// should never be reached
      	 break;
	}*/
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
	addItem();
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



