#!/bin/bash

cd "$(dirname "$(dirname "$0")")"
git ls-files \
    | egrep '.*(\.cpp|\.h|\.hpp)$' \
    | xargs -n 10 -P $(nproc) clang-format-19 -i
