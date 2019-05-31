The current working file is Carnival-clients/huzzahWithLibrary4.  Remaining
folders are various tests and older files.  This is a .ino file, and should
work on something like an Adfruit Huzzah, or the NodeMCU and the Arduino IDE.


1) Be sure and symlink the Carnival-clients directory into your sketchbook 
directory.

2) Also symlink the "Carnival" folder inside the library folder to your
sketchbook->library folder (or wherever you keep your Arduino libraries).

Do note that in the library, there is a file Carnival_PW.h.default.  Make
a copy of this file, Carnival_PW.h, and enter the appropriate network
information for you ESP to connect to.  By default, the file will (when copied
or renamed) connect to the BurnTech network provided by the [burntech rPi image] 
(https://github.com/burntech/burntech-image/)

3) To get best results at this point, you should be using at least the ESP 
core 2.4.0, better still is using the current version on github:

[https://github.com/esp8266/Arduino](https://github.com/esp8266/Arduino)



