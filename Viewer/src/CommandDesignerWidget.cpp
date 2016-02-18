//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <boost/bind.hpp>

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


	// ensure the Save button is in the right state
	on_commandLineEdit__textChanged();

	saveNameLineEdit_->setPlaceholderText(tr("Unnamed"));

	currentCommandSaved_ = false;

	refreshSavedCommandList();


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


	// temporary
	//saveCommandGroupBox_->setVisible(false);
	//tabWidget_->setTabEnabled(1, false);
	//savedCommandsGroupBox_->setVisible(false);

}

CommandDesignerWidget::~CommandDesignerWidget()
{
	delete clientOptionsDescriptions_;

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
		tabWidget_->setCurrentIndex(1);
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



// when the mouse moves over an item, display the help text for it
void CommandDesignerWidget::on_componentsList__itemEntered(QListWidgetItem *item)
{
	showCommandHelp(item, false);
	initialiseCommandLine();
}

// when the mouse moves over an item, display the help text for it
void CommandDesignerWidget::on_componentsList__itemClicked(QListWidgetItem *item)
{
	showCommandHelp(item, true);
	commandLineEdit_->setFocus(); // to keep the text cursor visible
}

// when the mouse moves over an item, display the help text for it
void CommandDesignerWidget::on_componentsList__itemDoubleClicked(QListWidgetItem *item)
{
	insertComponent(item);
	commandLineEdit_->setFocus(); // to keep the text cursor visible
}


void CommandDesignerWidget::insertComponent(QListWidgetItem *item)
{
	//commandLineEdit_->setText("Silly");
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

void CommandDesignerWidget::updateSaveButtonStatus()
{

	// logic:
	// - if no row in the saved command table is selected, then 'overwrite'
	//   is disabled
	// - if nothing has been modified, then 'overwrite' is disabled
	// - otherwise 'overwrite' is enabled
	// - if the name is not empty, and it exists in the current list, then
	//   then 'save as new' is disabled
	// - if no command is entered, then both buttons are disabled

	if (savedCommandsTable_->currentRow() == -1 ||
		currentCommandSaved_ ||
		commandLineEdit_->text().isEmpty())
		overwriteButton_->setEnabled(false);
	else
		overwriteButton_->setEnabled(true);


	int thisRow = CustomSavedCommandHandler::instance()->findIndexFromName(saveNameLineEdit_->text().toStdString());

	if ((!saveNameLineEdit_->text().isEmpty() && thisRow != -1) ||
		commandLineEdit_->text().isEmpty())
		saveAsNewButton_->setEnabled(false);
	else
		saveAsNewButton_->setEnabled(true);



/*

	saveAsNewButton_->setEnabled((!commandLineEdit_->text().isEmpty()));


	if (commandLineEdit_->text().isEmpty() || currentCommandSaved_)
	{
		overwriteButton_->setEnabled(false);
	}
	else
	{
		overwriteButton_->setEnabled(true);
	}
*/
/*
	// if the currently-entered name already exists in our list of
	// commands, change the Save button to 'Overwrite'

	QString name(saveNameLineEdit_->text());
	if (!name.isEmpty() && CustomCommandHandler::instance()->find(name.toStdString()))
	{
		overwriteButton_->setText(tr("Overwrite"));
	}
	else
	{
		overwriteButton_->setText(tr("Save"));
	}
*/
}


void CommandDesignerWidget::on_saveAsNewButton__clicked()
{
	std::string name, command;
	bool context;

	name    = saveNameLineEdit_->text().toStdString();
	command = commandLineEdit_->text().toStdString();
	context = addToContextMenuCb_->isChecked();
	CustomCommand *cmd = CustomSavedCommandHandler::instance()->add(name, command, context);
	refreshSavedCommandList();
	currentCommandSaved_ = true;
	updateSaveButtonStatus();
}

void CommandDesignerWidget::on_overwriteButton__clicked()
{
	std::string name, command;
	bool context;

	name    = saveNameLineEdit_->text().toStdString();
	command = commandLineEdit_->text().toStdString();
	context = addToContextMenuCb_->isChecked();
	CustomCommand *cmd = CustomSavedCommandHandler::instance()->replace(savedCommandsTable_->currentRow(), name, command, context);
	refreshSavedCommandList();
	currentCommandSaved_ = true;
	updateSaveButtonStatus();
}


void CommandDesignerWidget::on_runButton__clicked()
{
	std::string command = commandLineEdit_->text().toStdString();

	// save this in the command history
	CustomCommandHistoryHandler::instance()->add(command, command, true);


	// close the dialogue - the calling function will call the command() function
	// to retrieve the user's command
	//accept();
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
}



void CommandDesignerWidget::addCommandToSavedList(CustomCommand *command, int row)
{
	QTableWidgetItem *nameItem    = new QTableWidgetItem(QString::fromStdString(command->name()));
	QTableWidgetItem *contextItem = new QTableWidgetItem();
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
	savedCommandsTable_->setItem(row, 2, commandItem);
}


void CommandDesignerWidget::on_savedCommandsTable__cellClicked(int row, int column)
{
	// get the details of this command from the table
	QTableWidgetItem *nameItem    = savedCommandsTable_->item(row, 0);
	QTableWidgetItem *contextItem = savedCommandsTable_->item(row, 1);
	QTableWidgetItem *commandItem = savedCommandsTable_->item(row, 2);

	// insert the details into the edit boxes
	commandLineEdit_->setText(commandItem->text());
	saveNameLineEdit_->setText(nameItem->text());
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
