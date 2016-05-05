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
#include "VConfig.hpp"
#include "VProperty.hpp"

#define _UI_VARIABLEITEMWIDGET_DEBUG

#if 0
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


#endif

//======================================
//
// VariablePropDialog
//
//======================================

VariablePropDialog::VariablePropDialog(VariableModelDataHandler *data,int defineIndex,QString name, QString value,bool frozen,QWidget *parent) :
   QDialog(parent),
   genVar_(false),
   data_(data),
   defineIndex_(defineIndex),
   oriName_(name)
{
	setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);

    Q_ASSERT(data_);
    Q_ASSERT(data_->count() > 0);
    Q_ASSERT(data_->count() > defineIndex_);

    nodeName_=QString::fromStdString(data_->data(0)->name());
    nodeType_=QString::fromStdString(data_->data(0)->type());
    nodeTypeCapital_=nodeType_;
    if(nodeTypeCapital_.size() > 0)
    {
        QChar s=nodeTypeCapital_.at(0);
        s=s.toUpper();
        nodeTypeCapital_.replace(0,1,s);
    }

    data_->addObserver(this);

    genVar_=data_->data(defineIndex)->isGenVar(name.toStdString());

    QString path=QString::fromStdString(data_->data(0)->fullPath());
    QString h="<b>Node to modify</b>: " + path + "<br>";

    VariableModelData* defineData=data_->data(defineIndex_);
    Q_ASSERT(defineData);
    defineNodeName_=QString::fromStdString(defineData->name());
    defineNodeType_=QString::fromStdString(defineData->type());

    genVar_=defineData->isGenVar(name.toStdString());
    h+="<b>Type</b>: ";
    h+=(genVar_)?tr("generated variable"):tr("user variable");

    bool readOnly=defineData->isReadOnly(name.toStdString());
    if(readOnly)
    {
        h+=" (read only)";
    }

    if(defineIndex_ > 0)
    {
        QString definePath=QString::fromStdString(defineData->fullPath());
        h+="<br>(inherited from: " + definePath + ")";
    }
    header_->setText(h);
    valueEdit_->setProperty("form","1");   
	nameEdit_->setText(name);
	valueEdit_->setPlainText(value);
    valueEdit_->setFocus();

	if(frozen || readOnly)
	{
		nameEdit_->setReadOnly(true);
		valueEdit_->setReadOnly(true);

		QPushButton* sb=buttonBox_->button(QDialogButtonBox::Save);
        Q_ASSERT(sb);
		sb->setEnabled(false);
    }

    messageLabel_->hide();
}

VariablePropDialog::~VariablePropDialog()
{
    Q_ASSERT(data_);
    data_->removeObserver(this);
}

