/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#define PANEL_INFO		     0
#define PANEL_MANUAL		 (PANEL_INFO+1)
#define PANEL_SCRIPT		 (PANEL_MANUAL+1)
#define PANEL_JOB		 (PANEL_SCRIPT+1)
#define PANEL_JOBSTATUS		 (PANEL_JOB+1)
#define PANEL_OUTPUT		 (PANEL_JOBSTATUS+1)
#define PANEL_WHY		 (PANEL_OUTPUT+1)
#define PANEL_TRIGGER            (PANEL_WHY+1)
#define PANEL_JOBCHECK           (PANEL_TRIGGER+1)
#define PANEL_TIMETABLE          (PANEL_JOBCHECK+1)
#define PANEL_VARIABLES		 (PANEL_TIMETABLE+1)
#define PANEL_EDIT_TASK		 (PANEL_VARIABLES+1)
#define PANEL_EDIT_LABEL	 (PANEL_EDIT_TASK+1)
#define PANEL_EDIT_LIMIT	 (PANEL_EDIT_LABEL+1)
#define PANEL_EDIT_VARIABLE	 (PANEL_EDIT_LIMIT+1)
#define PANEL_EDIT_METER	 (PANEL_EDIT_VARIABLE+1)
#define PANEL_EDIT_REPEAT	 (PANEL_EDIT_METER+1)
#define PANEL_HISTORY		 (PANEL_EDIT_REPEAT+1)
#define PANEL_MESSAGES	  	 (PANEL_HISTORY+1)
#define PANEL_SUITES             (PANEL_MESSAGES+1)
#define PANEL_USERS		 (PANEL_SUITES+1)
#define PANEL_ZOMBIES		 (PANEL_USERS+1)
#define PANEL_ECF_OPTIONS 	 (PANEL_ZOMBIES+1)

#define PANEL_MAX_FACTORIES      (PANEL_ECF_OPTIONS+1)
