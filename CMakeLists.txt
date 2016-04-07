############################################################################################
# cmake options:
#
#       -DCMAKE_BUILD_TYPE=Debug|RelWithDebInfo|Release|Production
#       -DCMAKE_INSTALL_PREFIX=/path/to/install
#
#       -DCMAKE_MODULE_PATH=/path/to/ecbuild/cmake
#
#       -DCMAKE_C_COMPILER=gcc
#       -DCMAKE_CXX_COMPILER=g++
#
#       -DCMAKE_PREFIX_PATH=/path/to/jasper:/path/to/any/package/out/of/place
#       -DBUILD_SHARED_LIBS=OFF
# =========================================================================================
# Usage instructions:
#
# cd $WK
# release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
# major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
# minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
#
##############################################################################

cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

# ===========================================================================
# Change the default if NO CMAKE_BUILD_TYPE specified
if (NOT DEFINED CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Configuration type" FORCE)
endif()


project( ecflow CXX )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild/cmake")
# message( STATUS "CMAKE_MODULE_PATH    : ${CMAKE_MODULE_PATH}")

include( ecbuild_system )

ecbuild_requires_macro_version( 1.6 )


###############################################################################
# local project

ecbuild_declare_project()

# =========================================================================================
# VERSION  
# Get the ecflow version specified in 'VERSION.cmake'. This is only accessible after ecbuild_declare_project()
# The ecflow version config is done in ACore directory
# =========================================================================================

message( STATUS "CMAKE_INSTALL_PREFIX : ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "ECFLOW_RELEASE       : ${ECFLOW_RELEASE}" )
message( STATUS "ECFLOW_MAJOR         : ${ECFLOW_MAJOR}" )
message( STATUS "ECFLOW_MINOR         : ${ECFLOW_MINOR}" )
message( STATUS "ECFLOW_VERSION       : ${ECFLOW_VERSION_STR}" )

# =========================================================================================
# Options , be aware of caching, when modifying on the command line. 
# Ideally start fresh or remove cache CmakeCache.txt in build dir
# =========================================================================================
option( ENABLE_PYTHON         "enable python interface"   ON  )
option( ENABLE_GUI            "Build ecflowview"          ON  )
option( ENABLE_QT5            "enable Qt5"                OFF )
option( ENABLE_ALL_TESTS      "This includes performance/migration/regression tests" OFF )
option( ENABLE_UI_BACKTRACE   "Print a UI debug backtrace" OFF ) 

# Qt4 and Qt5 require different packages to be found, so we need to know
# in advance which version to search for (user-specified so that it
# works on systems with both installed)
if(ENABLE_QT5)
    list(APPEND QT_DEPS "Qt5Widgets REQUIRED 5.0.0")
    list(APPEND QT_DEPS "Qt5Gui REQUIRED 5.0.0")
    list(APPEND QT_DEPS "Qt5Network REQUIRED 5.0.0")
    list(APPEND QT_DEPS "Qt5Multimedia REQUIRED 5.0.0")
else()
    set(QT_DEPS "Qt4 4.8.0 REQUIRED QtCore QtGui QtNetwork" )
endif()

ecbuild_add_option( FEATURE VIEWER
                    DEFAULT ON
                    DESCRIPTION       "the Qt version of the viewer"
                    REQUIRED_PACKAGES "${QT_DEPS}" )


# sanity check - cannot set ENABLE_UI_BACKTRACE if VIEWER is OFF
if(ENABLE_UI_BACKTRACE AND (NOT ENABLE_VIEWER))
    message(WARNING "Cannot ENABLE_UI_BACKTRACE if VIEWER is not enabled")
    set(ENABLE_UI_BACKTRACE OFF)
endif()

# sanity check - cannot set UI_BACKTRACE_EMAIL_ADDRESSES if VIEWER and ENABLE_UI_BACKTRACE are OFF
if(UI_BACKTRACE_EMAIL_ADDRESSES AND (NOT ENABLE_VIEWER))
    message(WARNING "Cannot set UI_BACKTRACE_EMAIL_ADDRESSES if VIEWER is not enabled")
    set(UI_BACKTRACE_EMAIL_ADDRESSES)
endif()


message( STATUS "ENABLE_PYTHON         : ${ENABLE_PYTHON}" )
message( STATUS "ENABLE_GUI            : ${ENABLE_GUI}" )
message( STATUS "ENABLE_VIEWER         : ${ENABLE_VIEWER}" )
message( STATUS "ENABLE_QT5            : ${ENABLE_QT5}" )
message( STATUS "ENABLE_ALL_TESTS      : ${ENABLE_ALL_TESTS}" )

if (ENABLE_VIEWER)
    message( STATUS "ENABLE_UI_BACKTRACE   : ${ENABLE_UI_BACKTRACE}" )
    if(UI_BACKTRACE_EMAIL_ADDRESSES)
        message( STATUS "UI_BACKTRACE_EMAIL_ADDRESSES : ${UI_BACKTRACE_EMAIL_ADDRESSES}" )
    endif()
endif()

# ======================================================================================
# GUI requires X11 and Motif
# ======================================================================================

if(ENABLE_GUI)
  find_package( X11 )
#  if ( X11_FOUND )
#	debug_var( X11_INCLUDE_DIR )
#	debug_var( X11_LIBRARIES )  
#	debug_var( X11_Xt_INCLUDE_PATH )
#	debug_var( X11_Xt_LIB )
#	debug_var( X11_Xpm_INCLUDE_PATH )
#	debug_var( X11_Xpm_LIB )
#  endif()

  find_package( Motif )
#  if ( MOTIF_FOUND )
#	debug_var( MOTIF_INCLUDE_DIR )
#	debug_var( MOTIF_LIBRARIES )  
#  endif()
endif()
 
# =========================================================================================
# Boost
# =========================================================================================

ecbuild_add_extra_search_paths( boost ) # also respects BOOST_ROOT

# Ecflow test require statics libs for boost < 1.59, otherwise get double free error.
# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
# See: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_SYSTEM_PATHS        ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
#set(Boost_DEBUG                 ON)

find_package( Boost 1.53.0 REQUIRED COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time regex)

# Available boost lib should be referenced as:
#
#    ${Boost_SYSTEM_LIBRARY}
#    ${Boost_SERIALIZATION_LIBRARY}
#    ${Boost_THREAD_LIBRARY}
#    ${Boost_FILESYSTEM_LIBRARY}
#    ${Boost_PROGRAM_OPTIONS_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY}
#
#    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} 
#    ${Boost_TEST_EXEC_MONITOR_LIBRARY} 


#message( STATUS "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )



# =========================================================================================
# Qt
# =========================================================================================

if(ENABLE_VIEWER)
 if( ENABLE_QT5 )
      #find_package(Qt5Widgets REQUIRED)
      #find_package(Qt5Gui REQUIRED)
#      find_package(Qt5Network REQUIRED)
#      find_package(Qt5Xml REQUIRED)
#      find_package(Qt5XmlPatterns REQUIRED)
#      find_package(Qt5PrintSupport REQUIRED)

      if( Qt5Widgets_FOUND )
          message(STATUS "Qt5 version ${Qt5Widgets_VERSION_STRING} was found")
          SET(ECFLOW_QT_INCLUDE_DIR ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIR} ${Qt5Network_INCLUDE_DIR} ${Qt5Multimedia_INCLUDE_DIR})
          SET(ECFLOW_QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5Multimedia_LIBRARIES})
          set( ECFLOW_QT 1)
          set( ECFLOW_QT5 1)
          add_definitions( -DECFLOW_QT5 )
          #We need to disable keywords because there is a problem in using Qt and boost together.
          ADD_DEFINITIONS(-DQT_NO_KEYWORDS)
      else()
          message(FATAL_ERROR "Qt5 was NOT found - this is required for the new user interface")
      endif()
  else( ENABLE_QT5 )
    #find_package(Qt4 4.6.2 REQUIRED QtCore QtGui QtNetwork)
    if( QT_FOUND )
        set( ECFLOW_QT 1)
        #We need to disable keywords because there is a problem in using Qt and boost together.
        ADD_DEFINITIONS(-DQT_NO_KEYWORDS)
    else()
        message(FATAL_ERROR "Qt was not found - this is required for the new user interface")
    endif()
  endif( ENABLE_QT5 )
