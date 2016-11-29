# pHerminator: Arduino pH and gas monitoring fermentation
---

**Table of Contents** 

- [Purpose](#purpose)
- [Features](#features)
- [Materials and Wireing](#materials-and-wireing)
- [Operation](#operation)
  - [Single sector mode](#single-sector-mode)
  - [Multi sector mode](#multi-sector-mode)
 - [Future improvements and notes](#future-improvements-and-notes)

## Purpose
At my [work](http://www.chr-hansen.com/) we ferment a lot of milk making either yoghurt or cheese. This involves expensive equipment, often only pH is meassured untill a given piont where the process is stopped. This is an attepmt to make a cheep alternative based on an arduino which also measures gasses and provide a realtime interface using [thingspeak](http://thingspeak.com/) channelid 96629.

## Features
* Serial interactive menus
* Callibration og pH probe and gas sensors
* Serial repporting
* Logging in realtime to thingspeak

## Materials and Wireing
* Arduino Uno
* [Liquid PH Value Sensor with BNC Electrode Probe](https://www.dfrobot.com/index.php?route=product/product&product_id=1025)
* Selection of gas sensors (I used 4 from a kit with cheep MQ-sensors) 
  * MQ4 for alcohol
  * MQ7 for carbonmonooxide
  * MQ8 for hydrogene
  * MQ135 for ammonia
* Waterproof Plastic Electronic Project Box Enclosure Case 100×68×50MM (ebay)
* Optional a raspberry pi for online monitoring and logging

![assembly](https://raw.githubusercontent.com/SeniorWizard/pherminator/master/assembly.png)

Drill holes in the project box for gas sensors and BNC. Wireing is a lot simpler than it looks, ground and 5v is wired to all the sensors (blue and brown) then each sensors analog outputis connected to an analog pin on the arduino. 

![wireing](https://raw.githubusercontent.com/SeniorWizard/pherminator/master/wires.png)

## Operation



![setup](https://raw.githubusercontent.com/SeniorWizard/pherminator/master/setup.png)

![dataoutput](https://raw.githubusercontent.com/SeniorWizard/pherminator/master/dataoutput.png)

![yoghurt](https://raw.githubusercontent.com/SeniorWizard/pherminator/master/youghurt.png)



## Future improvements and notes


