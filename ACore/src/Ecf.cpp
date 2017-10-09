//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Provides globals used by server
//============================================================================

#include "Ecf.hpp"

bool Ecf::server_ =  false;
bool Ecf::debug_equality_ = false;
unsigned int Ecf::debug_level_ = 0;
unsigned int Ecf::state_change_no_ = 0;
unsigned int Ecf::modify_change_no_ = 0;
bool DebugEquality::ignore_server_variables_ = false;


const char* Ecf::SERVER_NAME() { static const char* SERVER_NAME = "ecflow_server"; return SERVER_NAME;}
const char* Ecf::CLIENT_NAME() { static const char* CLIENT_NAME = "ecflow_client"; return CLIENT_NAME;}

const std::string& Ecf::LOG_FILE() { static const std::string LOG_FILE = "ecf.log"; return LOG_FILE;}
const std::string& Ecf::CHECKPT() { static const std::string CHECKPT= "ecf.check"; return CHECKPT;}
const std::string& Ecf::BACKUP_CHECKPT() { static const std::string BACKUP_CHECKPT= "ecf.check.b";return BACKUP_CHECKPT;}
const std::string& Ecf::MICRO() { static const std::string MICRO= "%";return MICRO;}
const std::string& Ecf::JOB_CMD() { static const std::string JOB_CMD= "%ECF_JOB% 1> %ECF_JOBOUT% 2>&1";return JOB_CMD;}
const std::string& Ecf::KILL_CMD() { static const std::string KILL_CMD= "kill -15 %ECF_RID%";return KILL_CMD;}     // "${ECF_KILL:=/home/ma/emos/bin/ecfkill}   %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1";
const std::string& Ecf::STATUS_CMD() { static const std::string STATUS_CMD= "ps --sid %ECF_RID% -f";return STATUS_CMD;}// "${ECF_STAT:=/home/ma/emos/bin/ecfstatus} %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1";
const std::string& Ecf::URL_CMD() { static const std::string URL_CMD= "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'";return URL_CMD;}
const std::string& Ecf::URL_BASE() { static const std::string URL_BASE= "https://software.ecmwf.int";return URL_BASE;}
const std::string& Ecf::URL() { static const std::string URL = "wiki/display/ECFLOW/Home";return URL;}


unsigned int Ecf::incr_state_change_no() {
	if ( server_ ) {
		return ++state_change_no_;
	}
	return state_change_no_;
}

unsigned int Ecf::incr_modify_change_no() {

	if ( server_ ) {
		return ++modify_change_no_;
	}
	return modify_change_no_;
}

// =======================================================

EcfPreserveChangeNo::EcfPreserveChangeNo()
: state_change_no_(Ecf::state_change_no()),
  modify_change_no_(Ecf::modify_change_no())
{}

EcfPreserveChangeNo::~EcfPreserveChangeNo()
{
	Ecf::set_state_change_no(state_change_no_);
	Ecf::set_modify_change_no(modify_change_no_);
}

