#!/bin/sh
if [ -x /usr/local/bin/convert ]; then
  /usr/local/bin/convert \
    -background lightgrey -fill black \
    -pointsize 14 -font Courier \
    label:"$1.hex $2" \
    -trim +repage -bordercolor lightgrey  -border 3 \
    -bordercolor black  -border 1 \
    $1.gif
fi
