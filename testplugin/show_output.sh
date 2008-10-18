#!/bin/sh
sed -r 's/:/ /g' $1 | awk '{ print "head -n", $2,  $1, " | tail -1"}' | sh
