all:  ecf view

ecf:
	echo . ./view/tool/env.sh 
	$(BOOST_ROOT)/bjam -j4
rel:
	$(BOOST_ROOT)/bjam -j4 variant=release

view:
	(cd view; $(BOOST_ROOT)/bjam -j4)

clean:
	$(BOOST_ROOT)/bjam clean;
	(cd view; $(BOOST_ROOT)/bjam clean)

realclean:
	rm -rf */bin Pyext/ecflow/ecflow.so

tar:
	cd ../..; tar -cjvf $SCRATCH/ecf.tbz workspace


