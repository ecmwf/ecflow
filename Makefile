BJAM=$(BOOST_ROOT)/bjam c++-template-depth=512

all:  ecf view

ecf:
	echo . ./view/tool/env.sh 
	$(BJAM) -j4
rel:
	$(BJAM) -j4 variant=release

view:
	(cd view; $(BJAM) -j4)

clean:
	$(BJAM) clean;
	(cd view; $(BJAM) clean)

realclean:
	rm -rf */bin Pyext/ecflow/ecflow.so

tar:
	cd ../..; tar -cjvf $SCRATCH/ecf.tbz workspace


