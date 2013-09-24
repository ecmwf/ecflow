#ifndef STL_HPP_
#define STL_HPP_
//============================================================================
// Name        : stl
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

/*****************************************************************************
 * Standard Library Include Files                                            *
 *****************************************************************************/
#include <algorithm> //for for_each()
//#include <iostream>


namespace ecf
{
	/// Helper struct that will aid the deletion of Pointer from a container
	template <typename T> struct TSeqDeletor {
		void operator () (T pointer) {
			// std::cout << "Destroy of this pointer" << std::endl;
			delete pointer;
			pointer = 0;
		}
	};
	/// This function can be used to delete the pointers in a container
	/// i.e int main (int argc, char **argv) {
	///        std::vector <std::string *> vect;
	///        vect.push_back (new std::string ("Stephane"));
	///        DeletePtrs (vect);
	///     }
	template <typename Container> void DeletePtrs (Container& pContainer) {
		std::for_each( pContainer.begin (),
					   pContainer.end (),
					   TSeqDeletor<typename Container::value_type> ());
		pContainer.clear();
	}


	/// Helper struct that will aid the deletion of Pointer from a Associative container
	template <typename TPair> struct TAsoDeletor {
    	void operator () (TPair& tElem)  {
    		if(tElem.second) {
        		delete tElem.second;
    		}
    	}
	};
	/// This function can be used to delete the pointers in a Assoc container
	/// i.e int main (int argc, char **argv) {
	///        std::map <int,std::string *> theMap;
	///        theMap[0] =  new std::string ("Stephane");
	///        AssoDeletePtrs(theMap);
	///     }
	template <typename Container> void AssoDeletePtrs (Container& pContainer) {
		std::for_each( pContainer.begin (),
					   pContainer.end (),
					   TAsoDeletor<typename Container::value_type> ());
		pContainer.clear();
	}
}

#endif /* STL_HPP_ */