endif()


# =========================================================================================
# debug
# =========================================================================================

if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )

	message( STATUS "INFO: DEBUG BUILD" )

    # Tell C/C++ that we're doing a debug build
    add_definitions( -DDEBUG )

endif()

# =========================================================================================
# build source code
# =========================================================================================

add_subdirectory( ACore )
add_subdirectory( ANattr )
add_subdirectory( ANode )
add_subdirectory( AParser )
add_subdirectory( Base )
add_subdirectory( Client )
add_subdirectory( CSim )
add_subdirectory( Server )
add_subdirectory( Test )

if (ENABLE_PYTHON)
    ecbuild_find_python( VERSION 2.7 REQUIRED )
    #debug_var(PYTHON_LIBRARIES)
    #debug_var(PYTHON_INCLUDE_DIR)
	add_subdirectory( Pyext )
endif()	
	
if(ENABLE_VIEWER)
    add_subdirectory( Viewer )
    add_subdirectory( share )
endif()

if( ENABLE_GUI AND X11_FOUND AND MOTIF_FOUND )
   add_subdirectory( view )
endif()

# =========================================================================================
# tar ball, by default ecbuild will tar everything apart from hard wired directory called 'build' 
#   hence we can tell it, what directories not to pack
# =========================================================================================

ecbuild_dont_pack( DIRS 
                   	ACore/doc ANattr/doc ANode/doc Client/doc Pyext/doc Server/doc
                   	ecbuild
                   	Viewer
                    SCRATCH 
                    build_scripts/nightly build_scripts/test_bench
                    Doc/func_spec Doc/misc Doc/New_viewer Doc/newsletter Doc/online Doc/presentations Doc/tac
)

# temporary until its officially released
if (NOT ENABLE_VIEWER)
   ecbuild_dont_pack( DIRS Viewer )
endif()  

# =========================================================================================
# install scripts
# =========================================================================================

install( FILES
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.pl
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_start.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_stop.sh
            ${CMAKE_SOURCE_DIR}/tools/noconnect.sh
            ${CMAKE_SOURCE_DIR}/Pyext/migrate/ecflow_migrate.py
         DESTINATION bin
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
)

# install documentation         
install (FILES   
             ${CMAKE_SOURCE_DIR}/Doc/user-manual/client_options.docx  
         DESTINATION doc/ecflow
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
)
        
# =========================================================================================
# final
# =========================================================================================

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )

# print the summary of the configuration
ecbuild_print_summary()
