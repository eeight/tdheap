#!/bin/sh

readelf -wi $1 | grep -E 'DW_AT_(name|(high|low)_pc)' | sed -re '
  s/DW_AT_//
  s/^\s*<[0-9a-z]+>\s*//
  s/\([^\)]+\)//
  s/(\s|:)+/ /
  '
