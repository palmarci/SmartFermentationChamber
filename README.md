# Smart Fermentation Chamber

This is a SW + HW (and woodworking) project to create an insulated chamber with regulated temperature and humidity to perform food fermentation. This project is currently work-in-progress, please adapt the design for your needs and also make sure to double check the schematic/PCB before ordering.

## Table of contents
- [Table of contents](#table-of-contents)
- [Architecture](#architecture)
- [Firmware](#firmware)
- [PCB](#pcb)
- [Bill of materials](#bill-of-materials)
  - [Main components](#main-components)
  - [PCB parts](#pcb-parts)
- [Pictures](#pictures)
  - [Box](#box)
  - [Web server graphs](#web-server-graphs)
  - [ESP Web UI](#esp-web-ui)
- [TODOs](#todos)



## Architecture

The ESP32 gathers environmental data through sensors, evaluates it against target values, and manages the heater and humidifiers accordingly. This data is logged to a Python controller via MQTT, which generates graphs viewable through a web server. The ESP32 is configurable via a built-in web interface powered by [ESPUI](https://github.com/s00500/ESPUI). Additionally, the controller can send real-time alerts to a smartphone using the Telegram API. An optional 230V dimmer is available to regulate the heater's current based on the size of the heated space (setting is done via a potentiometer externally to the whole control circuit).

<picture>
    <source srcset="doc/architecture_dark.png"  media="(prefers-color-scheme: dark)">
    <img src="doc/architecture.png">
</picture>

## Firmware

It is a PlatformIO project. There is a [bug](https://github.com/lorol/LITTLEFS/pull/56
) in the external littlefs library, I had to fix it manually to get the FW compiling. The [fix](https://github.com/lorol/LITTLEFS/issues/43#issuecomment-1763347319
) is very easy to do.

## PCB

This is my first-ever PCB design, created in KiCad and manufactured by JLCPCB. Most of the routing was done using the auto-router, so please go easy on me. I made some mistakes, most notably I used an incorrect ESP devboard, these were fixed in 2.2, however I have not ordered those.

<img src="doc/pcb.png">

## Bill of materials

Prices are for one working unit with shipping, the date of prices is 2025.01. 
I ordered my parts from [Hestore](http://hestore.hu), if anyone needs to look up part numbers.

### Main components
|Name  |Link|Price |Description         |
|----------|-----|----|----------|
|*220V 300W Incubator heater Insulation-Thermostatic PTC ceramic air heater* | https://www.aliexpress.com/item/1005007503406690.html | 12 euro | You should buy one with a fan, I added one externally!<br> ![alt text](doc/heater.png)|
|*DC 5V Switchable Four Spray Humidifier Module Atomization Control Board DIY Ultrasonic Atomizer* | https://www.aliexpress.com/item/1005007493271175.html | 6 euro | Make sure to order one without a button, which powers on right after voltage is present<br><img src="doc/humidifier_modules.png">|
| NodeMCU ESP-32S development board | https://www.aliexpress.com/w/wholesale-nodemcu-32s.html | 6 euro | - | 
| PCB parts | - | 17 euro | *see BOM below* |
| PCB itself | https://jlcpcb.com/ | 22 euro *(for minimum order of 5 boards)* | I am not providing Gerber files, you will have to check the board/modify it if you need something, and just create the ZIP for yourself in KiCad!
| Wood + insulation | your local hardware store | max 20 euro | I don't remember the exact price
| *2000/4000W High Power Thyristor Electronic Voltage AC 220V Regulator Dimming Speed Temperature Regulation control switch* | https://www.aliexpress.com/item/1005006015316145.html | 5 euro | <img src="doc/dimmer.png"> |
| BME280 sensor | https://www.aliexpress.com/w/wholesale-bme280-sensor.html | 4 euro | <img src="doc/bme280.png"> |
| Waterproof DS18B20 Temperature Sensor | https://www.aliexpress.com/item/1005001601986600.html | 4 euro |<img src="doc/dallas_probe.png"> |
| **Total** | - | **~100 euro** | + cables, wires and misc soldering stuff |


### PCB parts

|Quantity  |Item name|Description         |
|----------|---------|--------------------|
|6         |1000 uF / 35V|Capacitor 12.5mm/5mm       |
|5         |LED 5 mm D-RD|LED                 |
|1         |1N4007   |Diode               |
|1|	? |4.7k Resistor            |
|3|	? |220R Resistor            |
|2|	?| 10k Resistor            |
|1|	? |1.5k Resistor            |
|2|	?|500R Resistor            |
|1         |?   |NPN transistor      |
|1         |BAT85    |Schottky-diode      |
|3         |ZL262-20SG|Pin header          |
|1         |IRLZ44NPBF|FET N               |
|1         |SRD-5VDC-SL-C|Relay, SPDT, 5V DC, 10A / 250V AC|
|1         |330uH / 1.2A / 630mR|Inductor            |
|1         |PTF/78   |Fuse holder, socket, 5x20mm|
|1         |BS232    |Fuse holder cover   |
|3         | ? |PCB terminal block, 90 degrees, 0.5cm, 2-pole|
|1         | ? |PCB terminal block, 90 degrees, 0.5cm, 4-pole|
|3         | ?|PCB terminal block, 90 degrees, 0.5cm, 3-pole|
|1         |XL2576S-5.0E1|Voltage stabilizer IC, both TO220 and TO263 fit |
|3         |5X20-F-3A|Fuse, fusible, glass, fast, 5x20mm, 250V AC, 3A|


## Pictures

### Box
<img src="doc/box_inside.jpg" width="50%">
<img src="doc/box_outside.jpg" width="50%">

### Web server graphs
<img src="doc/graph_food.png" width="50%"><br>
<img src="doc/graph_humidity.png" width="50%"><br>

### ESP Web UI
![alt text](doc/web1.png)<br>
![alt text](doc/web2.png)<br>

## TODOs
- went hole with a steppermotor  
- hum & temp updating at the same time bug