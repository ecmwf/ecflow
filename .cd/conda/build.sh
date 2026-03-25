
#!/usr/bin/env bash

set -e # Abort on error.
set -x # Display executed commands

ENABLE_SSL=ON
ENABLE_PYTHON=ON
ENABLE_HTTP=ON
ENABLE_UDP=ON
ENABLE_UI=ON

# find the boost libs/includes we need
export LDFLAGS="$LDFLAGS -L$PREFIX/lib -Wl,-rpath,$PREFIX/lib"
export CFLAGS="$CFLAGS -fPIC -I$PREFIX/include"


if [[ $(uname) == Darwin && ${target_platform} == osx-64 ]]; then
    # Disable use of std::aligned_alloc by boost, as this is not available on macOS 10.9
    export CXXFLAGS="$CXXFLAGS -DBOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC"

    # https://conda-forge.org/docs/maintainer/knowledge_base.html#newer-c-features-with-old-sdk
    export CXXFLAGS="${CXXFLAGS} -D_LIBCPP_DISABLE_AVAILABILITY"

    # Disable ecflow_http build, as it uses C++17 features only available on macOS 10.12+
    ENABLE_HTTP=OFF
    # Disable ecflow_udp build, as it uses C++17 features only available on macOS 10.13+
    ENABLE_UDP=OFF
fi

# Diagnostic information
${CXX} --version
${CXX} -dM -E - <<HERE
#include <sys/socket.h>
HERE

export UNDEF_LOOKUP=0
if [[ $(uname) == Darwin ]]; then
    export UNDEF_LOOKUP=1
fi

mkdir build && cd build

conda info

echo "which python"
which python
echo "python version"
python --version

cmake ${CMAKE_ARGS} \
      -D CMAKE_INSTALL_PREFIX=$PREFIX \
      -D ENABLE_PYTHON=$ENABLE_PYTHON \
      -D ENABLE_SSL=$ENABLE_SSL \
      -D ENABLE_HTTP=$ENABLE_HTTP \
      -D ENABLE_UDP=$ENABLE_UDP \
      -D ENABLE_UI=$ENABLE_UI \
      -D BOOST_ROOT=$PREFIX \
      -D ECBUILD_LOG_LEVEL=DEBUG \
      -D ENABLE_STATIC_BOOST_LIBS=OFF \
      -D Python3_FIND_STRATEGY=LOCATION \
      -D Python3_EXECUTABLE=$PYTHON \
      -D ENABLE_PYTHON_UNDEF_LOOKUP=$UNDEF_LOOKUP \
      -D Cereal_INCLUDE_DIRS=../3rdparty/cereal/include \
      -D JSON_INCLUDE_DIRS=../3rdparty/json/include \
      -D HTTPLIB_INCLUDE_DIRS=../3rdparty/cpp-httplib/include \
      ..

make -j $CPU_COUNT VERBOSE=1


# only run certain tests
if [[ $(uname) == Linux ]]; then
    echo "1,2,3,4,5,6,7,8" > ./test_list.txt
elif [[ $(uname) == Darwin ]]; then
    echo "1,2,3,4,5,6,8" > ./test_list.txt
fi

if [[ "${CONDA_BUILD_CROSS_COMPILATION:-}" != "1" || "${CROSSCOMPILING_EMULATOR}" != "" ]]; then
ctest -VV -I ./test_list.txt
fi

make install
