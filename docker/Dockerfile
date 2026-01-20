# Building ecflow from source with static boost for Rocky Linux 9.5.
# https://ecflow.readthedocs.io/en/5.14.0/install/build_from_source.html
# https://gitlab.met.no/ecflow-modellprod/build-ecflow

# Build:
# docker build -t ecflowrocky9 --build-arg ECVERSION="5.14.0" -f Dockerfile.rocky .
# docker run -it --rm ecflowrocky9 bash

FROM docker.io/rockylinux/rockylinux:9.5


# System
RUN dnf upgrade --quiet -y && \
  dnf install -y langpacks-en && \
  dnf install -y \
    cmake python3-devel wget \
    qt5-qtbase-devel qt5-qtbase-gui qt5-qtconnectivity-devel qt5-qtsvg-devel \
    motif-devel libX11-devel libXext-devel libXpm-devel libXt-devel \
    gcc-toolset-14 openssl-devel which

WORKDIR /ecflow_build
ENV BUILD_DIR=/ecflow_build/

ARG ECVERSION="5.14.1" \
    BOOST_VERSION="1.75.0" \
    BOOST_VERSION_SUB="1_75_0" \
    MAKEJ="8"

## Download, extract and cache files. Wget is required for this step

# tarball from github doesn't contain tools we need
# https://www.boost.org/users/download/
RUN wget https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_SUB}.tar.gz && \
    tar xzf boost_${BOOST_VERSION_SUB}.tar.gz

RUN wget https://github.com/ecmwf/ecflow/releases/download/${ECVERSION}/ecFlow-${ECVERSION}-Source.tar.gz && tar -xvf ecFlow-${ECVERSION}-Source.tar.gz


## Build boost
RUN cd ${BUILD_DIR}boost_${BOOST_VERSION_SUB} &&  source scl_source enable gcc-toolset-14 && ./bootstrap.sh  --prefix=/modules/rhel9/user-apps/ecflow/$ECVERSION --with-python=/usr/bin/python3 --with-libraries=system,timer,filesystem,program_options,chrono,date_time,test,python && \
    for lib in system timer filesystem program_options chrono date_time test python  ; do ./tools/build/src/engine/bjam link=static cxxflags=-fPIC cflags=-fPIC libs/$lib/build; done;
RUN cd ${BUILD_DIR}boost_${BOOST_VERSION_SUB} &&  source scl_source enable gcc-toolset-14 && ./b2 install --with-python --with-system --with-timer --with-filesystem --with-program_options --with-chrono --with-date_time --with-test link=static cxxflags=-fPIC cflags=-fPIC; true

## Build ecflow
RUN cd ecFlow-${ECVERSION}-Source && rm -rf ${BUILD_DIR}/build ; mkdir -p ${BUILD_DIR}/build && \
  source scl_source enable gcc-toolset-14 && \
  cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/modules/rhel9/user-apps/ecflow/${ECVERSION} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS=-w \
  -DENABLE_GUI=OFF -DENABLE_SERVER=ON -DENABLE_PYTHON=ON -DENABLE_UI=ON -DPYTHON_EXECUTABLE=/usr/bin/python3 \
  -DBoost_INCLUDE_DIR=/modules/rhel9/user-apps/ecflow/${ECVERSION}/include -DCMAKE_CXX_FLAGS=-w -DCMAKE_PREFIX_PATH=/modules/rhel9/user-apps/ecflow/${ECVERSION}/ \
  -DBOOST_LIBRARYDIR=/modules/rhel9/user-apps/ecflow/${ECVERSION}/lib \
  -DENABLE_STATIC_BOOST_LIBS=ON \
  -S .

RUN cd ecFlow-${ECVERSION}-Source/build && \
  source scl_source enable gcc-toolset-14 && make -j${MAKEJ} && make install && make test || true

CMD ["/modules/rhel9/user-apps/ecflow/5.14.1/bin/ecflow_server"]
