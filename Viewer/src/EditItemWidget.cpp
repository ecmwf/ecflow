//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "EditItemWidget.hpp"

#include "EditProvider.hpp"
#include "Highlighter.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// EditItemWidget
//
//========================================================

EditItemWidget::EditItemWidget(QWidget *parent) :
   QWidget(parent),
   preproc_(false),
   alias_(false)
{
	setupUi(this);

	/*QFont f("Courier");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	f.setPointSize(10);
	textEdit_->setFont(f);*/

	infoProvider_=new EditProvider(this);

	Highlighter* ih=new Highlighter(textEdit_->document(),"script");

	searchLine_->setEditor(textEdit_);

	searchLine_->setVisible(false);

	//connect(submitTb_,SIGNAL(clicked(bool)),
	//		this,SLOT(on_submitTb__clicked(bool)));
}

QWidget* EditItemWidget::realWidget()
{
	return this;
}

void EditItemWidget::reload(VInfo_ptr info)
{
	clearContents();

	enabled_=true;
	info_=info;

	if(info_ && info_.get())
	{
		//Get file contents
		EditProvider* ep=static_cast<EditProvider*>(infoProvider_);
		ep->preproc(preproc());
		infoProvider_->info(info_);
	}
}

void EditItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
}

void EditItemWidget::infoReady(VReply* reply)
{
	QString s=QString::fromStdString(reply->text());
	textEdit_->setPlainText(s);
}

void EditItemWidget::infoFailed(VReply*)
{

}
void EditItemWidget::infoProgress(VReply*)
{

}

void EditItemWidget::on_preprocTb__toggled(bool)
{
	reload(info_);
}

void EditItemWidget::on_submitTb__clicked(bool)
{
	QStringList lst=textEdit_->toPlainText().split("\n");
	std::vector<std::string> txt;
	Q_FOREACH(QString s,lst)
	{
		txt.push_back(s.toStdString());
	}

	EditProvider* ep=static_cast<EditProvider*>(infoProvider_);
	ep->submit(txt,alias());
}

void EditItemWidget::on_searchTb__clicked()
{
	searchLine_->setVisible(true);
	searchLine_->setFocus();
	searchLine_->selectAll();
}

void EditItemWidget::on_gotoLineTb__clicked()
{
	textEdit_->gotoLine();
}

bool EditItemWidget::alias() const
{
	return aliasTb_->isChecked();
}

bool EditItemWidget::preproc() const
{
	return preprocTb_->isChecked();
}


static InfoPanelItemMaker<EditItemWidget> maker1("edit");




