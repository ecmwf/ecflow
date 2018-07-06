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
##############################################################################

cmake_minimum_required( VERSION 2.8.11 FATAL_ERROR )

# ===========================================================================
# Change the default if NO CMAKE_BUILD_TYPE specified
if (NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Configuration type" FORCE)
endif()


project( ecflow CXX )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild/cmake")
# ecbuild_info( "CMAKE_MODULE_PATH    : ${CMAKE_MODULE_PATH}")

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

ecbuild_info( "CMAKE_INSTALL_PREFIX     : ${CMAKE_INSTALL_PREFIX}" )
ecbuild_info( "ECFLOW_RELEASE           : ${ECFLOW_RELEASE}" )
ecbuild_info( "ECFLOW_MAJOR             : ${ECFLOW_MAJOR}" )
ecbuild_info( "ECFLOW_MINOR             : ${ECFLOW_MINOR}" )
ecbuild_info( "ECFLOW_VERSION           : ${ECFLOW_VERSION_STR}" )

# =========================================================================================
# Options , be aware of caching, when modifying on the command line. 
# Ideally start fresh or remove cache CmakeCache.txt in build dir
# =========================================================================================
option( ENABLE_SERVER              "Build the server, can be switched off if just building UI" ON  )
option( ENABLE_PYTHON              "enable python interface"         ON  )
option( ENABLE_UI                  "Build ecFlowUI"                  ON  ) 
option( ENABLE_STATIC_BOOST_LIBS   "Link with boost libs statically" ON  )
option( ENABLE_ALL_TESTS           "This includes performance/migration/regression tests" OFF )
option( ENABLE_UI_BACKTRACE        "Print a UI debug backtrace"         OFF ) 
option( ENABLE_UI_USAGE_LOG        "Enable UI usage logging"            OFF )
option( ENABLE_SSL                 "Enable SSL encrypted communication" OFF )
option( ENABLE_SECURE_USER         "Enable passwords for user"          OFF )
option( ENABLE_PYTHON_PTR_REGISTER "Some compilers/boost versions do not register shared ptr automatically" OFF  )

# =========================================================================================
# Qt for ecFlowUI. 
# Algorithm: we test for Qt5 - if it's there, then use it; otherwise look for Qt4.
#            if we don't find that, then we cannot build ecFlowUI.
# =========================================================================================
if(ENABLE_UI)

    ecbuild_info("")
    ecbuild_info("------------------------------ Qt start ---------------------------")
    ecbuild_info("ecFlowUI is enabled - searching for Qt5; if not found, will try Qt4")
    ecbuild_info("To use a self-built Qt installation, try setting CMAKE_PREFIX_PATH")

    find_package(Qt5Widgets 5.0.0)
    if (Qt5Widgets_FOUND)
        find_package(Qt5Gui 5.0.0)
        if (Qt5Gui_FOUND)
            find_package(Qt5Network 5.0.0)
            if (Qt5Network_FOUND)
                find_package(Qt5Svg 5.0.0)
                if (Qt5Svg_FOUND)
                    find_package(Qt5Charts 5.0.0)
                endif()
            endif()
        endif()
    endif()

    if (Qt5Widgets_FOUND AND Qt5Gui_FOUND AND Qt5Network_FOUND AND Qt5Svg_FOUND)
          ecbuild_info("Qt5 version ${Qt5Widgets_VERSION_STRING} was found")
          set(ECFLOW_QT_INCLUDE_DIR ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Svg_INCLUDE_DIRS})
          set(ECFLOW_QT_LIBRARIES   ${Qt5Widgets_LIBRARIES}    ${Qt5Gui_LIBRARIES}   ${Qt5Network_LIBRARIES} ${Qt5Svg_LIBRARIES})
          set(ECFLOW_QT5 1)
          add_definitions(-DECFLOW_QT5)

          if(Qt5Charts_FOUND)
              list(APPEND ECFLOW_QT_INCLUDE_DIR ${Qt5Charts_INCLUDE_DIRS})
              list(APPEND ECFLOW_QT_LIBRARIES  ${Qt5Charts_LIBRARIES})
              set(ECFLOW_LOGVIEW 1)
              add_definitions(-DECFLOW_LOGVIEW)
          endif()

    else()
        ecbuild_info("Could not find all required Qt5 packages (Qt5Widgets, Qt5Gui and Qt5Network) - searching for Qt4")
        ecbuild_info(" ** To use a self-built Qt installation, try setting CMAKE_PREFIX_PATH **")

        find_package(Qt4 4.8.0 REQUIRED QtCore QtGui QtNetwork QtSvg)
        if(QT_FOUND)
            ecbuild_info("Found Qt 4, please ignore any warnings about Qt5 components that were not found.")
        else()
            ecbuild_info(" ** To use a self-built Qt installation, try setting CMAKE_PREFIX_PATH **")
            ecbuild_critical("Qt5 and Qt4 not found - one of these is required for ecFlowUI; or use -DENABLE_UI=OFF")
        endif()
    endif()

    set(ECFLOW_QT 1)
    add_definitions(-DQT_NO_KEYWORDS) # We need to disable keywords because there is a problem in using Qt and boost together.
    ecbuild_info("------------------------------ Qt end----------------------------")
    ecbuild_info("")

