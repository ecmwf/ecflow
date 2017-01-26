//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MenuHandler.hpp"
#include "MenuConfigDialog.hpp"


MenuConfigDialog::MenuConfigDialog(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
	//connect (insertPushButton_, SIGNAL(clicked()), this, SLOT(insertCurrentText()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));



	// load up the user menu file if it exists  XXXX

	if (0)
	{
	}
	else
	{
		// otherwise create a dummy menu to start with

		Menu *menu = new Menu("UserTemp");
		MenuHandler::addMenu(menu);

		MenuItem *item = new MenuItem("UserTemp");
		item->setAsSubMenu();
		item->setCommand("NoCommand");

		MenuHandler::addItemToMenu(item, "Node");


		//MenuItem *item2 = new MenuItem("Com");
		//item2->setCommand("ecflow --whatever");
		//MenuHandler::addItemToMenu(item2, "UserTemp");

		//updateMenuTree(menu);
		updateMenuTree(MenuHandler::findMenu("Node"));
	}


}


void MenuConfigDialog::updateMenuTree(Menu *menu)
{
	menuTreeWidget_->clear();
	menuTreeWidget_->setColumnCount(1);


	QTreeWidgetItem *topLevelItem = new QTreeWidgetItem(menuTreeWidget_);
	topLevelItem->setText(0, QString::fromStdString(menu->name()));
	//topLevelItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);

	addChildrenToMenuTree(menu, topLevelItem);

}


void MenuConfigDialog::addChildrenToMenuTree(Menu *menu, QTreeWidgetItem *parent)
{
	std::vector<MenuItem *>&items = menu->items();

	for (std::vector<MenuItem*>::iterator itItems = items.begin(); itItems != items.end(); ++itItems)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(parent);
		item->setText(0, QString::fromStdString((*itItems)->name()));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);

		if ((*itItems)->isSubMenu())
		{
			Menu *submenu = MenuHandler::findMenu((*itItems)->name());
			if (menu)
			{
				addChildrenToMenuTree(submenu, item);
			}
		}
		else if  ((*itItems)->isDivider())
		{
			//qmenu->addSeparator();
		}
		else
		{
			//QAction *action = (*itItems)->action();
			//qmenu->addAction(action);
			//action->setParent(parent);
			//action->setEnabled(compatible);
		}
	}
}



//void CommandDesignerWidget::insertCurrentText()
//{
//	//commandLineEdit_->setText("Silly");
//	commandLineEdit_->insert(componentsComboBox_->currentText() + " ");
//}

void MenuConfigDialog::reject()
{
	QDialog::reject();
}
