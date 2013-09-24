#!/usr/bin/env python2.7
import ecflow 
           
def create_suite(name) : 
    suite = ecflow.Suite(name)
    for i in range(1, 7) :
        fam = suite.add_family("f" + str(i))
        for t in ( "a", "b", "c", "d", "e" ) :
            fam.add_task(t)
    return suite     
 
def create_seqeuntial_suite(name) :
    suite = ecflow.Suite(name)
    for i in range(1, 7) :
        fam = suite.add_family("f" + str(i))
        if i != 1: 
            fam.add_trigger("f" + str(i-1) + " == complete")  #  or fam.add_family( "f%d == complete" % (i-1) )
        for t in ( "a", "b", "c", "d", "e" ) :
            fam.add_task(t) 
    return suite
 

if __name__ == "__main__":
    print "Creating suite definition"   
    print create_suite("test")   
    print create_seqeuntial_suite("test")