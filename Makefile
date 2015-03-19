# This is used as part of install build/install for cmake/ecbuild

default: release

release:
	(cd ../cmake_build_dir/ecflow/release; make -j4)
	
install:
	(cd ../cmake_build_dir/ecflow/release; make install)
