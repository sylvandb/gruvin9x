#!/bin/sh
LASTREV=`cat .svn/entries | sed -n '4p'`
THISREV=$(($LASTREV + 1))
if [ -x /usr/local/bin/convert ]; then
  svn info |\
    sed "s/Revision: $LASTREV/Revision: $THISREV/g" |\
    sed "s/Rev: $LASTREV/Rev: $THISREV/g" |\
  /usr/local/bin/convert \
    -background lightgrey -fill black \
    -pointsize 14 -font Courier \
    label:@- \
    -trim +repage -bordercolor lightgrey  -border 3 \
    -bordercolor black  -border 1 \
    svnversion.gif
else
  echo "ImageMagic utilities not present"
fi
