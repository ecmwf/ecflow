
#include "NodeSearchDialog.hpp"

#include "ComboMulti.hpp"
#include "Highlighter.hpp"
#include "ServerFilter.hpp"

#include <QtGlobal>
#include <QVBoxLayout>


NodeSearchDialog::NodeSearchDialog(QWidget *parent) :
    QDialog(parent),
	serverFilter_(NULL)
{
    setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    searchLe_->setPlaceholderText(tr("Search"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    searchLe_->setClearButtonEnabled(true);
    rootLe_->setClearButtonEnabled(true);
#endif

    //Query list
    queryListPanel_->hide();
    queryListTb_->setChecked(false);

    connect(queryListTb_,SIGNAL(clicked(bool)),
    		queryListPanel_,SLOT(setVisible(bool)));

    QFont f;
    QFontMetrics fm(f);

    queryTe_->setFixedHeight((fm.height()+2)*3+6);
    queryTe_->setReadOnly(true);

	Highlighter* ih=new Highlighter(queryTe_->document(),"query");

	//Query
	queryTb_->setChecked(true);

	connect(queryTb_,SIGNAL(clicked(bool)),
			queryTe_,SLOT(setVisible(bool)));

	connect(queryTb_,SIGNAL(clicked(bool)),
			queryLabel_,SLOT(setVisible(bool)));

    //Node type
    typeCb_->addItem("server");
    typeCb_->addItem("suite");
    typeCb_->addItem("family");
    typeCb_->addItem("task");
    typeCb_->addItem("alias");

    //Node state
    stateCb_->addItem("aborted");
    stateCb_->addItem("active");
    stateCb_->addItem("complete");
    stateCb_->addItem("queued");
    stateCb_->addItem("submitted");
    stateCb_->addItem("suspended");
    stateCb_->addItem("unknown");

    //Node attributes
    inCb_->addItem("node_name");
    inCb_->addItem("node_path");
    inCb_->addItem("meter");
    inCb_->addItem("label_name");
    inCb_->addItem("label_value");
    inCb_->addItem("event");
    inCb_->addItem("repeat");
    inCb_->addItem("date");
    inCb_->addItem("time");
    inCb_->addItem("limit");
    inCb_->addItem("limiter");
    inCb_->addItem("late");
    inCb_->addItem("trigger");
    inCb_->addItem("variable_name");
    inCb_->addItem("variable_value");
    inCb_->addItem("generated_variable_name");
    inCb_->addItem("generated_variable_value");

    //Flags
    flagCb_->addItem("is_late");
    flagCb_->addItem("has_date");
    flagCb_->addItem("has_message");
    flagCb_->addItem("has_time");
    flagCb_->addItem("is_rerun");
    flagCb_->addItem("is_waiting");
    flagCb_->addItem("is_zombie");


    connect(exactMatchCh_,SIGNAL(clicked(bool)),
    		this,SLOT(slotExactMatch(bool)));

    connect(searchLe_,SIGNAL(textEdited(QString)),
        	this,SLOT(slotSearchTermEdited(QString)));

    connect(rootLe_,SIGNAL(textEdited(QString)),
            this,SLOT(slotSearchTermEdited(QString)));

    connect(stateCb_,SIGNAL(selectionChanged()),
            this,SLOT(buildQuery()));

    connect(typeCb_,SIGNAL(selectionChanged()),
    		this,SLOT(buildQuery()));

    connect(flagCb_,SIGNAL(selectionChanged()),
            this,SLOT(buildQuery()));

    connect(inCb_,SIGNAL(selectionChanged()),
            this,SLOT(slotInCbChanged()));

    //Reset buttons
    connect(stateResetTb_,SIGNAL(clicked()),
    		stateCb_,SLOT(clearSelection()));

    connect(typeResetTb_,SIGNAL(clicked()),
        	typeCb_,SLOT(clearSelection()));

    connect(flagResetTb_,SIGNAL(clicked()),
            flagCb_,SLOT(clearSelection()));

    connect(inResetTb_,SIGNAL(clicked()),
            inCb_,SLOT(clearSelection()));

    //Find button
    connect(findPb_,SIGNAL(clicked()),
    		this,SLOT(slotFind()));
}

NodeSearchDialog::~NodeSearchDialog()
{
	if(serverFilter_)
		serverFilter_->removeObserver(this);
}

void NodeSearchDialog::slotExactMatch(bool)
{
	buildQuery();
}

void NodeSearchDialog::slotSearchTermEdited(QString)
{
	buildQuery();
	check();
}

void NodeSearchDialog::slotRootNodeEdited(QString)
{
	buildQuery();
}

void NodeSearchDialog::slotInCbChanged()
{
	buildQuery();
	check();
}

void NodeSearchDialog::check()
{
	if(!searchLe_->text().simplified().isEmpty() &&
	   inCb_->selection().isEmpty())
	{
		findPb_->setEnabled(false);
	}
	else
	{
		findPb_->setEnabled(true);
	}
}

void NodeSearchDialog::buildQuery()
{
	QString s;
	QString sTerm=searchLe_->text().simplified();
	if(sTerm.isEmpty())
	{
		sTerm="ANY";
	}
	else
	{
		sTerm="\'" +  sTerm + "\'";
 	}

	if(stateCb_->selection().count() >0)
	{
		s+="( " + stateCb_->selection().join(" or ") + " )";
	}

	if(typeCb_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + typeCb_->selection().join(" or ") + " )";
	}

	if(flagCb_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + flagCb_->selection().join(" or ") + " )";
	}

	if(inCb_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";

		s+="(";

		QString eqOper="=";
		if(exactMatchCh_->isChecked())
		{
			eqOper="~";
		}

		for(int i=0; i < inCb_->selection().count(); i++)
		{
			if(i >0)
				s+=" or ";
			s+=inCb_->selection().at(i) + " " + eqOper + " " + sTerm;
		}

		s+=")";
	}

	if(!rootLe_->text().simplified().isEmpty())
	{
		s+=" from " + rootLe_->text();
	}

	queryTe_->setPlainText(s);
}

void NodeSearchDialog::updateServers()
{
	serverCb_->clear();

	if(serverFilter_)
	{
		for(std::vector<ServerItem*>::const_iterator it=serverFilter_->items().begin(); it != serverFilter_->items().end(); ++it)
		{
			serverCb_->addItem(QString::fromStdString((*it)->name()));
		}
	}

	buildQuery();
}


void NodeSearchDialog::setServerFilter(ServerFilter* sf)
{
	if(serverFilter_ == sf)
		return;

	if(serverFilter_)
	{
		serverFilter_->removeObserver(this);
	}

	serverFilter_=sf;

	if(serverFilter_)
	{
		serverFilter_->addObserver(this);
	}

	updateServers();
}

void NodeSearchDialog::notifyServerFilterAdded(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->addServerObserver(this);
	updateTitle();*/
}

void NodeSearchDialog::notifyServerFilterRemoved(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->removeServerObserver(this);
	updateTitle();*/
}

void NodeSearchDialog::notifyServerFilterChanged(ServerItem*)
{
	//updateTitle();
}

void NodeSearchDialog::notifyServerFilterDelete()
{
	serverFilter_->removeObserver(this);
	serverFilter_=0;
}

void NodeSearchDialog::slotFind()
{
	QString q=queryTe_->toPlainText();
}

