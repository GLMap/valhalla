#!/bin/bash

for file in date_time/*; do
    variable="${file/\//_}"
    variable="${variable/./_}"
    variable="${variable/windowsZones/windows_zones}"
    
    output=$variable
    output="${output/_xml/}"
    output="${output/_csv/}"
    output="genfiles/$output.h"

    echo $file $output $variable
    cmake -P cmake/Binary2Header.cmake $file $output --variable-name $variable --skip 1 --raw
done

## temporary fix for third_party/date
pushd third_party/date
patch -N -p1 <../../date.patch
popd