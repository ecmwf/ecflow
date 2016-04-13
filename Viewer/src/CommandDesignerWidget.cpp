//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <boost/bind.hpp>

#include <QMessageBox>

#include "CommandDesignerWidget.hpp"
#include "CustomCommandHandler.hpp"
#include "Child.hpp"
#include "Str.hpp"
#include "MenuHandler.hpp"
#include "NodeQueryResult.hpp"

using namespace boost;
namespace po = boost::program_options;


CommandDesignerWidget::CommandDesignerWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);


	//at least for now, all commands will start with 'ecflow_client' and end with '<full_name>'
	commandLineEdit_->setText("ecflow_client  <full_name>");
	//commandLineEdit_->installEventFilter(this);
	commandLineEdit_->setFocus();

	haveSetUpDefaultCommandLine_ = false;
	inCommandEditMode_           = false;
	saveCommandsOnExit_          = false;

	addToContextMenuCb_->setChecked(true);  // by default, suggest saving to context menu

	// ensure the Save button is in the right state
	on_commandLineEdit__textChanged();

	saveNameLineEdit_->setPlaceholderText(tr("Unnamed"));

	currentCommandSaved_ = false;
	refreshSavedCommandList();

	// ensure we start on the command-builder tab
	changeToTab(TAB_BUILD);
	on_tabWidget__currentChanged(TAB_BUILD);  // trigger the callback to ensure the correct visibility of the save buttons

	// set up and populate our list of options to the ecflow client
	// - this would be more efficient if we did it only once, in a singleton, but
	//   it seems pretty fast so we'll leave it like this for now
	clientOptionsDescriptions_ = new po::options_description("help" , po::options_description::m_default_line_length + 80);
	cmdRegistry_.addAllOptions(*clientOptionsDescriptions_);
	addClientCommandsToComponentList();


	infoLabel_->setShowTypeTitle(false);
	infoLabel_->showInfo(tr("Click command for help, double-click to insert"));

	nodeSelectionView_->enableContextMenu(false);


	nodeListLinkLabel_->setOpenExternalLinks(false);


	connect(nodeSelectionView_, SIGNAL(selectionChanged()), this, SLOT(on_nodeSelectionChanged()));


	setSavedCommandsButtonStatus();


	// temporary
	//saveCommandGroupBox_->setVisible(false);
	//tabWidget_->setTabEnabled(2, false);
	//savedCommandsGroupBox_->setVisible(false);

}

CommandDesignerWidget::~CommandDesignerWidget()
{
	delete clientOptionsDescriptions_;

	if (saveCommandsOnExit_)
		CustomSavedCommandHandler::instance()->writeSettings();

	MenuHandler::refreshCustomMenuCommands();
}


void CommandDesignerWidget::initialiseCommandLine()
{
	if (!haveSetUpDefaultCommandLine_)
	{
		// put the cursor between the two pre-defined strings - this is where the command will go
		commandLineEdit_->home(false);
		commandLineEdit_->setCursorPosition(14);
		commandLineEdit_->deselect();

		haveSetUpDefaultCommandLine_ = true;
	}
}


void CommandDesignerWidget::changeToTab(TabIndexes i)
{
	tabWidget_->setCurrentIndex(i);
}


void CommandDesignerWidget::setNodes(std::vector<VInfo_ptr> &nodes)
{
	nodes_ = nodes;


	// populate the list of nodes
	nodeSelectionView_->setSourceModel(&nodeModel_);
	nodeModel_.slotBeginReset();
	nodeModel_.data()->add(nodes);
	nodeModel_.slotEndReset();

	// all should be selected at first
	nodeSelectionView_->selectAll();


	on_nodeSelectionChanged();  // get the number of selected nodes and act accordingly
}



// when the user clicks on the hyperlinked label which tells them how many nodes
// will be acted on, we want to switch to the Nodes tab
void CommandDesignerWidget::on_nodeListLinkLabel__linkActivated(const QString &link)
{
	if (link == "#nodes")
	{
		changeToTab(TAB_NODES);
	}
}


void CommandDesignerWidget::setNodeNumberLinkText(int numNodes)
{
	QString s;

	s = (numNodes == 1) ? "" : "s";

	nodeListLinkLabel_->setText(tr("Command will be run on %1 node%2: <a href=\"#nodes\">click here for list</a>").arg(numNodes).arg(s));
}

// triggered when the user changes their node selection
void CommandDesignerWidget::on_nodeSelectionChanged()
{
	setNodeNumberLinkText(selectedNodes().size());
	on_commandLineEdit__textChanged();  // trigger the enabling/disabling of the Run button
}


std::vector<VInfo_ptr> &CommandDesignerWidget::selectedNodes()
{
	nodeSelectionView_->getListOfSelectedNodes(nodes_);
	return nodes_;
}


