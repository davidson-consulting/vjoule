#!/bin/bash

if [[ ! -f ./libpfm_patched.a ]]
then
    git clone https://github.com/gfieni/libpfm4.git
    ls
    cd libpfm4
    git config pull.rebase true
    git checkout smartwatts
    git pull origin smartwatts
    make
    cd ..
    cp libpfm4/lib/libpfm.a libpfm_patched.a
fi

echo "[100%] patch_libpfm.a build"
