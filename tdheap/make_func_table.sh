#!/bin/sh

readelf -wi $1 | grep -E '(DW_AT_(name|(high|low)_pc))|DW_TAG_subprogram' | sed -re '
  s/.*DW_TAG_(subprogram).*/\1/
  s/DW_AT_//
  s/^\s*<[0-9a-z]+>\s*//
  s/\([^\)]+\)//
  s/(\s|:)+/ /
  '
