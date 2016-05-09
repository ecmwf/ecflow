/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #3 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#if defined(linux) || defined(alpha)
#include <regex.h>


class re {
	regex_t re_;
	char *loc_;
public:
	re(const char* r): loc_(0) { regcomp(&re_,r,0); }

	~re() { regfree(&re_); }

	char* match(char* a,char* b)
	{
		regmatch_t   pmatch;
		if(!regexec(&re_,a,1,&pmatch,0))
			return 0;

		loc_ = a+pmatch.rm_so;
		return a+pmatch.rm_eo;
	}

	char* loc() { return loc_; }
};

#else
#include <stdlib.h>
#include <libgen.h>

class re {
	char *re_;
public:
	re(const char* r) : re_(::regcmp(r,0)) {}
	~re() { ::free(re_); }
	char* match(char* a,char* b)  { return ::regex(a,b); }
	char* loc() { return ::__loc1; }
};

#endif
