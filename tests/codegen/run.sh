#!/bin/bash
# Non regression of the GRAFCET code generator: for each GRAFCET example, the
# generated code (compiled in C++11, as for a micro-controller, then with
# GRAFCET_DEBUG and ZeroMQ) shall give the same steps and outputs as the
# engine of the editor, cycle after cycle. Run from the root of the project
# after make.
set -u
BUILD=${BUILD:-build}
OUT=$BUILD/codegen
mkdir -p $OUT
g++ -std=c++20 -Isrc -Iexternal -Iexternal/json/include tests/codegen/GrafcetCodegen.cpp \
    -L$BUILD -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$PWD/$BUILD -o $OUT/GrafcetCodegen || exit 1
status=0
for json in $(grep -rl --include='*.json' '"type": "GRAFCET"' examples); do
    name=$(basename $json .json)
    if ! $OUT/GrafcetCodegen $json $PWD/$OUT/$name.hpp $OUT/$name.engine $OUT/$name.driver.cpp; then
        echo "FAIL $name: generation"; status=1; continue
    fi
    for mode in plain debug; do
        flags="-std=c++11 -Wall -Wextra -Werror"
        libs=""
        if [ $mode = debug ]; then
            pkg-config --exists libzmq || continue
            flags="$flags -DGRAFCET_DEBUG $(pkg-config --cflags libzmq)"
            libs="$(pkg-config --libs libzmq)"
        fi
        if ! g++ $flags $OUT/$name.driver.cpp -o $OUT/$name.$mode $libs 2> $OUT/$name.$mode.err; then
            echo "FAIL $name ($mode): compilation"; head -5 $OUT/$name.$mode.err; status=1; continue
        fi
        $OUT/$name.$mode > $OUT/$name.$mode.txt
        if diff -q $OUT/$name.engine $OUT/$name.$mode.txt > /dev/null; then
            echo "ok   $name ($mode)"
        else
            echo "FAIL $name ($mode): first difference:"; diff $OUT/$name.engine $OUT/$name.$mode.txt | head -4
            status=1
        fi
    done
done
exit $status