void CommandDesignerWidget::addClientCommandsToComponentList()
{
	// sort the commands into alphabetical order
	std::vector< boost::shared_ptr<po::option_description> > options = clientOptionsDescriptions_->options();

	std::sort(options.begin(),options.end(),
		boost::bind(std::less<std::string>(),
			boost::bind(&po::option_description::long_name,_1),
			boost::bind(&po::option_description::long_name,_2)));

	// loop through the sorted commands and add them to the list
	size_t numOptions = options.size();

	for(size_t i = 0; i < numOptions; i++)
	{
		if (!ecf::Child::valid_child_cmd(options[i]->long_name()))  // do not show the 'child' options
		{
			componentsList_->addItem(QString("--") + QString::fromStdString(options[i]->long_name()));
			QString statusTip(QString::fromStdString(options[i]->long_name()));
			componentsList_->item(componentsList_->count()-1)->setStatusTip(statusTip);
		}
	}

	// ensure the itemEntered slot is triggered
	componentsList_->setMouseTracking(true);

	// when the mouse hovers over an item, set the background colour of that item
	componentsList_->setStyleSheet("QListWidget::item:hover {background-color:#FFFFDD;color:black}");
}


void CommandDesignerWidget::showCommandHelp(QListWidgetItem *item, bool showFullHelp)
{
	// get the command name
	QString qCommand(item->text());
	std::string command = qCommand.toStdString();
	ecf::Str::replace_all(command, "--", "");  // remove the "--" from the start


	// try to find it in our list of commands
	const po::option_description* od = clientOptionsDescriptions_->find_nothrow(command,
				false,  /*approx, will find nearest match*/
				false, /*long_ignore_case = false*/
				false  /*short_ignore_case = false*/
				);

	if (od)
	{
		// get the description, but only take the first line
		std::vector< std::string > lines;
		ecf::Str::split(od->description(),lines,"\n");
		if (!lines.empty())
		{
			QString text = qCommand + QString(": ");
			commandHelpLabel_->setText(text + QString::fromStdString(lines[0]));
		}

		if (showFullHelp)
		{
			commandManPage_->setText(qCommand + "\n\n" + QString::fromStdString(od->description()));
		}
	}
	else
	{
		// not a command that we have help text for
		commandHelpLabel_->setText("");
	}
}


void CommandDesignerWidget::on_tabWidget__currentChanged(int index)
{
	bool onSaveTab = (index == TAB_SAVE);
	saveCommandGroupBox_->setVisible(onSaveTab);
	saveOptionsButton_->setVisible(!onSaveTab);
}



// when the mouse moves over a command, display the help text for it
void CommandDesignerWidget::on_componentsList__itemEntered(QListWidgetItem *item)
{
	showCommandHelp(item, false);
	initialiseCommandLine();
}

// when the mouse is clicked on a command, display the help text for it
void CommandDesignerWidget::on_componentsList__itemClicked(QListWidgetItem *item)
{
	showCommandHelp(item, true);
	commandLineEdit_->setFocus(); // to keep the text cursor visible
}

// when the mouse is double-clicked on a command, insert it into the command line box
void CommandDesignerWidget::on_componentsList__itemDoubleClicked(QListWidgetItem *item)
{
	insertComponent(item);
	commandLineEdit_->setFocus(); // to keep the text cursor visible
}


void CommandDesignerWidget::insertComponent(QListWidgetItem *item)
{
	commandLineEdit_->insert(item->text() + " ");
}



void CommandDesignerWidget::on_commandLineEdit__textChanged()
{
	 // only allow to run a non-empty command, and on 1 or more nodes
	runButton_->setEnabled((!commandLineEdit_->text().isEmpty()) && nodes_.size() > 0);

	currentCommandSaved_ = false;
	updateSaveButtonStatus();
}

void CommandDesignerWidget::on_saveNameLineEdit__textChanged()
{
	currentCommandSaved_ = false;
	updateSaveButtonStatus();
}

void CommandDesignerWidget::on_addToContextMenuCb__stateChanged()
{
	currentCommandSaved_ = false;
	updateSaveButtonStatus();
}


