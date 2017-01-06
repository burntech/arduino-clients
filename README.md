Two different Arduino-style clients for an Adafruit ESP8266 (the Huzzah), or
similar (e.g. NodeMCU, and etc.).

1) Symlink the Carnival-clients directory into your sketchbook directory.

2) Symlink the "Carnival" folder inside the library folder to your
sketchbook->library folder (or wherever you keep your Arduino libraries).

Do note that in the library, there is a file Carnival_PW.h.default.  Make
a copy of this file, Carnival_PW.h, and enter the appropriate network
information for you ESP to connect to.  By default, the file will (when copied
or renamed) connect to the BurnTech network provided by the [burntech rPi image] 
(https://github.com/burntech/burntech-image/)

3) NOTE:  you need to be running the ESP8266 board version 2.1.0 (or less)
versions 2.2.0 and 2.3.0 do not correctly connect to a wireless AP.  Use the 
"Boards Manager" under "Boards" to install the correct board version.



