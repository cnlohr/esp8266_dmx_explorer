# esp8266_dmx_explorer

This project uses esp82xx.

Instructions:
(1) Flash in accordance with esp82xx, or flash using binaries. Note web/page.mpfs needs to be at 524288.
(2) Connect to the ESP8266 with it's new AP. Visit http://192.168.4.1.
(3) Reconfigure it to be on your wifi network using the wifi configuration.
(4) Reconnect to your own wifi. visit http://esp8266dmx.local
(5) Start Colorchord's "netlight-dmx.conf"

DMX Will now be spewed out your "TX" pin.  You may need to either put on a line driver, or connect TX to D+ on DMX, and put two resistors in series across 3.3v, and center-tap for 1.65v, and connect to D- on DMX.



