# Particle Powered PM2.5 Air Quality Sensor

Connecting all the sensors.

I'm working with a Silicon Labs Si7021, AMS CCS811, and Honeywell HPM Series PM2.5 sesnor.

Luckily lots of the work has been done for me by Particle. I just need to wire everything together.

## Wiring it all together

[ ] TODO: schematic of "prototype" setup

## Plumbing the firmware

It's been a while where I've coded in C++ but I tried to keep the same conventions that I would have using C code.

### Si7021

The best say to read the temperature/humidity sensor is by issuing a blocking command. This is not ideal but unfortunately, the Si7021 does not have a interrupt pin.

As described in the datasheet, you first write the command and then attempt to read directly from the device. It will stretch the clock until ready to be read. There are some settings that can be tweaked, like how much current to sink through the heater, but i've left those things alone for this example.

### The CCS811

I am a big fan of AMS. They're a good company that make solid products. I used them in a previous life building smart cups.

This particular chip gives me a little more wiggle room to play and keep everything as asyncronous as possible. It includes a reset pin, wake pin and interrupt pin. The most important being the interrupt pin. Interrupt pins make it extremely easy to asycronously read from a device once it's ready to share a measurment. That's what we'll do here.

### The HPMA115S0-XXX

I originally chose this sensor a while back for it's enclosed nature. It was, in my option, easier to integrate becuase as an electrical engineer I dont want to worry about board mounted solutions that require more design effort.

