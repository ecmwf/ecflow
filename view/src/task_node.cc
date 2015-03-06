//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #15 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "task_node.h"
#include "text_lister.h"
#include "late.h"
#include "zombie.h"
#include "to_check.h"
#include "host.h"
#include "url.h"
#include "re.h"
#include "ecf_node.h"

char *ecf_flag_name[]  = { (char*)"has been forced to aborted",
      (char*)"user edit failed",
      (char*)"the job failed",
      (char*)"editing failed (.job file can not be created)",
      (char*)"job could not be submitted (ECF_CMD failed)",
      (char*)"ECF could not find the script",
      (char*)"killed by user",
			   (char*)"", // has been migrated",
      (char*)"is late",
      (char*)"has user messages",
      (char*)"complete by rule",
      (char*)"queue limit reached",
      (char*)"running task is waiting for trigger",
      (char*)"node is locked by a user",
      (char*)"zombie is trying to communicate",
      (char*)"task is submitted or active (ecf) but not matching job visible",
      NULL };

#ifdef BRIDGE
task_node::task_node(host& h,sms_node* n, char b):
  simple_node(h,n, b)
{}
#endif

task_node::task_node(host& h,ecf_node* n):
	simple_node(h,n)
{
  if (kids_ == 0x0) {
    folded_ = False; 
  } else {
    folded_ = True; 
  }
}


task_node::~task_node()
{
}

void task_node::info(std::ostream& f) {
  simple_node::info(f);
  if (0 == owner_) return;
  if (status() == STATUS_ABORTED && owner_->get_node()) {
    f << owner_->get_node()->abortedReason() << "\n";
  }
  f << owner_->toString() << "\n";
}

void task_node::update(int oldstatus,int oldtryno,int oldflags)
{
	simple_node::update(oldstatus,oldtryno,oldflags);
	check(oldstatus,oldtryno,oldflags);
}

void task_node::adopt(node* n)
{
	simple_node::adopt(n);
	check(n->status(),n->tryno(),n->flags());
}

void task_node::create()
{
	simple_node::create();
	check(0,0,0);
}

#ifdef FLAG_ISSET
#undef FLAG_ISSET
#endif
#define FLAG_ISSET(flag) (1<<(flag))
inline bool is_late(int f) { return (f & FLAG_ISSET(FLAG_LATE)); } 
inline bool is_zombie(int f) { return (f & FLAG_ISSET(FLAG_ZOMBIE)); }
inline bool is_to_check(int f) { return (f& FLAG_ISSET(FLAG_TO_CHECK));}

void task_node::check(int oldstatus,int oldtryno,int oldflags)
{
  
	if(status() != old_status_ && status() == STATUS_ABORTED)
		serv().aborted(*this);

	if(tryno() > 1 && tryno() != old_tryno_ && (
		status() == STATUS_SUBMITTED || 
		status() == STATUS_ACTIVE))
		serv().restarted(*this);

	if(is_late(flags()) != is_late(old_flags_)) {
		if(is_late(flags()))
                  serv().late(*this);
		else
                  late::hide(*this);
	}

	if(is_zombie(flags()) != is_zombie(old_flags_)) {
		if(is_zombie(flags()))
                  serv().zombie(*this);
		else
                  zombie::hide(*this);
	}

	if(is_to_check(flags()) != is_to_check(old_flags_)) {
		if(is_to_check(flags()))
                  serv().to_check(*this);
		else
                  to_check::hide(*this);
	}
	old_flags_ = flags();
	old_status_ = status();
	old_tryno_ = tryno();
}

void task_node::aborted(std::ostream& f)
{
	if(status() == STATUS_ABORTED)
	{
		f << "task " << this << " is aborted";
		long flg = flags();
		int i = 0;
		while(flg>0)
		{
			if(flg%2) 
			{
				f << " (" << ::ecf_flag_name[i] << ")";
			}
			flg /= 2;
			i++;
		}
		f << "\n";
	}
	simple_node::aborted(f);
}

const char* task_node::html_page(url& u)
{
	return "node.html";
}


void task_node::html_name(FILE* f,url& u)
{
	node::html_name(f,u);
}


class cpp_translator: public url_translator {
	re re_;
	node* n_;
public:
	cpp_translator(node* n);
	~cpp_translator();

	virtual void save(FILE*,const char *line);
};

cpp_translator::cpp_translator(node* n):
	re_("%([^%]+)$0%"),
	n_(n)
{
}

cpp_translator::~cpp_translator()
{
}

void cpp_translator::save(FILE* f,const char *line)
{
	if(strncmp(line,"%manual",7) == 0)
	{
		fprintf(f,"<b>");
		url_translator::save(f,line);
		fprintf(f,"</b>");
		fprintf(f,"<i>");
		return;
	}

	if(strncmp(line,"%end",4) == 0)
	{
		fprintf(f,"</i>");
		fprintf(f,"<b>");
		url_translator::save(f,line);
		fprintf(f,"</b>");
		return;
	}

	if(strncmp(line,"%include",8) == 0)
	{
		fprintf(f,"<b>");
		url_translator::save(f,line);
		fprintf(f,"</b>");
		return;
	}


	char val[1024];
	char buf[1024];

	strcpy(buf,line);
	char *p = buf;
	char *q;

	while((q = re_.match(p,val)))
	{
		char *loc = re_.loc();
		char w = *loc;
		*loc = 0;
		url_translator::save(f,p);
		*loc = w;

		node *n = n_->variableOwner(val);
		if(n == 0) n = n_;

		// TODO fprintf(f,"<b><a href=\"%s:%s\">%%",n->html_url(),val);
		url_translator::save(f,val);
		fprintf(f,"%%</a></b>");
		p = q;
	}

	url_translator::save(f,p);
}

void task_node::html_script(FILE* f,url& u)
{
	cpp_translator cpp(this);
	tmp_file tmp = serv().script(*this);
	u.add(tmp,cpp);
}

void task_node::html_output(FILE* f,url& u)
{
	url_translator t;
	tmp_file tmp = serv().output(*this);
	u.add(tmp,t);
}

void task_node::html_job(FILE* f,url& u)
{
	url_translator t;
	tmp_file tmp = serv().job(*this);
	u.add(tmp,t);
}

void task_node::html_jobstatus(FILE* f,url& u)
{
	url_translator t;
	std::string job    = variable("ECF_JOB");
	std::string stat   = job + ".stat";
	serv().jobstatus(*this, "");
	tmp_file tmp (stat); 
	u.add(tmp,t);
}
