#ifndef ecflowview_h
#define ecflowview_h
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #13 $                                                                    */
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


#ifdef hpux
// hpux X11 headers are wrong
#undef bcopy
#undef bzero
#undef bcmp
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ecflow.h"

#define NOTIMP throw "not_implemented"
#define XECF_SNAP_ENABLED TRUE

#ifdef BRIDGE 
/* #define appName "XCdp - ecFlowview" */
  #define appName "XCdp"
#else
#define appName "ecFlowview"
#endif

#define clientName "ecflow_client"
#define snapshotName "${TMPDIR:=/tmp}/ecflowview$USER.png"
#define browserName "${BROWSER:firefox --new-tab}"
#define urlRef "http://software.ecmwf.int/issues/browse/ECFLOW"
#define tmpName   "ecFlowvw" /* 8 char */

#if 1 // DEBUG
#define XECFDEBUG if(getenv("XECFLOW_DEBUG"))
#else
#define XECFDEBUG if (0)
#endif
#endif
