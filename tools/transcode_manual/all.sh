#!/bin/sh

rm -f /tmp/transcode.log

/home/samson/Max/cmd/transcode.sh /home/samson/Max/cmd/result/result_202001_flv.txt
/home/samson/Max/cmd/transcode.sh /home/samson/Max/cmd/result/result_202002_flv.txt

echo "done"