endif()


# sanity check - cannot set ENABLE_UI_BACKTRACE if UI is OFF
if(ENABLE_UI_BACKTRACE AND (NOT ENABLE_UI))
	ecbuild_warn("Cannot ENABLE_UI_BACKTRACE if UI is not enabled")
    set(ENABLE_UI_BACKTRACE OFF)
endif()

# sanity check - cannot set UI_BACKTRACE_EMAIL_ADDRESS_FILE if UI and ENABLE_UI_BACKTRACE are OFF
if(UI_BACKTRACE_EMAIL_ADDRESS_FILE AND (NOT ENABLE_UI))
	ecbuild_warn("Cannot set UI_BACKTRACE_EMAIL_ADDRESS_FILE if UI is not enabled")
    set(UI_BACKTRACE_EMAIL_ADDRESS_FILE)
endif()

# sanity check - cannot set UI_LOG_FILE if ENABLE_UI_USAGE_LOG is OFF
if(UI_LOG_FILE AND (NOT ENABLE_UI_USAGE_LOG))
	ecbuild_warn("Cannot set UI_LOG_FILE if ENABLE_UI_USAGE_LOG is not enabled")
	set(UI_LOG_FILE)
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_FILE
if(ENABLE_UI_USAGE_LOG AND (NOT UI_LOG_FILE))
	ecbuild_error("If ENABLE_UI_USAGE_LOG is set, UI_LOG_FILE must also be set")
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_FILE
if(ENABLE_UI_USAGE_LOG AND (NOT LOGUI_LOG_FILE))
    ecbuild_error("If ENABLE_UI_USAGE_LOG is set, LOGUI_LOG_FILE must also be set")
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_SITE_TAG
if(ENABLE_UI_USAGE_LOG AND (NOT UI_LOG_SITE_TAG))
	ecbuild_error("If ENABLE_UI_USAGE_LOG is set, UI_LOG_SITE_TAG must also be set")
endif()


ecbuild_info( "ENABLE_SERVER              : ${ENABLE_SERVER}" )
ecbuild_info( "ENABLE_PYTHON              : ${ENABLE_PYTHON}" )
ecbuild_info( "ENABLE_PYTHON_PTR_REGISTER : ${ENABLE_PYTHON_PTR_REGISTER}" )
ecbuild_info( "ENABLE_UI                  : ${ENABLE_UI}" )
ecbuild_info( "ENABLE_ALL_TESTS           : ${ENABLE_ALL_TESTS}" )
ecbuild_info( "ENABLE_STATIC_BOOST_LIBS   : ${ENABLE_STATIC_BOOST_LIBS}" )
ecbuild_info( "ENABLE_SSL                 : ${ENABLE_SSL}" )
ecbuild_info( "ENABLE_SECURE_USER         : ${ENABLE_SECURE_USER}" )


if (ENABLE_UI)
	ecbuild_info( "ENABLE_UI_BACKTRACE      : ${ENABLE_UI_BACKTRACE}" )
	if(UI_BACKTRACE_EMAIL_ADDRESS_FILE)
		ecbuild_info( "UI_BACKTRACE_EMAIL_ADDRESS_FILE : ${UI_BACKTRACE_EMAIL_ADDRESS_FILE}" )
	endif()

    if(LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE)
        ecbuild_info( "LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE : ${LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE}" )
    endif()

	ecbuild_info( "ENABLE_UI_USAGE_LOG      : ${ENABLE_UI_USAGE_LOG}" )
	if(ENABLE_UI_USAGE_LOG)
		ecbuild_info( "UI_LOG_FILE              : ${UI_LOG_FILE}" )
        ecbuild_info( "LOGUI_LOG_FILE           : ${LOGUI_LOG_FILE}" )
        ecbuild_info( "UI_LOG_SITE_TAG          : ${UI_LOG_SITE_TAG}" )
	endif()
