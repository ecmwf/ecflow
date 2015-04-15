//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputItemWidget.hpp"

#include "Highlighter.hpp"
#include "OutputProvider.hpp"
#include "OutputModel.hpp"
#include "VReply.hpp"

#include <QDebug>

OutputItemWidget::OutputItemWidget(QWidget *parent) :
	QWidget(parent)
{
	setupUi(this);

	QFont f;
	f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	textEdit_->setFont(f);

	Highlighter* ih=new Highlighter(textEdit_->document(),"output");

	infoProvider_=new OutputProvider(this);

	outputView_->setRootIsDecorated(false);
	outputView_->setAllColumnsShowFocus(true);
	outputView_->setUniformRowHeights(true);
	outputView_->setAlternatingRowColors(true);
	outputView_->setSortingEnabled(true);

	//OutputData* data=new OutputData;
	model_=new OutputModel(this);

	outputView_->setModel(model_);
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(VInfo_ptr info)
{
	loaded_=true;
	info_=info;

	if(!info.get())
	{
		fileLabel_->clear();
		textEdit_->clear();
	}
    else
	{
	    clearContents();

	    QString name=QString::fromStdString(info_->genVariable("ECF_JOBOUT"));

	    fileLabel_->setText(tr("File: ") + name);
	    infoProvider_->info(info_);

	    OutputProvider* op=static_cast<OutputProvider*>(infoProvider_);

	    VDir_ptr dir=op->directory();

	    if(dir)
	    {
	    	model_->setData(dir);
	    	outputView_->show();
	    }
	    else
	    {
	    	outputView_->hide();
	    }



	    /*QString name=QString::fromStdString(info_->genVariable("ECF_JOBOUT"));

	    qDebug() << "NAME" << name;

	    fileLabel_->setText(tr("File: ") + name);
	    infoProvider_->info(info_);

	    int lindex=name.lastIndexOf("/");
	    if(lindex != -1)
	    {
	    	name=name.left(lindex+1);

	    	QFileInfo fileInfo(name);
	    	QDir dir=fileInfo.absoluteDir();

	    	qDebug() << "DIR" << dir.absolutePath();
	    	if(dir.exists())
	    	{
	    		model_->setRootPath(dir.absolutePath());
	    		outputView_->setRootIndex(model_->index(dir.absolutePath()));
	    	}
	    }*/
	}
}

void OutputItemWidget::clearContents()
{
	loaded_=false;
	fileLabel_->clear();
	textEdit_->clear();
	//model_->clear();
}

void OutputItemWidget::infoReady(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void OutputItemWidget::infoProgress(VReply* reply)
{
    QString s=QString::fromStdString(reply->text());
    textEdit_->setPlainText(s);
}

void OutputItemWidget::infoFailed(VReply* reply)
{
    QString s=QString::fromStdString(reply->errorText());
    textEdit_->setPlainText(s);
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");

/*
std::string error;
 bool read = direct_read_;

    if (name == "ECF_SCRIPT")
    {
    	error = "no script!\n"
    			"check ECF_FILES or ECF_HOME directories, for read access\n"
    			"check for file presence and read access below files directory\n"
    			"or this may be a 'dummy' task.\n";
 	 }
    else if (name == "ECF_JOB")
    {
    	std::string filename = n.variable(name);
    	if (read && (access(filename.c_str(), R_OK) == 0))
    		return tmp_file(filename.c_str(), false);

    	if (std::string::npos != filename.find(".job0")) {
    		error = "job0: no job to be generated yet!";
    		return tmp_file(error);
    	} else
    		error = "no script!\n"
    				"check ECF_HOME,directory for read/write access\n"
    				"check for file presence and read access below\n"
    				"The file may have been deleted\n"
    				"or this may be a 'dummy' task.\n";

    }
    else if (boost::algorithm::ends_with(name, ".0"))
    {
    	error = "no output to be expected when TRYNO is 0!\n";
    	return tmp_file(error);
    }
    else if (name != ecf_node::none())
    { // Try logserver
    	loghost_ = n.variable("ECF_LOGHOST", true);
    	logport_ = n.variable("ECF_LOGPORT");
    	if (loghost_ == ecf_node::none()) {
    		loghost_ = n.variable("LOGHOST", true);
    		logport_ = n.variable("LOGPORT");
    	}
    	std::string::size_type pos = loghost_.find(n.variable("ECF_MICRO"));
    	if (std::string::npos == pos && loghost_ != ecf_node::none())
    	{
    		logsvr the_log_server(loghost_, logport_);
    		if (the_log_server.ok()) {
    			tmp_file tmp = the_log_server.getfile(name); // allow more than latest output
    			if (access(tmp.c_str(), R_OK) == 0) return tmp;
    		}
    	}
    }

    if (read && (access(name.c_str(), R_OK) == 0))
    {
    	return tmp_file(name.c_str(), false);
    }
    else
    {
    	gui::message("%s: fetching %s", this->name(), name.c_str());
    	try
    	{
    		if (name == "ECF_SCRIPT")
    			client_.file(n.full_name(), "script");
    		else if (name == "ECF_JOB")
    		{
    			client_.file(n.full_name(), "job", boost::lexical_cast<std::string>(jobfile_length_));
    		}
    		else if (name == "ECF_JOBOUT")
    			client_.file(n.full_name(), "jobout");
    		else
    		{
    			client_.file(n.full_name(), "jobout");
    		}

    		// Do *not* assign 'client_.server_reply().get_string()' to a separate string, since
    		// in the case of job output the string could be several megabytes.
    		return tmp_file( client_.server_reply().get_string()
    				+ "\n# file is served by ecflow-server\n" );
    	}
    	catch ( std::exception &e )
    	{
    		std::cerr << "host::file-error:" << e.what() << "\n";
    		gui::message("host::file-error: %s", e.what());
    	}
    }

 return tmp_file(error);

*/









/*




class output_lister : public lister<ecf_dir> {
	Widget list_;
	bool sort() { return true; }
	bool compare(ecf_dir&,ecf_dir&);
	void next(ecf_dir&);
public:
	output_lister(Widget l) : list_(l) {}
};

void output_lister::next(ecf_dir& d)
{
  if(S_ISREG(d.mode))
    {
      time_t t   = d.mtime;
      time_t now = time(0);

      int delta  = now - t;
      if(delta<0) delta = 0;

      char buf[80];
      strcpy(buf,"Right now");

      if(delta >=1  && delta < 60)
	{
	  sprintf(buf,"%d second%s ago",delta,delta>1?"s":"");
	}

      if(delta >= 60 && delta < 60*60)
	{
	  sprintf(buf,"%d minute%s ago",delta/60,delta/60>1?"s":"");
	}

      if(delta >= 60*60 && delta < 60*60*24)
	{
	  sprintf(buf,"%d hour%s ago",delta/60/60,delta/60/60>1?"s":"");
	}

      if(delta >= 60*60*24)
	{
	  sprintf(buf,"%d day%s ago",delta/60/60/24,delta/60/60/24>1?"s":"");
	}

      xec_VaAddListItem(list_,(char*) "%-60s (%s)",d.name_,buf);
    }
}

bool output_lister::compare(ecf_dir& a,ecf_dir& b)
{
  return a.mtime > b.mtime;
}


















// Name        :

#include "output.h"
#include "node.h"
#include "host.h"
#include <Xm/Text.h>
#include <Xm/TextStrSoP.h>
#include <Xm/List.h>
#include "ecf_node.h"
extern "C" {
#include "xec.h"
}

#include <sys/types.h>
#include <sys/stat.h>

output::output(panel_window& w):
	text_window(true),
	panel(w),
	file_(0x0)
{
}

output::~output()
{
	if(file_)
		free(file_);
}

void output::create (Widget parent, char *widget_name )
{
	output_form_c::create(parent,widget_name);
}

void output::clear()
{
	if(file_) free(file_);
	file_ = 0x0;
	XmTextSetString(name_,(char*) "");
	XmListDeleteAllItems(list_);
	//active(False);
	text_window::clear();
}

class output_lister : public lister<ecf_dir> {
	Widget list_;
	bool sort() { return true; }
	bool compare(ecf_dir&,ecf_dir&);
	void next(ecf_dir&);
public:
	output_lister(Widget l) : list_(l) {}
};

void output_lister::next(ecf_dir& d)
{
  if(S_ISREG(d.mode))
    {
      time_t t   = d.mtime;
      time_t now = time(0);

      int delta  = now - t;
      if(delta<0) delta = 0;

      char buf[80];
      strcpy(buf,"Right now");

      if(delta >=1  && delta < 60)
	{
	  sprintf(buf,"%d second%s ago",delta,delta>1?"s":"");
	}

      if(delta >= 60 && delta < 60*60)
	{
	  sprintf(buf,"%d minute%s ago",delta/60,delta/60>1?"s":"");
	}

      if(delta >= 60*60 && delta < 60*60*24)
	{
	  sprintf(buf,"%d hour%s ago",delta/60/60,delta/60/60>1?"s":"");
	}

      if(delta >= 60*60*24)
	{
	  sprintf(buf,"%d day%s ago",delta/60/60/24,delta/60/60/24>1?"s":"");
	}

      xec_VaAddListItem(list_,(char*) "%-60s (%s)",d.name_,buf);
    }
}

bool output_lister::compare(ecf_dir& a,ecf_dir& b)
{
  return a.mtime > b.mtime;
}

class search_me : public runnable {
	find& find_;

	void run() {
	  // text case regexp back wrap
	  find_.search("System Billing Units",true,false,false,true);
	  find_.search("smscomplete",true,false,false,true);
	  find_.search("smsabort",true,false,false,true);
	  // display init but not appreciated when updating for tail:
	  // find_.search("ecflow_client",true,false,false,true);
	  find_.search("xcomplete",true,false,false,true);
	  find_.search("xabort",true,false,false,true);
	  find_.search(" --complete",true,false,false,true);
	  find_.search(" --abort",true,false,false,true);
	  find_.no_message();
	  find_.pending(0);
	  delete this;
	}

public:
	search_me(find& f)  : find_(f) { find_.pending(this); enable();}
};

void output::show(node& n)
{
  std::string jobout = n.variable("ECF_JOBOUT");
  if (!n.__node__())
    jobout = n.variable("SMSJOBOUT");
  else if (!n.__node__()) return;
  else if (!n.__node__()->get_node()) return;
  else n.__node__()->get_node()->variableSubsitution(jobout);

  if(jobout == ecf_node::none()) {
    clear();
    return;
  }

  // output variable may contain micro

  if(file_) free(file_);
  file_ = strdup(jobout.c_str());
  load(n);
  XmListDeleteAllItems(list_);

  output_lister ol(list_);
  n.serv().dir(n,file_,ol);

  std::string remote = n.variable("ECF_OUT");
  std::string job    = n.variable("ECF_JOB");
  if (!n.__node__()) {
    remote = n.variable("SMSOUT");
    job    = n.variable("SMSJOB");
  }
  if (!remote.empty() && !job.empty()) {
    // display both remote and local dir
    if (remote == job) {
      output_lister rem(list_);
      n.serv().dir(n,job.c_str(),rem);
    }
  }
  new search_me(*this);
}

void output::load(node& n)
{
	if(file_)
		XmTextSetString(name_,(char*)file_);
	else
		clear();

	if(file_) {
	  tmp_file f = n.serv().file(n,file_);
	  text_window::load(f);
	} else {
	  tmp_file f = n.serv().output(n);
	  text_window::load(f);
	}
}

void output::updateCB(Widget,XtPointer data)
{
	if(get_node())
		show(*get_node());
	else
		clear();
	XmTextShowPosition(text_,XmTextGetLastPosition(text_));
}

void output::browseCB(Widget,XtPointer data)
{
	XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	if(file_) free(file_);

	char *p = xec_GetString(cb->item);
	char buf[1024];
	sscanf(p,"%s",buf);
	XtFree(p);

	file_ = strdup(buf);

	if(get_node())
		load(*get_node());
	else
		clear();
}

Boolean output::enabled(node& n)
{
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) return False;
  if (!n.__node__())
    return n.variable("SMSJOBOUT") != ecf_node::none();
  return n.variable("ECF_JOBOUT") != ecf_node::none();
}

*/