void VariablePropDialog::accept()
{
    QString name=nameEdit_->text();
	QString value=valueEdit_->toPlainText();

    Q_ASSERT(data_);
    Q_ASSERT(data_->count() > 0);
    Q_ASSERT(data_->count() > defineIndex_);

    //var does not exists in SELECTED node
    if(!data_->data(0)->hasName(name.toStdString()))
    {
        for(size_t i=1; i < data_->count(); i++)
        {
            //but exists in one of the parents
            if(data_->data(i)->hasName(name.toStdString()))
            {
                QString type=QString::fromStdString(data_->data(i)->type());
                QString path =QString::fromStdString(data_->data(i)->fullPath());
                if(QMessageBox::question(0,tr("Confirm: create new variable"),
                    tr("Variable <b>") + name + tr("</b> is originally defined in ") + type + " <b>" + path +
                       tr("</b>. A new user variable will be created for ") +  nodeType_ + " <b>"  + nodeName_ +
                       tr("</b> and shadow the original one.<br><br>Do you want to proceed?"),
                    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
                {
                    QDialog::reject();
                    return;
                }
                else
                {
                    QDialog::accept();
                    return;
                }
            }
        }

        //It is a ne variable
		if(QMessageBox::question(0,tr("Confirm: create new variable"),
                        tr("You are about to create a <b>new</b> variable in ") + nodeType_ + " " + nodeName_ + "." +
                            tr("<br>Do you want to proceed?"),
                        QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
        {
            QDialog::reject();
            return;
        }
        else
        {
            QDialog::accept();
            return;
        }
	}
#if 0
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
#endif
	QDialog::accept();
}

void VariablePropDialog::on_nameEdit__textEdited(QString)
{
    messageLabel_->hide();
}

void VariablePropDialog::on_valueEdit__textChanged()
{
    messageLabel_->hide();
}

QString VariablePropDialog::name() const
{
	return nameEdit_->text();
}

QString VariablePropDialog::value() const
{
	return valueEdit_->toPlainText();
}

void VariablePropDialog::notifyCleared(VariableModelDataHandler*)
{
    messageLabel_->showWarning(nodeTypeCapital_ + " <b>" + nodeName_ +
         "</b> is not the node to modify any more in the Variables panel. Please close the dialog!");

    QPushButton* sb=buttonBox_->button(QDialogButtonBox::Save);
    Q_ASSERT(sb);
    sb->setEnabled(false);
    form_->setEnabled(false);

    data_->removeObserver(this);
}

void VariablePropDialog::notifyUpdated(VariableModelDataHandler*)
{
    QString name=nameEdit_->text();
    QString value=valueEdit_->toPlainText();

    bool st=false;
    QString v=QString::fromStdString(data_->value(defineNodeName_.toStdString(),name.toStdString(),st));
    if(!st)
    {
        messageLabel_->showWarning("Variable <b>" + name + "</b> is not defined any more in " + defineNodeType_ +
                                   " <b>" + defineNodeName_ + "</b>!");
    }
    else if(v != value)
    {
        messageLabel_->showWarning("The value of variable <b>" + name + "</b> changed in " + defineNodeType_ +
                " <b>" + defineNodeName_ + "</b>!");
    }
    else
    {
        messageLabel_->hide();
    }
}

//======================================
//
// VariableAddDialog
//
//======================================

VariableAddDialog::VariableAddDialog(VariableModelDataHandler *data,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

    init();
    nameEdit_->setFocus();
}

VariableAddDialog::VariableAddDialog(VariableModelDataHandler *data,QString name, QString value,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

    init();

	nameEdit_->setText(name + "_copy");
	valueEdit_->setText(value);
    nameEdit_->setFocus();
}

VariableAddDialog::~VariableAddDialog()
{
     data_->removeObserver(this);
}

void VariableAddDialog::init()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);

    Q_ASSERT(data_);
    Q_ASSERT(data_->count() > 0);

    nodeName_=QString::fromStdString(data_->data(0)->name());
    nodeType_=QString::fromStdString(data_->data(0)->type());
    nodeTypeCapital_=nodeType_;
    if(nodeTypeCapital_.size() > 0)
    {
        QChar s=nodeTypeCapital_.at(0);
        s=s.toUpper();
        nodeTypeCapital_.replace(0,1,s);
    }
    data_->addObserver(this);

    label_->setText(tr("Add new variable to ") +
            nodeType_ + " <b> " +
            nodeName_ + "</b>") ;
        //+ "<br>Path: " +
        //            QString::fromStdString(data_->data(0)->fullPath()));

    messageLabel_->hide();
}


void VariableAddDialog::accept()
{
	QString name=nameEdit_->text();

    if(name.simplified().isEmpty())
    {
        QMessageBox::critical(0,tr("Invalid variable name"),
                     tr("Variable name cannot be empty! Please specify a valid name!"),
                      QMessageBox::Ok,QMessageBox::Ok);
        return;
    }

    Q_ASSERT(data_);
    Q_ASSERT(data_->count() > 0);

    if(data_->data(0)->hasName(name.toStdString()))
    {
        QString q;
        if(data_->data(0)->isGenVar(name.toStdString()))
        {
            q=tr("Generated variable <b>") + name + tr("</b> is already defined in ") +
                nodeType_ + " <b>" + nodeName_ + "</b>" +
                tr("</b>. A new user variable will be created and the original variable will be hidden. \
                <br>Do you want to proceed?");
        }
        else
        {
            q=tr("User variable <b>") + name + tr("</b> is already defined in ") +
                nodeType_ + " <b>" + nodeName_ + "</b>" +
                tr(".<br>Do you want to overwrite it?");
        }

        if(QMessageBox::question(0,tr("Confirm: overwrite variable"),q,
           QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
        {
            return;
        }
        else
        {
            QDialog::accept();
        }
        return;
    }

    for(size_t i=1; i <data_->count(); i++)
    {
        if(data_->data(i)->hasName(name.toStdString()))
        {
            QString nodeName=QString::fromStdString(data_->data(i)->name());
            QString nodeType=QString::fromStdString(data_->data(i)->type());

            QString q;
            if(data_->data(i)->isGenVar(name.toStdString()))
            {
                q=tr("Generated variable");
            }
            else
            {
                q=tr("User variable");
            }
            q+=" <b>" + name + tr("</b> is already defined in ") +
                    nodeType + " <b>" + nodeName + "</b>" +
                    tr("</b>. A new user variable will be created for ") +  nodeType_ + " <b>" +
                    nodeName_ + tr(" </b> and shadow the original one. \
                    <br>Do you want to proceed?");

            if(QMessageBox::question(0,tr("Confirm: overwrite variable"),q,
					    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
            {
                return;
            }
            else
            {
                QDialog::accept();
            }
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

void VariableAddDialog::notifyCleared(VariableModelDataHandler*)
{
    messageLabel_->showWarning(nodeTypeCapital_ + " <b>" + nodeName_ +
         "</b> is not the node to modify any more in the Variables panel. Please close the dialog!");

    QPushButton* sb=buttonBox_->button(QDialogButtonBox::Ok);
    Q_ASSERT(sb);
    sb->setEnabled(false);
    form_->setEnabled(false);

    data_->removeObserver(this);
}

//========================================================
//
// VariableItemWidget
//
//========================================================

VariableItemWidget::VariableItemWidget(QWidget *parent) : shadowProp_(0)
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
    varView->setSelectionMode(QAbstractItemView::SingleSelection);

    //Shadowed variables
    bool showShadowed = true;
    //We do not want to observe it!
    shadowProp_=VConfig::instance()->find("panel.variable.showShadowed");
    if(shadowProp_)
    {
       showShadowed=shadowProp_->value().toBool();
    }
    sortModel_->slotShowShadowed(showShadowed);
    shadowTb->setChecked(showShadowed);

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

    if(data_->count() > 0)
        actionAdd->setText(tr("Add &new variable to ") +
                           QString::fromStdString(data_->data(0)->name()));
}

void VariableItemWidget::clearContents()
{
	InfoPanelItem::clear();
    data_->clear();
    actionAdd->setText(tr("Add &new variable"));
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
                int block=-1;
                if(model_->indexToData(index,block))
                {
                    if(block==0)
                        actionDelete->setEnabled(true);
                    else
                        actionDelete->setEnabled(false);
                }
                else
                    actionDelete->setEnabled(false);
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

    int block=-1;
    VariableModelData* data=model_->indexToData(vIndex,block);

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::qdebug("   block=" + QString::number(block));
#endif

    //Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        qDebug() << "selected before:" <<   varView->currentIndex();
#endif
        Q_ASSERT(data_->count() > 0);
        Q_ASSERT(block >=0);

        //Start edit dialog (will be deleted on close - deleteOnClose is set)
        VariablePropDialog* d=new VariablePropDialog(data_,block,name,value,frozen_,this);
        d->show();
        connect(d,SIGNAL(accepted()),
                this,SLOT(slotVariableEdited()));
#if 0
        if(d.exec()== QDialog::Accepted && !frozen_)
		{
            //data might have been deleted while the dialog was open
            //so we alter it via the model that can properly lookup the
            //data object            
            model_->alterVariable(vIndex,d.name(),d.value());
        }
#endif

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        qDebug() << "selected after:" <<   varView->currentIndex();
#endif
	}

    //Having finished editing we need to reselect the current row. See issue ECFLOW-613.
    //reselectCurrent();

#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("<-- VariableItemWidget::editItem");
#endif

}

void VariableItemWidget::duplicateItem(const QModelIndex& index)
{
	if(frozen_)
			return;

#if 0
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
#endif

}

void VariableItemWidget::addItem(const QModelIndex& index)
{
	if(frozen_)
		return;

    if(data_->count() > 0)
    {
        //Start add dialog (will be deleted on close - deleteOnClose is set)
        VariableAddDialog* d=new VariableAddDialog(data_,this);
        d->show();
        connect(d,SIGNAL(accepted()),
                this,SLOT(slotVariableAdded()));

#if 0
        if(d.exec() == QDialog::Accepted)
        {
            Q_ASSERT(data_->count() > 0);
            data_->data(0)->alter(d.name().toStdString(),d.value().toStdString());
        }
#endif

    }

#if 0
	QModelIndex vIndex=sortModel_->mapToSource(index);

	if(VariableModelData* data=model_->indexToData(vIndex))
	{
		//Start add dialog
        VariableAddDialog d(data_,this);

		if(d.exec() == QDialog::Accepted)
        {
            data->alter(d.name().toStdString(),d.value().toStdString());           
		}
	}
#endif

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

void VariableItemWidget::slotVariableEdited()
{
    VariablePropDialog* d=static_cast<VariablePropDialog*>(sender());
    Q_ASSERT(d);

    model_->alterVariable(QModelIndex(),d->name(),d->value());
}

void VariableItemWidget::slotVariableAdded()
{
    VariableAddDialog* d=static_cast<VariableAddDialog*>(sender());
    Q_ASSERT(d);
    Q_ASSERT(data_->count() > 0);
    data_->data(0)->alter(d->name().toStdString(),d->value().toStdString());
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

void VariableItemWidget::on_shadowTb_clicked(bool showShadowed)
{
   if(shadowProp_)
   {
       shadowProp_->setValue(showShadowed);
       qDebug() << shadowProp_->value().toBool();
   }
   sortModel_->slotShowShadowed(showShadowed);
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

    QModelIndex cIdx=sortModel_->mapToSource(varView->currentIndex());

    if(data_->nodeChanged(node,aspect))
    {
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        UserMessage::debug("   reselect currentIndex!");
#endif
        //After any change we need to reselect the current row. See issue ECFLOW-613.
        reselectCurrent();
    }
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "selected after:" <<   varView->currentIndex();
    UserMessage::debug("<-- VariableItemWidget::nodeChanged");
#endif
}

void VariableItemWidget::defsChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    UserMessage::debug("VariableItemWidget::defsChanged -->");
    qDebug() << "selected before:" <<   varView->currentIndex();
#endif
    if(data_->defsChanged(aspect))
    {
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
        UserMessage::debug("   reselect currentIndex!");
#endif
        //After any change we need to reselect the current row. See issue ECFLOW-613.
        reselectCurrent();
    }
#ifdef _UI_VARIABLEITEMWIDGET_DEBUG
    qDebug() << "selected after:" <<   varView->currentIndex();
    UserMessage::debug("<-- VariableItemWidget::defsChanged");
#endif
}

//This is a fairly complicated solution but reselection does not work otherwise
//if the second column is selected initially!!!
void  VariableItemWidget::reselectCurrent()
{
    QModelIndex idx=sortModel_->mapToSource(varView->currentIndex());
    if(idx.column() == 1)
    {
        idx=model_->index(idx.row(),0,idx.parent());
    }
    varView->selectionModel()->clear();
    QModelIndex sortIdx=sortModel_->mapFromSource(idx);
    varView->selectionModel()->setCurrentIndex(sortIdx,QItemSelectionModel::Rows|
                                   QItemSelectionModel::Select);
}

//Register at the factory
static InfoPanelItemMaker<VariableItemWidget> maker1("variable");
