#!/bin/bash

function build() {
    CURRENT_DIR=$(cd $(dirname $0) && pwd)
    cd ..
    mkdir -p build && cd build
    cmake ..
    make -j
    cd $CURRENT_DIR
}

function run_test() {
    COMPILE=../build/mini

    find "$(pwd)" -type f -name "*.mini" | while read file; do
        echo "testing $file"

        $COMPILE $file
        ./a.out
        CODE=$?

        rm a.out
        if [ $CODE -ne 0 ]; then
            echo "test failed with code $CODE"
            exit 1
        fi
    done

    CODE=$?
    if [ $CODE -eq 0 ]; then
        echo "all test passed"
    fi
}

build
run_test
