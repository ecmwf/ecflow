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

#include <stdio.h>

main(int argc,char **argv)
{
	FILE* f = argc>2 ? fopen(argv[2],"r"):stdin;
	int l = atol(argv[1]);
	int n = 0;

	char line[1024];

	if(!f)
	{
		perror(argv[2]);
		exit(1);
	}

	while(fgets(line,sizeof(line),f))
		if(++n == l)
		{ 
			printf("%s\n",line);
			exit(0);
		}


	exit(1);

}
