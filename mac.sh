#/bin/bash

#https://medium.com/@timmykko/using-openssl-library-with-macos-sierra-7807cfd47892
#
#brew install cmake
#brew install boost
#brew install boost-python3
#brew install openssl  # however this may not set the right links
#
# Do the following if config steps fails with cannot find openssl
#ln -s /usr/local/opt/openssl/include/openssl /usr/local/include
#
#ln -s /usr/local/opt/openssl/lib/libssl.1.1.1.dylib /usr/local/lib/
#
# to list the clang default system search path use:
#clang -x c -v -E /dev/null
#

cmake -DCMAKE_MODULE_PATH=${HOME}/bitbucket/ecbuild/cmake/ ${HOME}/bitbucket/ecflow/  \
      -DCMAKE_CXX_FLAGS='-fvisibility=hidden -fvisibility-inlines-hidden -ftemplate-depth=512 -Wno-deprecated-declarations' -DBOOST_ROOT=/usr/local -DCMAKE_PREFIX_PATH=/usr/local/opt/qt \
      -DCMAKE_INSTALL_PREFIX=${HOME}/install_test -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl