#!/bin/sh

cd ${MESON_SOURCE_ROOT}/scripts
run-clang-tidy.py -p ${MESON_BUILD_ROOT} -fix -format -header-filter='^((?!\.pb).)*\.h$' '^((?!pb).)*\.cc$'
