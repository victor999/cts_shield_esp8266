# cts_shield_esp8266

Energy monitor for measuring energy consumption.
Data logged to openenergymonitor.org - you need to open account there. Alternatively you can clone emoncms to your hosting and send data to your emoncms.
Used sensor SCT013-30 for measuring current. Voltage is constant and can be set on the web page.

Project page: https://hackaday.io/project/8505-energy-monitor-with-arduino

ESP8266 used as web server with possibility to set different parameters. When there is no Wifi, ESP creates access point, where you can choose Wifi network and password. Wifi network remembered after restart.

Libraries used in sketch (you should install them):

https://github.com/adafruit/Adafruit_SSD1306

(modify H file: enable #define SSD1306_128_64 and disable #define SSD1306_128_32)

https://github.com/adafruit/Adafruit-GFX-Library

https://github.com/openenergymonitor/EmonLib

https://github.com/tzapu/WiFiManager
