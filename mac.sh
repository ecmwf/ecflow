#/bin/bash

cmake -DCMAKE_MODULE_PATH=${HOME}/bitbucket/ecbuild/cmake/ ${HOME}/bitbucket/ecflow/  \
      -DCMAKE_CXX_FLAGS='-ftemplate-depth=512 -Wno-deprecated-declarations' -DBOOST_ROOT=/usr/local -DCMAKE_PREFIX_PATH=/usr/local/opt/qt \
      -DCMAKE_INSTALL_PREFIX=${HOME}/install_test -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl