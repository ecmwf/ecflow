## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

KIND=bin/gcc-4.5
CLNT=../Client/bin/gcc-4.5/debug/ecflow_client --port 3199
CLNT=../Client/bin/gcc-4.5/release/ecflow_client --port 3199
CLNT2=../Client/bin/gcc-4.5/debug/ecflow_client
DEFS=
DEFS=
# DEFS=define=PORTABLE_BINARY_ARCHIVE
DIRRC=~/.ecflowrc

all:
	$(BOOST_ROOT)/bjam -j4 variant=debug $(DEFS) ecflowview

fast:
	$(BOOST_ROOT)/bjam variant=debug $(DEFS) ecflowview

rel:
	$(BOOST_ROOT)/bjam -j4 variant=release $(DEFS) ecflowview 

val:
	/tmp/map/work/valgrind-3.7.0/coregrind/valgrind bin/gcc-4.5/release/ecflowview 

vald:
	/tmp/map/work/valgrind-3.7.0/coregrind/valgrind bin/gcc-4.5/debug/ecflowview 

profile:
	$(BOOST_ROOT)/bjam -j4 variant=profile $(DEFS) ecflowview 

prof:   profile $(KIND)/profile/ecflowview 
	date
	- time $(KIND)/profile/ecflowview 
	gprof $(KIND)/profile/ecflowview  gmon.out  > gprof.out
	@cp gprof.out $(SCRATCH)/.	# - kprof # 

# useful when updating the menu from the sources
menuclean:
	rm -rf $(DIRRC)
	mkdir $(DIRRC)

parse:
	p4 edit src/menul.c src/menuy.c 
	cd src && lex  -t menul.l > menul.c && yacc  menuy.y && mv -f y.tab.c menuy.c && cd ../

menu-full: menu parse all rel 

menu:
	p4 edit src/ecflowview.menu.h
	sh src/menu2c.sh  < src/ecflowview.menu > src/ecflowview.menu.h
	cp -f src/ecflowview.menu $(DIRRC)/.

	p4 edit src/xcdp.menu.h
	sh src/menu2c.sh < src/xcdp.menu  > src/xcdp.menu.h
	cp -f src/xcdp.menu $(DIRRC)/.

clean-run: menu all
	rm $(DIRRC)/ecflowview.menu || :
	exec `	find bin/ -name ecflowview`

tags:
	etags ../[ABCv]*/src/*.[hcp]

d:
	time $(KIND)/debug/ecflowview
ddd:
	time ddd $(KIND)/debug/ecflowview

clean:
	-\rm *~ gmon.out
	$(BOOST_ROOT)/bjam clean

calib:
	date; time $(CLNT) --get > /dev/null
	date; time $(CLNT) --get > 3199.tmp
	date; time python tool/timing.py
	date

SUITE=test_suite
load:	
	 # $(CLNT) --replace /$(SUITE) tool/$(SUITE).exp
	 # $(CLNT) --replace /test_ui tool/test.def
	 $(CLNT) --replace /gui tool/test.def

	 $(CLNT2) --host badger --port 31415 --group="get; show" > badger.def
	 $(CLNT) --load badger.def

	 # cd /var/tmp/map/work/p4/metapps/suites/o/def && make
	 # cd /var/tmp/map/work/p4/metapps/suites/o/def && make test

boot:
	 - $(CLNT) --load tool/3199.exp
ping:
	 - $(CLNT) --ping

relbin:
	$(BOOST_ROOT)/bjam -j4 variant=release define=PORTABLE_BINARY_ARCHIVE ecflowview 
