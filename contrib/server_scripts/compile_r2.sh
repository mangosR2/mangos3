#!/bin/bash
#
#
PREFIX=${PWD}/bin
CONF_OPTS+="-DPCH=1 "
CONF_OPTS+="-DDEBUG=1 "
CONF_OPTS+="-DACE_USE_EXTERNAL=1 "
CONF_OPTS+="-DUSE_STD_MALLOC=1 "
#CONF_OPTS+="-DUSE_TBB_MALLOC=1"
LIB_INSTALL_DIR=${PWD}/lib
INCLUDE_INSTALL_DIR=${PWD}/include

CFLAGS="-march=native -O2 -DNDEBUG"
#CFLAGS="-O1 -fno-inline"
CFLAGS+=" -pipe -ggdb -fno-strict-aliasing -fno-delete-null-pointer-checks -D_LARGEFILE_SOURCE -finput-charset=utf-8 -fexec-charset=utf-8"
CXXFLAGS="${CFLAGS}"

cd ~/Mangos-Sources/mangos

#patches=`find ./src/bindings/scriptdev2/patches -maxdepth 1 -name "*.patch"`
patches=`find ../patches -maxdepth 1 -name "*.patch"`

for j in $patches; do
    echo "Apply patch : "$j
    echo $(git apply <$j);
    echo $?
done

echo "Do compile now";

if [ ! -d ./build ]; then
    echo "Reconfiguration...";
    mkdir build
    cd build
    cmake .. ${CONF_OPTS} \
        -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DLIB_INSTALL_DIR="${LIB_INSTALL_DIR}" \
        -DINCLUDE_INSTALL_DIR="${INCLUDE_INSTALL_DIR}" \
        -DCMAKE_C_FLAGS="${CFLAGS}" \
        -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
        -DCMAKE_C_COMPILER="gcc" \
        -DCMAKE_CXX_COMPILER="g++"
    ..
else
    cd build
fi

nice -15 make
RETVAL=$?

echo

if [ $RETVAL -eq 0 ]; then
    echo "Compiled fine!";
    nice -15 make install
    RETVAL=$?
    if [ $RETVAL -eq 0 ]; then
        echo "Installed OK!";
        . ~/bin/auto_update.sh
        . ~/bin/world_restart.sh
    else
        echo "Install ERROR!";
    fi
else
    echo "Compilation ERROR!";
fi
