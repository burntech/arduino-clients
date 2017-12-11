    ##########

    Rev 3.0 - Timing - Sep 17, 2016
    Rev 4.0 - Libraries, Debug messaging, Communications, LEDs

    Oct 12 - updated callServer to work correctly
    OCt 19 - beginning work non-blocking LEDs
    Dec  4 - created carnival libraries
    May 2, 2017 - added on board button=poofer
    Sep 2, 2017 - added check in startPoof to confirm wifi.  added wifi override mode.
    Dec 2017 - eliminatating "blocking code (in poofer, leds) in favor of timed
     event sequences.  Improved performance. cleaned code.

    The Carnival code is meant to be a very low latency (<1ms) network of "effects",
    such as flame poofers, neopixels, or in theory, about anything you can hook up
    to a microcontroller.  It is all meant to be "real time", meaning no appreciable
    lag time to a human, and allow fine-grained asynchronous control of effects, such 
    as converting a music signal to analog, broadcasting the result to another effect
    that acts as a "fire equalizer".  Minimally, you can wireless control poofers and
    make them all go off at the same time, or in some other pattern.  It is also
    intended to insure secure and reliable control of the effect, and in particular
    with poofing fire, to always return to the default state of off after a pre-set
    time limit (maxPoofLimit, 8 seconds by default), and to always be instantly 
    available to be stopped with a kill switch.
     
    This code is represents a "client" for the Carnival server, allowing you
    to control a poofer wirelessly, in an asynchronous (non-blocking) manner - which
    is particularly important for controlling fire.  The ESP client can also read analog
    devices (like a poteniometer), and will soon have integrated control of Neopixels.

    At a minimum, using one uniquely named client per effect, and a server, will allow
    you to poof everything at the same time, poof if a round robbin, kill everything
    centrally... well you get the idea.
    
    The specific server is "xc-socket-server", running on a Raspberry Pi 3 configured 
    as an AP with a static IP.

    NOTE that you will need to copy the file library/Carnival/Carnvial_PW.h.default to
    Carnival_PW.h, and change the wifi name and password to reflect your server's values.

    ***


    If using an ESP8266 dev board (e.g. the Adafruit Huzzah or similar), if the
    code is installed and running, with a unique WHOAMI name, and your server is
    up and running, the blue LED should blink on and off slowly, then go solid
    blue, indicating it's now on the network.  Pushing "The Button" on your pi
    should make the red LED light up.
    
    Physically, hook up "BAT" and ground to a 5V power supply, hookup pin
    12 (or more - see allSolenoids) to the relay (which needs it's own power!
    Huzzah can't drive the relay very well if at all).

    If all connections are good, the blue light is steady on.  When receiving a
    communication, the blue LED may "sparkle".  If poofing, the red LED will be
    steadily on. Once a minute, we check the network and socket connections, and (if DEBUG),
    we blink blue 3 times if solid, or turn off the blue LED and start blinking the red LED
    if no connection or no socket.

    wifi override - device won't poof without wireless connection to server.  If it
    has an onboard button, you should be able to hold the button down when booting
    or restarting, and override the need for a server. 

    All complex behavior, such as a pattern of poofing, is handled asynchronously
    with timed events.  See the Carnival_poof.cpp library for examples of generating
    timed sequences.


    An important aspect of the design is the ability to kill a poofer instantly,
    either with a local physical switch, and/or via a remote wireless signal.

    
    You can use a second ESP, make it WHOAMI="B" (i.e. "The Button"), and now 
    have a wireless button (you need to kill the but-client on the server if you
    plan to organize things this way).





   INTERESTING:

   #include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library

   ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

