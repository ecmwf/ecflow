
#include "NodeQueryDialog.hpp"

#include <QCloseEvent>
#include <QDebug>
#include <QSettings>

NodeQueryDialog::NodeQueryDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    //Read the qt settings
    readSettings();
}

NodeQueryDialog::~NodeQueryDialog()
{
}

void NodeQueryDialog::closeEvent(QCloseEvent * event)
{
	event->accept();
	writeSettings();
}

void NodeQueryDialog::accept()
{
	writeSettings();
    QDialog::accept();
}


void NodeQueryDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

void NodeQueryDialog::setServerFilter(ServerFilter* sf)
{
	queryWidget->setServerFilter(sf);
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void NodeQueryDialog::writeSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeQueryDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	//settings.setValue("current",list_->currentRow());
	settings.endGroup();
}

void NodeQueryDialog::readSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeQueryDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(550,540));
	}

	/*if(settings.contains("current"))
	{
		int current=settings.value("current").toInt();
		if(current >=0)
			list_->setCurrentRow(current);
	}*/
	settings.endGroup();
}
