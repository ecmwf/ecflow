//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "url.h"
#include "re.h"
#include "tmp_file.h"
#include "str.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

url::url(int soc):
	soc_(soc),
	code_(200),
	in_(fdopen(soc_,"r")),
	out_(fdopen(soc_,"w")),
	tmp_(tmpfile())
{
	char line[1024];
	method_[0] = what_[0] = 0;

	while(fgets(line,sizeof(line),in_))
	{
		if(method_[0] == 0)
			sscanf(line,"%s %s",method_,what_);
		printf("url->%s<-",line);
		if(strlen(line) == 2) break;
	}

	char *p = what_;
	char *s = what_;

	while(*s)
	{
		if(*s == '+')
		{
			*p =  ' ';	
		} 
		else if(*s == '%')
		{
			char h = s[1]; if(!h) break;
			char l = s[2]; if(!l) break;

			unsigned int a = (h>='A')?(h-'A'+10):(h-'0');
			unsigned int b = (l>='A')?(l-'A'+10):(l-'0');

			*p =   char(a * 16 + b);
			s += 2;
		}
		else *p = *s;

		p++;
		s++;
	}
	*p = 0;
}

void url::process(node* n)
{
  if (! node::is_json) {

	fprintf(out_,"\nHTTP/1.0 %d Document follows\r\n",code_);
	fprintf(out_,"MIME-Version: 1.0\r\n");
	fprintf(out_,"Content-Type: text/html\r\n");
	fprintf(out_,"\r\n");

	  if(n) {
	    n->as_perl(out_,true);
	  } else 
	    fprintf(out_,"bless({},'ecflow::node::error')");
  } else {
	  if (n) {
	    n->as_perl(out_,true);	    
	  } else
	    fprintf(out_,"{ }");
	}

	fflush(out_);
}

url::~url()
{

	fflush(tmp_);
	long len = ftell(tmp_);
	rewind(tmp_);

	if (!node::is_json) {
	  fprintf(out_,"\n");
	  fprintf(out_,"HTTP/1.0 %d Document follows\r\n",code_);
	  fprintf(out_,"MIME-Version: 1.0\r\n");
	  fprintf(out_,"Content-Type: text/html\r\n");
	  fprintf(out_,"Content-Length: %ld\r\n",len);
	  fprintf(out_,"\r\n");
	}
	copy(tmp_,out_);

	fflush(out_);

	if(in_)  fclose(in_);
	if(out_) fclose(out_);
	if(tmp_) fclose(tmp_);
}


void url::add(tmp_file& t,text_translator& trans)
{
	FILE* f = fopen(t.c_str(),"r");
	if(f) 
	{
		char line[1024];
		while(fgets(line,sizeof(line),f))
			trans.save(tmp_,line);
		fclose(f);
	}
	else {
		fprintf(tmp_,"Cannot open %s\n",t.c_str());
	}
}

void url::add(tmp_file& t)
{
	url_translator trans;
	add(t,trans);
}

void url::copy(FILE* in,FILE* out)
{
	char line[1024];
	long len;

	while( (len = fread(line,1,sizeof(line),in)) > 0)
		fwrite(line,1,len,out);
}

void url_translator::save(FILE* f,const char* p)
{
	while(*p)
	{
		switch(*p)
		{
			/* case '\n': fprintf(f,"<br>\n"); break; */

			case '<': fprintf(f,"&lt;"); break;
			case '>': fprintf(f,"&gt;"); break;
			case '&': fprintf(f,"&amp;"); break;

			default: fputc(*p,f); break;
		}

		p++;

	}
}

class scan_translator: public text_translator {
	re  re_;
	node* n_;
	url&  u_;
public:
	scan_translator(node* n,url& u);
	~scan_translator();

	virtual void save(FILE*,const char *line);
};

scan_translator::scan_translator(node* n,url& u):
	re_("<!-- wcdp ([^ ]*)$0 -->"),
	n_(n),
	u_(u)
{
}

scan_translator::~scan_translator()
{
}

void scan_translator::save(FILE* f,const char *line)
{
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
		fprintf(f,"%s",buf);
		*loc = w;

		// We need a factory here

		if(strcmp(val,"title") == 0)  n_->html_title(u_,u_);
		if(strcmp(val,"kids") == 0)   n_->html_kids(u_,u_);
		if(strcmp(val,"output") == 0) n_->html_output(u_,u_);
		if(strcmp(val,"script") == 0) n_->html_script(u_,u_);
		if(strcmp(val,"name") == 0)   n_->html_name(u_,u_);
		if(strcmp(val,"why") == 0)    n_->html_why(u_,u_);


		p = q;
	}
	fprintf(f,"%s",p);
}

void url::scan(node* n)
{
	tmp_file page(n->html_page(*this),false);
	scan_translator s(n,*this);
	add(page,s);
}


/*
perl -e'print q|{"foo":"XX","bar":1234567890000000000000000}|' |\
              json_pp -f json -t dumper -json_opt pretty,utf8,allow_bignum

wget http://127.0.0.1:8081/lhost/elaw_37r3
cat elaw_37r3 | grep -v -E '(HTTP|MIME|Content-)' | json_pp -f eval -t json -json_opt pretty,utf8,allow_bignum

/tmp/map/work/PythonWeb.org/examples/ecflow

*/
