#ifndef ecf_H
#define ecf_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #19 $                                                                    */
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
#include "std.h"
#include <sstream>
#include <extent.h>

#include "Str.hpp"
#include <inttypes.h>

#define ECF_PROG 3141

class ecf_dir : public extent<ecf_dir> {
 protected:
  bool sort() { return true; }
  ecf_dir(const ecf_dir&);
  ecf_dir operator=(const ecf_dir&);
 public:
  char *name_;
  ecf_dir *next;

  int mode;
  int uid;
  int gid;
  int size;
  int atime;
  int mtime;
  int ctime;

 ecf_dir() : name_ (0x0), next (0x0) {}
 virtual ~ecf_dir() {}

 std::string name() const { return (name_); }
};

template<class T>
bool ecf_list_add(T **list, T *kid)
{
  T **top = list; 
  T  *run = *top;
  if ( !kid ) return false;
  kid->next = NULL;
  if ( run ) {
    while( run->next ) run = run->next;
    run->next = kid;
  } else
    *top = kid;
  return true;
};

int ecf_nick_update(const std::string& name, 
		    const std::string& machine, 
		    int port);
int ecf_nick_write();
int ecf_nick_delete(const std::string& name);

class ecf_list {
 public:
  int       type_;
  char     *name_;  
  ecf_list *next;

  ecf_list(const char *text,
	  ecf_list *n= 0x0) : name_(0x0), next (n) { name_ = strdup(text); }
  virtual ~ecf_list() { if (name_) free(name_); }

  std::string name() const { return name_; }
 private:
  ecf_list(ecf_list& );
};

class ecf_map {
  std::string  name_;
  std::string  machine_;
  int          port_;

public:
  enum kind {GLOBAL, USER, NETWORK};
  kind from;

  ecf_map (const std::string n, const std::string m, int p, kind f)
    : name_(n)
    , machine_(m)
    , port_(p)
    , from(f)
  {}

  ~ecf_map() {}

  std::string name() const { return name_; }
  std::string machine() const { return machine_; }
  int port() const { return port_; }

  bool operator== (const std::string& n) const { return name_ == n; }

  bool operator== (const ecf_map& l) const;

  const std::string print() const;
};

int ecf_nick_load();
ecf_list *ecf_node_create(char *text);
int ecf_nick_origin(const std::string& name);

#include <Suite.hpp>
#include <Family.hpp>
#include <Task.hpp>
#include <ClientInvoker.hpp>
#include <AbstractObserver.hpp>
#include "NodeFwd.hpp"

#include "ExprAst.hpp"
#include "TimeAttr.hpp"
#include "TodayAttr.hpp"
#include "NodeAttr.hpp"
#include "DateAttr.hpp"
#include "DayAttr.hpp"
#include "CronAttr.hpp"
#include "LateAttr.hpp"
#include "ZombieAttr.hpp"
#include "RepeatAttr.hpp"
#include "AutoCancelAttr.hpp"

#include "Node.hpp"
#include "Defs.hpp"

typedef uint64_t uint64;
#include <boost/algorithm/string.hpp>

char    *ecf_string(char *str, char *file, int lineno);
ecf_dir *ecf_file_dir(char *path, char *pattern, int fullname, ecf_dir *dir = NULL);

#endif

#ifndef ECF_NO_DUPLICATE
#define SMS_PROG 314159

#ifndef NODE_COMPLETE
#define STR(x) ecf_string((x), __FILE__ , __LINE__ )

#define STATUS_RESUME -1
#define STATUS_UNKNOWN 0
#define STATUS_SUSPENDED 1
#define STATUS_COMPLETE 2
#define STATUS_QUEUED 3
#define STATUS_SUBMITTED 4
#define STATUS_ACTIVE 5
#define STATUS_ABORTED 6
#define STATUS_USABLE 7
#define STATUS_SHUTDOWN 7
#define STATUS_HALTED 8
#define STATUS_MAX 9

#define FLAG_FORCE_ABORT 0
#define FLAG_USER_EDIT 1
#define FLAG_TASK_ABORTED 2
#define FLAG_EDIT_FAILED 3
#define FLAG_CMD_FAILED 4
#define FLAG_NO_SCRIPT 5
#define FLAG_KILLED 6
#define FLAG_MIGRATED 7
#define FLAG_LATE 8
#define FLAG_MESSAGE 9
#define FLAG_BYRULE 10
#define FLAG_QUEUELIMIT 11
#define FLAG_WAIT 12
#define FLAG_LOCKED 13
#define FLAG_ZOMBIE 14
#define FLAG_TO_CHECK 15
#define FLAG_MAX 16

#define ZOMBIE_USER 1
#define ZOMBIE_NET 2
#define ZOMBIE_GET 0
#define ZOMBIE_FOB 1
#define ZOMBIE_DELETE 2
#define ZOMBIE_FAIL 3
#define ZOMBIE_RESCUE 4
#define ZOMBIE_KILL 5

#define ECF_E_NOTIN        26
#define ECF_E_IN           27
#define ECF_E_NONEWS       28      /* Not really an error */
#define ECF_E_HOST         29

#define NODE_LIST 0
#define NODE_USER 1
#define NODE_CONNECTION 2
#define NODE_VARIABLE 3
#define NODE_TIME 4
#define NODE_DATE 5
#define NODE_TRIGGER 6
#define NODE_TREE 7
#define NODE_COMPLETE 30
#define NODE_EVENT 9
#define NODE_TASK 10
#define NODE_FAMILY 11
#define NODE_SUITE 12
#define NODE_SUPER 13
#define NODE_REPEAT 22
#define NODE_DIR    23
#define NODE_METER 24
#define NODE_LABEL 25
#define NODE_LATE 28
#define NODE_UNKNOWN 35
#define NODE_ALIAS 32
#define NODE_LIMIT 33
#define NODE_INLIMIT 34
#define NODE_UNKNOWN 35

#define NODE_REPEAT_E 36
#define NODE_REPEAT_S 37
#define NODE_REPEAT_D 38
#define NODE_REPEAT_I 39
#define NODE_REPEAT_DAY 40

#define NODE_MAX 41

#define SUITES_LIST 3
#define SUITES_MINE 4
#define SUITES_REG  7

#ifndef NIL
#define NIL -1
#endif

#ifndef MIN
#define MIN(a,b) ((a)<=(b)?(a):(b))
#endif
#endif /* do not overwrite sms.h */

long ecf_repeat_julian_to_date(long jdate);
long ecf_repeat_date_to_julian(long ddate);
#endif /* ECF_NO_DUPLICATE */
#endif /* ecf_H */
