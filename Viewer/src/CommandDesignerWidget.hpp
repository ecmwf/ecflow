//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef COMMANDDESIGNERWIDGET_HPP_
#define COMMANDDESIGNERWIDGET_HPP_

#include <QDialog>

class CommandLineEdit;

#include "ui_CommandDesignerWidget.h"
#include "CustomCommandHandler.hpp"
#include "NodeQueryResultModel.hpp"

#include "CtsCmdRegistry.hpp"

class CommandDesignerWidget : public QWidget, private Ui::commandDesignerWidget
{
	Q_OBJECT

public:
	explicit CommandDesignerWidget(QWidget *parent = 0);
	~CommandDesignerWidget();

	QString command() {return commandLineEdit_->text();};
	void setNodes(std::vector<VInfo_ptr> nodes);



public Q_SLOTS:
	void insertComponent(QListWidgetItem *);
	void on_commandLineEdit__textChanged();
	void on_saveNameLineEdit__textChanged();
	void on_overwriteButton__clicked();
	void on_saveAsNewButton__clicked();
	void on_runButton__clicked();
	void on_savedCommandsTable__cellClicked(int row, int column);
	void on_componentsList__itemEntered(QListWidgetItem *item);
	void on_componentsList__itemClicked(QListWidgetItem *item);
	void on_componentsList__itemDoubleClicked(QListWidgetItem *item);
	QPushButton *runButton() {return runButton_;};


private:
	void updateSaveButtonStatus();
	void addCommandToSavedList(CustomCommand *command, int row);
	void refreshSavedCommandList();
	void addClientCommandsToComponentList();
	void showCommandHelp(QListWidgetItem *item, bool showFullHelp);
	void initialiseCommandLine();
	//bool eventFilter(QObject* object, QEvent* event);

	bool currentCommandSaved_;
	bool haveSetUpDefaultCommandLine_;

	std::vector<VInfo_ptr> nodes_;
	NodeQueryResultModel nodeModel_; 

	CtsCmdRegistry cmdRegistry_;
	boost::program_options::options_description* clientOptionsDescriptions_;
};




#endif
