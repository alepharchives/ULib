#!/bin/sh

# export WINELOADER=wine

for i in *.usp
do
	make $(basename $i .usp).la
done
