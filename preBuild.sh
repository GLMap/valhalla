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
    cmake -P cmake/ValhallaBin2Header.cmake $file $output --variable-name $variable --skip 1 --raw
done