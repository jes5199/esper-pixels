# EsperPixels: OpenPixelControl on ESP8266 (for WS2812/NeoPixels over WiFi)

This is an early draft, but it works. Due to hardware limitations, the ESP8266
can only reliably use two IO pins to control WS2812 pixels reliably while also
using WiFi.

Requirements:
  * ESP8266 board, I recommend the HiLetgo Development Board version, $9 on Amazon, includes a USB port.
  * NeoPixels, or compatable LEDs. Current code expects the 3-color species, but 4-color may work anyway.
  * recent Arduino environment (1.8.2 works for me)
  * "ESP8266 Community" Arduino board package: https://github.com/esp8266/Arduino (2.3.0 works for me)
  * "NeoPixelBus by Makuna" package: https://github.com/Makuna/NeoPixelBus (2.2.7 works for me)

Currenly, this code assumes you have up to 512 pixels connected on one or both of
GPIO3 (possibly labelled "RX" or "D10") and GPIO2 (possibly labelled "D4").
GPIO3 is using DMA to controll the output, while GPIO2 is using the UART. DMA seems
to be slightly more reliable - I've seen occational visual glitches on UART/GPIO2,
so if you have only one strand of LEDs, prefer using GPIO3.

When powered on, the ESP8266 will try to connect to the WiFi network specified by
`ssid` and `password`, and will also open its own WiFi network with a name like
"Esper✨123456" based on the ESP8266's internal serial number, using the password
specified by `esper_password`.

EsperPixels will listen on port 7890 on both networks, and will only allow one
client to connect at a time. It expects to receive OpenPixelControl messages
without any extensions (see http://openpixelcontrol.org/ for spec.)
The ESP8266 can only handle a moderate amount of network traffic without slowing down,
so don't send more pixel values than you actually need - but updating all 1024 pixels at
30 frames per second is totally fine. It should be possible to control a lot
more pixels if you can stand a lower framerate. (Or just get another ESP8266&mdash;
they're cheap!)

I've been able to power up to 210 pixels directly off of USB power using the
HiLetgo board's 3.3v voltage regulator as the only power source. If you need
many more pixels than that, you should consider a more reliable power supply.
(If you plug in 300 pixels, the ESP8266 won't even boot up, I haven't done the
experiment to figure out where the cutoff point is exactly.)

If you want to run pixels at 5v, you'll need to use a level shifter. I haven't
personally tried to do that.
