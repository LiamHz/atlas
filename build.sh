#!/bin/bash

mkdir build

cp -r resources/obj/* resources/shaders/* build/

cmake -S . -B build
cd build/
make

if [[ $* == *-p* ]] || [[ $* == *--pack* ]]
then
    cd ../
    tar -czvf build.tar.gz build
    cd build/
fi

if [[ $* == *-r* ]] || [[ $* == *--run* ]]
then
    ./Atlas
fi