void CommandDesignerWidget::updateSaveButtonStatus()
{
	// logic:
	// - if in editing mode and current name is unique (or unchanged) then enable 'overwrite'
	// - otherwise, disable 'overwrite'
	// - if in editing mode, disable 'save as new'
	// - otherwise, if name is unique then enable 'save as new'
	// - if in editing mode, enable 'add to context menu'
	// - otherwise, use the same logic as 'save as new'

	bool enoughInfo =  (!commandLineEdit_->text().isEmpty() && !saveNameLineEdit_->text().isEmpty());
	int commandWithThisName = CustomSavedCommandHandler::instance()->findIndexFromName(saveNameLineEdit_->text().toStdString());

	if (inCommandEditMode_)
	{
		bool nameUnique =  (commandWithThisName == -1 || commandWithThisName == savedCommandsTable_->currentRow());
		bool okToOverwrite =  (enoughInfo && nameUnique);
		overwriteButton_->setEnabled(okToOverwrite);
		saveAsNewButton_->setEnabled(false);
		addToContextMenuCb_->setEnabled(true);
	}
	else
	{
		overwriteButton_->setEnabled(false);

		bool nameUnique =  (commandWithThisName == -1);
		bool okToOverwrite =  (enoughInfo && nameUnique);
		saveAsNewButton_->setEnabled(okToOverwrite);
		addToContextMenuCb_->setEnabled(okToOverwrite);
	}

	// the cancel button is only available if we're in edit mode
	cancelSaveButton_->setEnabled(inCommandEditMode_);


	if (!saveAsNewButton_->isEnabled() && !overwriteButton_->isEnabled())
	{
		nameInfoLabel_->setShowTypeTitle(false);
		nameInfoLabel_->showWarning(tr("Command must have a unique name"));
		nameInfoLabel_->setVisible(true);
	}
	else
	{
		nameInfoLabel_->setVisible(false);
	}

}


void CommandDesignerWidget::setSavedCommandsButtonStatus()
{
	int row = savedCommandsTable_->currentRow();
	bool isRowSelected = (row != -1);
	deleteCommandButton_   ->setEnabled(isRowSelected);
	editCommandButton_     ->setEnabled(isRowSelected);
	duplicateCommandButton_->setEnabled(isRowSelected);
	useCommandButton_      ->setEnabled(isRowSelected);

	upButton_  ->setEnabled(isRowSelected && row != 0);  // not the first row
	downButton_->setEnabled(isRowSelected && row != savedCommandsTable_->rowCount()-1); // not the last row
}


bool CommandDesignerWidget::validSaveName(const std::string &name, int indexToIgnore)
{
	// is there already a command with this name?
	int foundIndex = CustomSavedCommandHandler::instance()->findIndexFromName(name);
	if (foundIndex != -1 and foundIndex !=indexToIgnore)
	{
		QMessageBox::critical(0,QObject::tr("Custom command"), tr("A command with that name already exists - please choose another name"));
		return false;
	}
	else
	{
		return true;
	}
}

void CommandDesignerWidget::selectRow(int row)
{
	savedCommandsTable_->setCurrentCell(row, 0);
}


void CommandDesignerWidget::selectLastSavedCommand()
{
	int lastRow = savedCommandsTable_->rowCount()-1;
	selectRow(lastRow);
}

// swap the commands in these two positions and select the one which will end up in the second position
void CommandDesignerWidget::swapSavedCommands(int i1, int i2)
{
	CustomSavedCommandHandler::instance()->swapCommandsByIndex(i1, i2);
	refreshSavedCommandList();
	selectRow(i2);
	setSavedCommandsButtonStatus();
	saveCommandsOnExit_ = true;  // we won't save the commands yet, but mark for save on exit
}


void CommandDesignerWidget::on_saveAsNewButton__clicked()
{
	std::string name, command;
	bool context;

	name    = saveNameLineEdit_->text().toStdString();
	command = commandLineEdit_->text().toStdString();
	context = addToContextMenuCb_->isChecked();

	//if (validSaveName(name, -1))
	{
		CustomCommand *cmd = CustomSavedCommandHandler::instance()->add(name, command, context, true);
		refreshSavedCommandList();
		currentCommandSaved_ = true;
		updateSaveButtonStatus();
		changeToTab(TAB_SAVE);
		selectLastSavedCommand();
		setSavedCommandsButtonStatus();
	}
}

void CommandDesignerWidget::on_overwriteButton__clicked()
{
	std::string name, command;
	bool context;

	name    = saveNameLineEdit_->text().toStdString();
	command = commandLineEdit_->text().toStdString();
	context = addToContextMenuCb_->isChecked();

	//if (validSaveName(name, -1))
	{
		CustomCommand *cmd = CustomSavedCommandHandler::instance()->replace(savedCommandsTable_->currentRow(), name, command, context);
		savedCommandsTable_->setEnabled(true);  // to show that we are no longer busy editing an entry
		inCommandEditMode_ = false;
		refreshSavedCommandList();
		currentCommandSaved_ = true;
		updateSaveButtonStatus();
	}
}


void CommandDesignerWidget::on_runButton__clicked()
{
	std::string command = commandLineEdit_->text().toStdString();

	// save this in the command history
	CustomCommandHistoryHandler::instance()->add(command, command, true, true);


	// close the dialogue - the calling function will call the command() function
	// to retrieve the user's command
	//accept();
}


void CommandDesignerWidget::on_saveOptionsButton__clicked()
{
	// we just switch to the Saved Commands tab
	changeToTab(TAB_SAVE);
}


