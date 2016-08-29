#!/bin/sh

# build update package
./update.sh develop &&
./update.sh demo &&
./update.sh operating
