#!/bin/sh

# compile freeswtich
cd freeswitch &&
chmod +x ./compile.sh &&
./compile.sh $1 &&
cd .. &&

# compile dependent tools
cd deps &&
chmod +x ./compile.sh &&
./compile.sh $1 &&
cd .. &&

# compile CamShareServer
cd CamShareServer &&
chmod +x ./compile.sh &&
./compile.sh $1 &&
cd ..

