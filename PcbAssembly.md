# Building Your Own G9X PC Board #

## Preliminary Information ##

This page has been thrown together quickly to help you get started. The information is probably not well laid out and also incomplete in places.

## Where to get your PCB ##

The V4.1 PCB is now available for purchase at http://gruvin9x.com.

You are also welcome to use the Gerber files from the project source repository and have your own PCB's made.

## Where to get the parts ##

In short, DigiKey or Farnell (element14.com, outside the UK). If you're in the UK, Farnell is probably the cheaper choice (free shipping for online orders.) Everywhere else will probably have better luck with DigiKey.

For parts details, please see the three BOM (bill of materials) files at [archive/pcb-v4.1a-frozen/assembly](http://code.google.com/p/gruvin9x/source/browse/#svn%2Farchive%2Fpcb-v4.1a-frozen/assembly). (If you're using just a web browser, you'll need to use the "View raw file" link, found at the lower right of the page, after clicking a BOM file link, to download the Excel spreadsheet file.)

NOTE: The production version **RTC/SD-card board BOM** is included at the bottom of the main board BOM file.

The BOM files are Excel spreadsheet format. You can also view them in Open Office or using Google Docs.

Another (better) way to get the BOM files would be to do an svn check-out of the `pcb-v4.1a-frozen/assembly` folder, using the command (assuming you have Subversion installed) ...

```
svn co https://gruvin9x.googlecode.com/svn/archive/pcb-v4.1a-frozen/assembly g9x-assembly
```

... which will create a local folder named `g9x-assembly`.

## Construction Advice ##

**YOU _can_ solder 0.5mm pitch SMD parts. It's amazingly easy, once you know how!**

These parts were hand soldered in just minutes using an average soldering iron, with a typical hobbyist size tip, ordinary 0.7mm solder wire, no special flux and finally, some de-soldering braid. There's practically no challenge at all in reality, with similar results being easily repeated, time after time. You just need to know how! ...

![http://gruvin9x.googlecode.com/svn/wiki/PcbAssembly.attach/smd-parts-macro.jpg](http://gruvin9x.googlecode.com/svn/wiki/PcbAssembly.attach/smd-parts-macro.jpg)

(The board was washed with eco-friendly PC board cleaning solution (available from most electronics supply stores) to remove all flux residues. HINT: Don't judge your work until the board has been cleaned!)



**Be sure to watch these two videos, showing how to almost effortlessly solder tiny SMD parts ...**

<wiki:gadget url="http://gruvin9x.googlecode.com/svn/wiki/gcVideo.xml" up\_video="https://vimeo.com/108601753" width="640" height="384"/>

<wiki:gadget url="http://gruvin9x.googlecode.com/svn/wiki/gcVideo.xml" up\_video="https://vimeo.com/108526858" width="640" height="384"/>

There's also a very nicely made [video by Curious Inventor](http://www.youtube.com/watch?v=3NN7UGWYmBY) over at YouTube. The techniques are roughly the same. But some additional options and informational gems are offered.

### Parts Placement Guide ###

Look for PDF files [gruvin9x-SilkS\_Front.pdf](http://code.google.com/p/gruvin9x/source/browse/archive/pcb-v4.1a-frozen/assembly/gruvin9x-SilkS_Front.pdf) and [9xLABEL.pdf](http://code.google.com/p/gruvin9x/source/browse/archive/pcb-v4.1a-frozen/assembly/9xLABEL.pdf) (contributed, colour-coded version) in the [archive/pcb-v4.1a-frozen/assembly](http://code.google.com/p/gruvin9x/source/browse/#svn%2Farchive%2Fpcb-v4.1a-frozen%2Fassembly) folder.


Here's a reference photo of Gruvin's 2nd build. (Note that some of the larger capacitors here (22u, 47u) are ceramic, whereas the BOM parts list may call for tantalum. Either will be OK.) **Click the image for full-size view.**

<a href='http://gruvin9x.googlecode.com/svn/wiki/PcbAssembly.attach/v41-reference.jpg'><img src='http://gruvin9x.googlecode.com/svn/wiki/PcbAssembly.attach/v41-reference-small.jpg' /></a>


### LCD Padding Foam ###

No one seems to know where to get the foam used to pad between the PCB and the LCD display. However, the best solution I know of is to simply buy the HobbyKing.com [LCD back-light kits](http://www.hobbyking.com/hobbyking/store/uh_viewItem.asp?idProduct=16720) and use the appropriately thinner foam pad that comes with it. (Also available at HobbyKing in white and green coloured lighting.)

By the way, that is also the LED back-light intended for the upper right connector (K5) on the G9X board, with J1 pads 1&2 bridged, as in the above photo. BE SURE TO USE THE RIGHT-HAND TWO PINS of K5 for the LED back-light. (The left pin is full-current, 5 Volts.)


## AVR Fuse Bits ##

See fuses.txt in the source folder for further details. Briefly, ...

Preferred values for ATmega2560 in this project are:
> lfuse = 0xd7

> hfuse = 0x11

> efuse = 0xfc

There's a nice [online JavaScript AVR fuse calculator](http://www.frank-zhao.com/fusecalc/fusecalc.php?chip=atmega2560&LOW=D7&HIGH=11&EXTENDED=FC&LOCKBIT=FF), which you could use to see details or make any custom changes. (<-- that link loads with the correct fuse bits already pre-selected.)

BE CAREFUL not to select the wrong type of oscillator. You could end up 'bricking' the chip, which requires an external oscillator to be connected to get back to normal operation.


---


Archived v4.0 PCB assembly notes can be found [here](V40Assembly.md).