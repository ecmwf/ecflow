#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # GNU GCC extensions are required by tools/ecflow_standalone

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


ecbuild_add_option( FEATURE WARNINGS
                    DEFAULT ON
                    DESCRIPTION "Enable extended compiler warnings" )

ecbuild_add_option( FEATURE COLOURED_OUTPUT
                    DEFAULT ON
                    DESCRIPTION "Make sure that GCC/Clang produce coloured output" )

ecbuild_add_option( FEATURE EXPORT_COMPILE_COMMANDS
                    DEFAULT ON
                    DESCRIPTION "Activate generation of compilation commands file" )


ecbuild_info( "CMAKE_C_COMPILER_ID        : ${CMAKE_C_COMPILER_ID}")
ecbuild_info( "CMAKE_C_COMPILER_VERSION   : ${CMAKE_C_COMPILER_VERSION}")
ecbuild_info( "CMAKE_CXX_COMPILER_ID      : ${CMAKE_CXX_COMPILER_ID}")
ecbuild_info( "CMAKE_CXX_COMPILER_VERSION : ${CMAKE_CXX_COMPILER_VERSION}")


if (HAVE_WARNINGS)

  ecbuild_add_c_flags(-Wall)
  ecbuild_add_c_flags(-Wextra)
  ecbuild_add_c_flags(-Wpedantic)

  ecbuild_add_cxx_flags(-Wall)
  ecbuild_add_cxx_flags(-Wextra)
  ecbuild_add_cxx_flags(-Wpedantic)

  # Silence compiler warnings
  # n.b. All these extra compiler options should be removed, and the compiler warnings properly silenced
  add_compile_options(
    # C++-related warnings
    ## GCC
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-array-bounds>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-deprecated-declarations>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-format>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-maybe-uninitialized>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-pessimizing-move>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-unused-result>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-unused-parameter>
    ## Clang (MacOS AppleClang + Homebrew, AMD Clang-base)
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-char-subscripts>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-deprecated-copy-with-user-provided-copy>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-deprecated-declarations>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-format>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-missing-field-initializers>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-overloaded-virtual>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-unused-but-set-variable>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-unused-parameter>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wno-unused-private-field>
    ## Intel (Clang-based)
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:IntelLLVM>>:-Wno-deprecated-declarations>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:IntelLLVM>>:-Wno-format>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:IntelLLVM>>:-Wno-overloaded-virtual>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:IntelLLVM>>:-Wno-unused-private-field>
  )

endif ()


if (HAVE_COLOURED_OUTPUT)
  if ("${CMAKE_GENERATOR}" STREQUAL "Ninja" AND ENABLE_COLOURED_OUTPUT)
    message(STATUS "Ninja generator detected! Ensuring GNU/Clang produce coloured output...")
    add_compile_options(
      $<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always>
      $<$<CXX_COMPILER_ID:Clang>:-fdiagnostics-color>
    )
  endif ()
endif ()


if (HAVE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()
