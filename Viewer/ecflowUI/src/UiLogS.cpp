//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "UiLogS.hpp"

#include "ServerHandler.hpp"

//---------------------------------
// UiFunctionLog
//---------------------------------

UiFunctionLogS::UiFunctionLogS(ServerHandler* server,const std::string& funcName) :
    UiFunctionLog(((server)?server->longName():""),funcName)
{
}

//---------------------------------
// UiLog
//---------------------------------

UiLogS::UiLogS(ServerHandler* server) : UiLog((server)?server->longName():"")
{
}