endif()
ecbuild_info("")


# =========================================================================================
# cereal is embedded
# =========================================================================================
include_directories( ${CMAKE_SOURCE_DIR}/cereal/include )
 
# =========================================================================================
# Boost
# =========================================================================================

# this can help in some situations:
set (_CMAKE_PREFIX_PATH_BACKUP ${CMAKE_PREFIX_PATH}) # make a backup of CMAKE_PREFIX_PATH
foreach(BOOST_PATH_TO_SEARCH ${BOOST_PATH} ${BOOST_ROOT} $ENV{BOOST_ROOT})
    ecbuild_debug("adding: ${BOOST_PATH_TO_SEARCH} to CMAKE_PREFIX_PATH")
    list(APPEND CMAKE_PREFIX_PATH ${BOOST_PATH_TO_SEARCH})
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endforeach()


# Ecflow test require statics libs for boost < 1.59, otherwise get double free error.
# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
# See: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html
if ( ENABLE_STATIC_BOOST_LIBS )
    set(Boost_USE_STATIC_LIBS ON)
endif()
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_SYSTEM_PATHS        ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
#set(Boost_DEBUG                 ON)

if ( ENABLE_PYTHON )
	find_package( Boost 1.53.0 REQUIRED COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time regex)
else()
	find_package( Boost 1.53.0 REQUIRED COMPONENTS serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time regex)
endif()

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

#ecbuild_info( "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )


if (NOT "${CMAKE_PREFIX_PATH}" EQUAL "${_CMAKE_PREFIX_PATH_BACKUP}")
    set (CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH_BACKUP}) # restore CMAKE_PREFIX_PATH
    ecbuild_debug("Resetting CMAKE_PREFIX_PATH to ${CMAKE_PREFIX_PATH}")
endif()

# =======================================================================
# SSL
# =======================================================================
if (ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
    if (NOT OPENSSL_FOUND )
        ecbuild_error("Can not find openssl libraries")
    endif() 
    include_directories( ${OPENSSL_INCLUDE_DIR} )
    add_definitions( -DECF_OPENSSL )
endif()

# =========================================================================================
# debug
# =========================================================================================

if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )

	ecbuild_info( "INFO: DEBUG BUILD" )

    # Tell C/C++ that we're doing a debug build
    add_definitions( -DDEBUG )

endif()

# =========================================================================================
# CLANG: /usr/local/include/boost/type_traits/is_base_and_derived.hpp:226:42: 
#           fatal error: recursive template instantiation exceeded maximum depth of 256
# =========================================================================================
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()


# =========================================================================================
# build source code
# =========================================================================================
add_subdirectory( ACore )
add_subdirectory( ANattr )
add_subdirectory( ANode )
add_subdirectory( Base )
add_subdirectory( CSim )
add_subdirectory( Client )
add_subdirectory( tools )

if (ENABLE_SERVER)
    add_subdirectory( Server )
    add_subdirectory( Test )
endif()

if (ENABLE_PYTHON)
    ecbuild_find_python( VERSION 2.6 REQUIRED )
    # ecbuild_debug_var(PYTHON_LIBRARIES)
    # ecbuild_debug_var(PYTHON_INCLUDE_DIR)
    
    if( NOT PYTHON_FOUND )
        ecbuild_error("ecflow python extension is enabled, but python interpreter or libraries not found")
    endif()

    if ( ENABLE_PYTHON_PTR_REGISTER )
        add_definitions( -DECF_ENABLE_PYTHON_PTR_REGISTER )
    endif()
	
    add_subdirectory( Pyext )
endif()	
	
if(ENABLE_UI)
    add_subdirectory( Viewer )
    add_subdirectory( share )
endif()

# =========================================================================================
# tar ball, by default ecbuild will tar everything apart from hard wired directory called 'build' 
# hence we can tell it, what directories not to pack
# =========================================================================================
ecbuild_dont_pack( DIRS 
                    .settings
                    bamboo
                   	ACore/doc ANattr/doc ANode/doc Client/doc CSim/doc Pyext/doc Server/doc
                   	ecbuild
                    SCRATCH
                    CUSTOMER 
                    build_scripts/nightly build_scripts/test_bench
                    Doc
)

# ignore eclipse files
ecbuild_dont_pack( FILES 
                   .project
                   .cproject
                   .pydevproject
                   build_scripts/.pydevproject
)


# =========================================================================================
# final
# =========================================================================================

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )

# print the summary of the configuration
ecbuild_print_summary()
