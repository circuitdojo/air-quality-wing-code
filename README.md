# Air Quality Wing Code! (Deprecated)

![Render](images/render.png)

The Air Quality Wing (Previously Particle Squared) is a circuit board that helps you monitor the air quality around you. This repository is the code for the Air Quality Wing which enables you to monitor humidity, temperature, air particulates, eC02 and TVOCs in one small package. You can use it with an Adafruit Feather compatible board. [For more information click here.](https://docs.jaredwolff.com/air-quality-wing/index.html)

[To get yours go here.](https://www.jaredwolff.com/store/air-quality-wing/)

*Update 1/8/2022*: This sample is now deprecated. Please see the [Zephyr samples](https://github.com/circuitdojo/air-quality-wing-zephyr-demo) for the latest supported working code.

*Update 10/27/2019:* This example now uses the Air Quality Wing Library! Search for `AirQualityWing` in ParticleWorkbench to install. It's also available on [Github](https://github.com/jaredwolff/air-quality-wing-library). Have fun!

## Quick Start:

1. Clone this repo onto a place on your machine: `git clone git@github.com:jaredwolff/air-quality-wing-code.git`
2. Open the repo with Visual Code: (`code .` using the command line, or `file`->`open`)
3. Open `/src/AirQualityWing.ino`
4. Select your target device in the lower bar (Options are `xenon`, `argon`, `boron`)
5. Hit `cmd` + `shift` + `p` to get the command menu
6. Select `Compile application (local)`. You can also choose `Cloud flash` as long as `board.h` has not been modified.
7. Enjoy playing around with your Air Quality Wing!

## License

See included `LICENSE` file.
