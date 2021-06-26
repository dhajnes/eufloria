# Eufloria v.2
Eufloria is a project for automatic irrigation and sensing of the state of home-grown plants.

## Introduction
The goal of this project is to experience the hands-on development process ranging from the code running on arduino and the MCU for wifi, through mobile application design for checking on how are your plants doing, automatic control of the irrigation based on the surrounding conditions to designing and soldering the actual circuit and possibly designing the actual PCB with the mounted chipset.

## Goals
The goal of this project is to go through the whole process from definition through design to implementation and testing.:
This can be formalized to:
1) define the system (plant's) variables
2) define all of the needed hardware
3) design and test automatic control (when to irrigate based on the plant variables)
4) data gathering (sd card) and data reporting (some easy indicator - LED or a small display)
5) wi-fi utilization for data transfer to an app through the local wi-fi or possibly through
an external web server (and then to the app) - the data may be shown either on an internet webpage or in the app or both
6) solder the whole setup on a conductive plate all together, model a case (a 3D printer is available)
7) PCB design, just to try out the process

## What has been done yet.
The result of the first version can be seen in the following video: 

[![Eufloria v.1 on STM32](https://i.imgur.com/QEbxguM.png)](https://www.youtube.com/watch?v=mrgRaGwNv90 "Eufloria v.1 on STM32")

The first version was running on STM32 chip, which was really limited input/output wise. In this version an interconnection between an Arduino and an MCU Wi-Fi module is planned.
Scheme can be seen here:

![Eufloria v.1 on STM32 in KiCAD](https://i.imgur.com/grU0Chy.png)


