#!/bin/sh

# compile dependent tools
cd deps
./compile.sh
cd ..

# compile freeswtich
cd freeswitch
./compile.sh
cd ..

# compile CamShareServer
cd CamShareServer
./compile.sh
cd ..
