#!/bin/sh

# compile freeswtich
cd freeswitch
chmod +x ./compile.sh || exit 1
./compile.sh $1 || exit 1
cd .. &&

# compile dependent tools
cd deps 
chmod +x ./compile.sh || exit 1
./compile.sh $1 || exit 1
cd .. &&

# compile CamShareServer
cd CamShareServer
chmod +x ./compile.sh || exit 1
./compile.sh $1 || exit 1
cd ..

