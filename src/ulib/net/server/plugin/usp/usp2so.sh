#!/bin/sh

# export WINELOADER=wine

set -x

for i in *.usp
do
	make $(basename $i .usp).la
done
