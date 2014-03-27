#!/bin/sh
PREFIX=${PWD}/bin
CONF_OPTS+="-DPCH=0 "
CONF_OPTS+="-DDEBUG=1 "
CONF_OPTS+="-DACE_USE_EXTERNAL=1 "
CONF_OPTS+="-DUSE_STD_MALLOC=1 "

CFLAGS="-march=native -O2 -DNDEBUG -std=c++11 -ggdb"
CXXFLAGS="${CFLAGS}"

rm -Rf build &&
mkdir build &&
cd build &&

cmake .. ${CONF_OPTS} \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_C_COMPILER="clang" \
    -DCMAKE_CXX_COMPILER="clang++"
