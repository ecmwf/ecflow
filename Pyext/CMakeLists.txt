### ecflow python bindings
# NOTES:
#
# To see the python link line: Do python-config  --ldflags, i.e
#   > /usr/local/apps/python/2.7.8-01/bin/python-config  --ldflags
#   > -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
#
# However on cct we get:
#   > /usr/local/apps/python/2.7.5-01/bin/python-config --ldflags
#   > -L/usr/local/apps/python/2.7.5-01/lib/python2.7/config -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
#
# cct is correct as it has the "-L" but on other machines we rely on /usr/lib/libpython2.7.so.1.0 being there!
# lxop-test does not have /usr/lib/libpython2.7.so.1.0 so ecbuild fails
#
# For problems with the python build, please look in <build-dir>/ecbuild.log, typically non developmental python
# installs may not include python libs or includes
# For ubuntoo to install we need:
#    sudo apt-get install libpython-dev
#

# =============================================================================
# Configure __init__.py to add __version__
# =============================================================================
set(INIT_PY_IN  "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py.in")
set(INIT_PY_OUT "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py")
configure_file(${INIT_PY_IN} ${INIT_PY_OUT} )


# ==============================================================================
# local includes
# ==============================================================================
include_directories(  
                     ../ACore/src 
                     ../ANattr/src
                     ../ANode/src
                     ../Base/src
                     ../Base/src/cts
                     ../Base/src/stc
                     ../Client/src
                     ../CSim/src
                    )

message( STATUS " CMAKE_VERSION                      : ${CMAKE_VERSION}, need cmake version >= 3.12.0 to build ecflow python3 extension" )
if(${CMAKE_VERSION} VERSION_LESS "3.12.0") 
	# We only support python2 extension for cmake less 3.12.0
	# cmake 3.12.0 or greater allows multiple boost python libs & hence multiple extensions to be built
	# Using -DPYTHON_EXECUTABLE=/usr/local/apps/python3/3.6.5-01/bin/python3 is not suffcient as the wrong(python2) libs are found
	ecbuild_find_python( VERSION 2.6 REQUIRED )  
	message( STATUS "  PYTHON_FOUND                      : ${PYTHON_FOUND}" )
    message( STATUS "  PYTHONINTERP_FOUND                : ${PYTHONINTERP_FOUND}" )
    message( STATUS "  PYTHONLIBS_FOUND                  : ${PYTHONLIBS_FOUND}" )
    message( STATUS "  PYTHON_VERSION_STRING             : ${PYTHON_VERSION_STRING}" )
    message( STATUS "  PYTHON_VERSION_MAJOR              : ${PYTHON_VERSION_MAJOR}" )
    message( STATUS "  PYTHON_VERSION_MINOR              : ${PYTHON_VERSION_MINOR}" )
    message( STATUS "  PYTHON_VERSION_PATCH              : ${PYTHON_VERSION_PATCH}" )
    message( STATUS "  PYTHON_CONFIG_EXECUTABLE          : ${PYTHON_CONFIG_EXECUTABLE}" )
    message( STATUS "  PYTHON_EXECUTABLE                 : ${PYTHON_EXECUTABLE}" )
    message( STATUS "  PYTHON_INCLUDE_DIRS               : ${PYTHON_INCLUDE_DIRS}" )
    message( STATUS "  PYTHON_LIBRARIES                  : ${PYTHON_LIBRARIES}" )
	if (${PYTHON_VERSION_MAJOR} EQUAL 3)
		ecbuild_error("Need cmake version >= 3.12.0 to build ecflow python3 extension, current cmake version is ${CMAKE_VERSION}")
	endif()
	find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS python )
	add_subdirectory( python2 )
else()
	# ======================================================================================
	# Attempt to build both python2 and python3 ecflow extension, depending on what is found
	# this assumes cmake 3.12.0 min, which added support for boost python2 and python3
	# ======================================================================================
	find_package(Python2 COMPONENTS Interpreter Development)
	message( STATUS "   Python2_FOUND                     : ${Python2_FOUND}" )
	if (Python2_FOUND)
	    if ( NOT Python2_LIBRARIES )             
	        ecbuild_error("ecflow python extension is enabled, but python2 libraries not found")
	    endif()
	
	    # *****************************************************************************************
	    # Although we have found python2 it could be that *ONLY* python3 was module loaded
	    #    Otherwise will build for python2 and test with python3 interpreter
	    # *****************************************************************************************
	    ecbuild_find_python( VERSION 2.6 REQUIRED ) # if not included iterpreter not found ?
	    if ( ${PYTHON_VERSION_MAJOR} EQUAL 2)
			message( STATUS "   Python2_Interpreter_FOUND         : ${Python2_Interpreter_FOUND}" )
			message( STATUS "   Python2_EXECUTABLE                : ${Python2_EXECUTABLE}" )
			message( STATUS "   Python2_STDLIB                    : ${Python2_STDLIB}    Standard platform independent installation directory" )
			message( STATUS "   Python2_STDARCH                   : ${Python2_STDARCH}   Standard platform dependent installation directory." )
			message( STATUS "   Python2_Development_FOUND         : ${Python2_Development_FOUND}" )
			message( STATUS "   Python2_INCLUDE_DIRS              : ${Python2_INCLUDE_DIRS}" )
			message( STATUS "   Python2_LIBRARIES                 : ${Python2_LIBRARIES}" )
			message( STATUS "   Python2_LIBRARY_DIRS              : ${Python2_LIBRARY_DIRS}" )
			message( STATUS "   Python2_VERSION                   : ${Python2_VERSION}" )
			message( STATUS "   Python2_VERSION_MAJOR             : ${Python2_VERSION_MAJOR}" )
			message( STATUS "   Python2_VERSION_MINOR             : ${Python2_VERSION_MINOR}" )
			message( STATUS "   Python2_VERSION_PATCH             : ${Python2_VERSION_PATCH}" )

	    	message( STATUS "   PYTHON_FOUND                      : ${PYTHON_FOUND}" )
	    	message( STATUS "   PYTHONINTERP_FOUND                : ${PYTHONINTERP_FOUND}" )
	   		message( STATUS "   PYTHONLIBS_FOUND                  : ${PYTHONLIBS_FOUND}" )
	    	message( STATUS "   PYTHON_VERSION_STRING             : ${PYTHON_VERSION_STRING}" )
	    	message( STATUS "   PYTHON_VERSION_MAJOR              : ${PYTHON_VERSION_MAJOR}" )
	    	message( STATUS "   PYTHON_VERSION_MINOR              : ${PYTHON_VERSION_MINOR}" )
	    	message( STATUS "   PYTHON_VERSION_PATCH              : ${PYTHON_VERSION_PATCH}" )
	    	message( STATUS "   PYTHON_CONFIG_EXECUTABLE          : ${PYTHON_CONFIG_EXECUTABLE}" )
	    	message( STATUS "   PYTHON_EXECUTABLE                 : ${PYTHON_EXECUTABLE}" )
	    	message( STATUS "   PYTHON_INCLUDE_DIRS               : ${PYTHON_INCLUDE_DIRS}" )
	    	message( STATUS "   PYTHON_LIBRARIES                  : ${PYTHON_LIBRARIES}" )
	    	
	    	find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS python )
			add_subdirectory( python2 )
		endif()
	endif()

	find_package(Python3 COMPONENTS Interpreter Development)
	message( STATUS "   Python3_FOUND                     : ${Python3_FOUND}" )
	if (Python3_FOUND)
		    	
	    find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS python3 )
	    if (Boost_FOUND) 
			message( STATUS "   Python3_Interpreter_FOUND         : ${Python3_Interpreter_FOUND}" )
			message( STATUS "   Python3_EXECUTABLE                : ${Python3_EXECUTABLE}" )
			message( STATUS "   Python3_STDLIB                    : ${Python3_STDLIB}    Standard platform independent installation directory" )
			message( STATUS "   Python3_STDARCH                   : ${Python3_STDARCH}   Standard platform dependent installation directory." )
			message( STATUS "   Python3_Development_FOUND         : ${Python3_Development_FOUND}" )
			message( STATUS "   Python3_INCLUDE_DIRS              : ${Python3_INCLUDE_DIRS}" )
			message( STATUS "   Python3_LIBRARIES                 : ${Python3_LIBRARIES}" )
			message( STATUS "   Python3_LIBRARY_DIRS              : ${Python3_LIBRARY_DIRS}" )
			message( STATUS "   Python3_VERSION                   : ${Python3_VERSION}" )
			message( STATUS "   Python3_VERSION_MAJOR             : ${Python3_VERSION_MAJOR}" )
			message( STATUS "   Python3_VERSION_MINOR             : ${Python3_VERSION_MINOR}" )
			message( STATUS "   Python3_VERSION_PATCH             : ${Python3_VERSION_PATCH}" )
		
	    	if ( NOT Python3_LIBRARIES  )             
	        	ecbuild_error("ecflow python extension is enabled, but python3 libraries not found")
	    	endif()
	    
	    	ecbuild_find_python( VERSION ${Python3_VERSION} REQUIRED )
			add_subdirectory( python3 )
		else()
			message( STATUS "   Boost python3 libraries *NOT* found, ignoring" )
		endif()
	endif()
endif()
