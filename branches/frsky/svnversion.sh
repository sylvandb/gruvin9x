#!/bin/sh
if [ -x /usrloca/bin/convert -o -x /opt/local/bin/convert ]; then
  svn info | grep -v "Path:" | grep -v "Node Kind:" | grep -v "Schedule:" |\
    convert \
    -background lightgrey -fill black \
    -pointsize 14 -font Courier \
    label:@- \
    -trim +repage -bordercolor lightgrey  -border 3 \
    -bordercolor black  -border 1 \
    svnversion.gif
else
  echo "ImageMagic utilities not present"
fi
