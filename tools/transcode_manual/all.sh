#!/bin/sh

rm -f transcode.log

./transcode.sh ./30_31/flv_30.txt
./transcode.sh ./30_31/flv_31.txt

echo "done"