/*
static const char* micro="%";
const char* sStart = "comment - ecf user variables";
const char* sEnd   = "end - ecf user variables";

extern "C" {
#include "xec.h"
}

edit::edit(panel_window& w):
	panel(w),
	text_window(false),
	loading_(False),
	preproc_(False),
	tmp_(0), kStart(0x0), kEnd(0x0)
{
  if (kStart==NULL)
    kStart=(char*)calloc(1024, sizeof(char*));
  if (kEnd==NULL)
    kEnd=(char*)calloc(1024, sizeof(char*));
}

edit::~edit()
{
  if(tmp_) XtFree(tmp_);
  if (kStart) free(kStart);
  if (kEnd) free(kEnd);
}

void edit::create (Widget parent, char *widget_name )
{
	edit_form_c::create(parent,widget_name);
        XmToggleButtonSetState(alias_, globals::get_resource("send_as_alias", 0), FALSE);
}

void edit::clear()
{
	loading_ = True;
	XmTextSetString(text_,(char*)"");
	loading_ = False;
}

void edit::show(node& n)
{
  loading_ = True;
  XmTextSetString(text_,(char*)"");

  // tmp_file v(tmpnam(0), true); FILE *f = fopen(v.c_str(),"w");
  char tmpname[] = "/tmp/xecfXXXXXX";
  int  fid = mkstemp(tmpname);
  FILE *f = fdopen(fid, "w");

  if(!f) {
    gui::syserr(tmpname);
    return;
  }

  std::list<Variable> vl; // FILL handle vl
  tmp_file tmp(NULL);
  tmp = n.serv().edit(n, vl, preproc_);

  if(fclose(f)) {
    gui::syserr(tmpname);
    return;
  }

  xec_LoadText(text_, tmpname, True);
  xec_LoadText(text_, tmp.c_str(), True);

  XmTextSetInsertionPosition(text_,0);
  XmTextShowPosition(text_, 0);

  loading_ = False;
}

void edit::changed(node&)
{
}

Boolean edit::enabled(node& n)
{
	return n.type() == NODE_TASK;
}

void edit::changedCB(Widget,XtPointer data)
{
	if(!loading_) freeze();
}

void edit::preprocCB(Widget,XtPointer data)
{
	preproc_ = XmToggleButtonGetState(preprocess_);
	if(get_node())
		show(*get_node());
	else
		clear();
}

static char* strip(char* n)
{
  int l = strlen(n) - 1;
  while(l >= 0 && n[l] == ' ')
    n[l--] = 0;

  char* p = n;
  while(*p && *p == ' ') p++;

  return p;
}

void edit::submitCB(Widget,XtPointer)
{
  bool alias = XmToggleButtonGetState(alias_);
  bool run   = true;
  char line[4096];
  node *nd = get_node();

  if(nd) {
    tmp_file t(tmpnam(0), true);
    if(xec_SaveText(text_,(char*)t.c_str())) {
      gui::syserr(t.c_str());
      return;
    }

    NameValueVec var;
    FILE *f = fopen(t.c_str(),"r");
    if(!f) {
      gui::syserr(t.c_str());
      return;
    }

    const std::string& mv = nd->__node__() ?
      nd->variable("ECF_MICRO") : nd->variable("SMSMICRO");
    const char * mic = (mv.size() == 1) ? mv.c_str() : micro;
    sprintf(kStart, "%s%s", mic, sStart);
    sprintf(kEnd,   "%s%s", mic, sEnd);

    bool isvars = false;
    while(fgets(line,sizeof(line),f)) {
      line[strlen(line)-1] = 0;

      if(isvars) {
	char* p = line;
	while(*p && *p != '=') p++;
	if(*p == '=') {
	  *p = 0;

	  char n[1024];
	  char v[1024];

	  strcpy(n,line);
	  strcpy(v,p+1);

	  var.push_back(std::make_pair(strip(n), strip(v)));
	}
      }

      if (strcmp(line,kStart) == 0)
	isvars = true;
      if(strcmp(line,kEnd) == 0)
	break;
    }

    if(var.empty()) {
      gui::message("No user variables!");
      // return;
    }

    get_node()->serv().send(*get_node(),alias,run,var,t.c_str());

  } else
    clear();

  if (alias != globals::get_resource("send_as_alias", 0))
    globals::set_resource("send_as_alias", alias);

  submit();
}

void edit::externalCB(Widget,XtPointer)
{
	if(tmp_) XtFree(tmp_);
	tmp_ = XtNewString(tmpnam(0));

	if(xec_SaveText(text_,tmp_))
	{
		gui::syserr(tmp_);
		return;
	}

	char cmd[1024];
	const char* xedit = getenv("XEDITOR");
	if (xedit)
	  sprintf(cmd,"${XEDITOR:=xterm -e vi} %s",tmp_);
	else
	  sprintf(cmd,"xterm -e ${EDITOR:=vi} %s",tmp_);

	FILE *f = popen(cmd,"r");
	if(!f) {
		gui::syserr(cmd);
		return;
	}
	XtSetSensitive(text_,False);
	XtSetSensitive(tools_,False);
	XtSetSensitive(tools2_,False);

	start(f);
}

void edit::ready(const char* line)
{
	gui::error("%s",line);
}

void edit::done(FILE* f)
{
  stop();

  if(pclose(f)) {
    gui::error("External editor returns error");
    return;
  }

  if(xec_LoadText(text_,tmp_,False))
    gui::syserr(tmp_);

  unlink(tmp_);

  XtSetSensitive(text_,True);
  XtSetSensitive(tools_,True);
  XtSetSensitive(tools2_,True);
}
*/
