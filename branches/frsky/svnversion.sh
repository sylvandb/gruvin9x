#!/bin/sh
if [ -x /usr/local/bin/convert ]; then
  svn info | /usr/local/bin/convert \
    -background lightgrey -fill black \
    -pointsize 14 -font Courier \
    label:@- \
    -trim +repage -bordercolor lightgrey  -border 3 \
    -bordercolor black  -border 1 \
    svnversion.gif
else
  echo "ImageMagic utilities not present"
fi
