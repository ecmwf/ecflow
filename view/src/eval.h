/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #3 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

	virtual Boolean is_MENU() { return 0;}
	virtual Boolean is_IDENT() { return 0;}
	virtual Boolean is_NONE() { return 0;}
	virtual Boolean is_ALL() { return 1;}
	virtual Boolean is_UNKNOWN() { return 0;}
	virtual Boolean is_SUSPENDED() { return 0;}
	virtual Boolean is_COMPLETE() { return 0;}
	virtual Boolean is_QUEUED() { return 0;}
	virtual Boolean is_SUBMITTED() { return 0;}
	virtual Boolean is_ACTIVE() { return 0;}
	virtual Boolean is_ABORTED() { return 0;}
	virtual Boolean is_CLEAR() { return 0;}
	virtual Boolean is_SET() { return 0;}
	virtual Boolean is_SHUTDOWN() { return 0;}
	virtual Boolean is_HALTED() { return 0;}
	virtual Boolean is_ECF() { return 0;}
	virtual Boolean is_SUITE() { return 0;}
	virtual Boolean is_FAMILY() { return 0;}
	virtual Boolean is_TASK() { return 0;}
	virtual Boolean is_EVENT() { return 0;}
	virtual Boolean is_LABEL() { return 0;}
	virtual Boolean is_METER() { return 0;}
	virtual Boolean is_REPEAT() { return 0;}
	virtual Boolean is_VARIABLE() { return 0;}
	virtual Boolean is_TRIGGER() { return 0;}
	virtual Boolean is_HAS_TRIGGERS() { return 0;}
	virtual Boolean is_HAS_TIME() { return 0;}
	virtual Boolean is_HAS_DATE() { return 0;}
	virtual Boolean is_SEPARATOR() { return 0;}
	virtual Boolean is_STRING() { return 0;}
	virtual Boolean is_DEFAULT_YES() { return 0;}
	virtual Boolean is_DEFAULT_NO() { return 0;}
	virtual Boolean is_EDIT() { return 0;}
	virtual Boolean is_OUTPUT() { return 0;}