void CommandDesignerWidget::on_useCommandButton__clicked()
{
	// just put the command into the command edit box
	int row = savedCommandsTable_->currentRow();
	QTableWidgetItem *commandItem = savedCommandsTable_->item(row, 2);
	commandLineEdit_->setText(commandItem->text());
}


void CommandDesignerWidget::on_editCommandButton__clicked()
{
	int row = savedCommandsTable_->currentRow();

	// get the details of this command from the table
	QTableWidgetItem *nameItem    = savedCommandsTable_->item(row, 0);
	QTableWidgetItem *contextItem = savedCommandsTable_->item(row, 1);
	QTableWidgetItem *commandItem = savedCommandsTable_->item(row, 2);


	inCommandEditMode_ = true;

	// insert the details into the edit boxes
	commandLineEdit_->setText(commandItem->text());
	saveNameLineEdit_->setText(nameItem->text());
	std::string context = contextItem->text().toStdString();
	addToContextMenuCb_->setChecked(CustomSavedCommandHandler::instance()->stringToBool(context));

	savedCommandsTable_    ->setEnabled(false);  // to show that we are busy editing an entry
	deleteCommandButton_   ->setEnabled(false);  // to show that we are busy editing an entry
	editCommandButton_     ->setEnabled(false);  // to show that we are busy editing an entry
	duplicateCommandButton_->setEnabled(false);  // to show that we are busy editing an entry
	useCommandButton_      ->setEnabled(false);  // to show that we are busy editing an entry
	upButton_              ->setEnabled(false);  // to show that we are busy editing an entry
	downButton_            ->setEnabled(false);  // to show that we are busy editing an entry

}


void CommandDesignerWidget::on_savedCommandsTable__cellDoubleClicked(int row, int column)
{
	on_editCommandButton__clicked();  // same as selecting a cell and clicking 'edit'
}

void CommandDesignerWidget::on_duplicateCommandButton__clicked()
{
	CustomSavedCommandHandler::instance()->duplicate(savedCommandsTable_->currentRow());
	refreshSavedCommandList();
}


void CommandDesignerWidget::on_deleteCommandButton__clicked()
{
	CustomSavedCommandHandler::instance()->remove(savedCommandsTable_->currentRow());
	refreshSavedCommandList();
}

void CommandDesignerWidget::on_upButton__clicked()
{
	int row = savedCommandsTable_->currentRow();
	swapSavedCommands(row, row-1);
}

void CommandDesignerWidget::on_downButton__clicked()
{
	int row = savedCommandsTable_->currentRow();
	swapSavedCommands(row, row+1);
}


void CommandDesignerWidget::on_cancelSaveButton__clicked()
{
	savedCommandsTable_->setEnabled(true);  // to show that we are no longer busy editing an entry
	inCommandEditMode_ = false;
	updateSaveButtonStatus();
	refreshSavedCommandList();
}


void CommandDesignerWidget::refreshSavedCommandList()
{
	int n = CustomSavedCommandHandler::instance()->numCommands();

	savedCommandsTable_->clearContents();

	for (int i = 0; i < n; i++)
	{
		CustomCommand *command = CustomSavedCommandHandler::instance()->commandFromIndex(i);
		addCommandToSavedList(command, i);
	}
	savedCommandsTable_->setRowCount(n);
	setSavedCommandsButtonStatus();
}


void CommandDesignerWidget::addCommandToSavedList(CustomCommand *command, int row)
{
	QTableWidgetItem *nameItem    = new QTableWidgetItem(QString::fromStdString(command->name()));
	QTableWidgetItem *contextItem = new QTableWidgetItem(QString::fromStdString(command->contextString()));
	QTableWidgetItem *commandItem = new QTableWidgetItem(QString::fromStdString(command->command()));

	// if the command already exists (by name) then we will replaced it;
	// otherwise add a new row to the table

//	int thisRow = CustomCommandHandler::instance()->findIndex(command->name());
//	if (thisRow == -1)
//		thisRow = savedCommandsTable_->rowCount();

	//contextItem->setCheckable();

	int lastRow = savedCommandsTable_->rowCount()-1;

	if (row > lastRow)
		savedCommandsTable_->insertRow(row);

	savedCommandsTable_->setItem(row, 0, nameItem);
	savedCommandsTable_->setItem(row, 1, contextItem);
	savedCommandsTable_->setItem(row, 2, commandItem);
}


void CommandDesignerWidget::on_savedCommandsTable__cellClicked(int row, int column)
{
	setSavedCommandsButtonStatus();
}


/*
bool CommandDesignerWidget::eventFilter(QObject* object, QEvent* event)
{
	if(object == commandLineEdit_ && event->type() == QEvent::FocusIn)
	{
		initialiseCommandLine();
		return false;
	}
	return false;
}
*/
