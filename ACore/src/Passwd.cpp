//============================================================================
// Name        : Passwd.hpp
// Author      : Avi
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "Passwd.hpp"
#include <iostream>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

//extern char *crypt(const char *key, const char *salt);


double ecf_drand48();


std::string Passwd::generate()
{
 	char pw[9];
	for (int i = 0; i < 8; i++) { /* generate a random password */

		pw[i] = 64.0 * ecf_drand48() + '.'; /* Just crack this one! */
		if ( pw[i] > '9' ) pw[i] += 7;
		if ( pw[i] > 'Z' ) pw[i] += 6;
	}
	pw[8] = '\0';
	return std::string (pw);
}

//std::string Passwd::doCrypt( const std::string& passwd ) {
//	/**************************************************************************
//	 ?  Crypt the passwd.
//	 =  Crypted password in static area.
//	 ************************************o*************************************/
//	char salt[3];
//
//	for (int i = 0; i < 2; i++) {
//		salt[i] = 64.0 * ecf_drand48() + '.';
//		if ( salt[i] > '9' ) salt[i] += 7;
//		if ( salt[i] > 'Z' ) salt[i] += 6;
//	}
//	salt[2] = '\0';
//
//	return std::string( crypt( passwd.c_str(), salt ));
//}


double ecf_drand48()
/**************************************************************************
 ?  Random number with time dependent seed.
 =  [0.0 - 1.0)
 ~  drand48(3) srand48(3) rand(3)
 ************************************o*************************************/
{
	//  extern double drand48();
	//  extern void   srand48();
	static int been_here;

	if ( !been_here ) {
#if defined(RAND_ONLY)
		srand( (int) time(NULL) + getpid() );
#else
		srand48( (long) time( NULL ) + getpid() );
#endif
		been_here = 1;
	}

#if defined(RAND_ONLY)
	return (rand()&0xffff) / 65535.0001;
#else
	return drand48();
#endif
}
