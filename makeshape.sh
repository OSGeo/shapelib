#!/bin/sh

#
#	Use example programs to create a very simple dataset that
#	should display in ARCView II.
#

shpcreate test polygon
dbfcreate test.dbf -s Description 30

shpadd test 0 0 100 0 100 100 0 100 0 0 + 20 20 20 30 30 30 20 20
dbfadd test.dbf "Square with triangle missing"

shpadd test 150 150 160 150 180 180 150 150
dbfadd test.dbf "Smaller triangle"


