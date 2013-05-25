Hot-Babe
========

Cairo-rewrite and compositing version of Hot-Babe CPU Monitor.

For information on running, compile (using make) and run with -h.

For author information see COPYING.

For change information see Changelog and NEWS.

This code is licensed under the artistic license (see copyright and
LICENSE for details)

For bugs, questions, etc send me an email at allan@allanwirth.com


Configuration Files
-------------------

    /usr/local/share/hot-babe/
    /usr/share/hot-babe/
    ~/.local/share/hot-babe
    ./
    /

Default directory of hot-babes pictures. If a path is specified on the
command line (or in your configuration file) it will be search relative
to these directories in this order.

Note: Each pictures subdirectory must contain a file named "descr" which gives 
some info to the hot\-babe process. The first line gives the number of 
pictures to use, and then the name of the pictures, one per
line. Furthmore, the pictures must have the same dimensions, and in 
non-compositing mode 127 on the alpha channel is used for defining a 
bitmask for the window. The bitmask is created from the last image in
the animation.

Here is an example:

    5
    hb01_4.png
    hb01_3.png
    hb01_2.png
    hb01_1.png
    hb01_0.png
