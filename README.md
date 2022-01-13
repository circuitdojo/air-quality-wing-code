# Air Quality Wing Code!

![Render](images/render.png)

The Air Quality Wing (Previously Particle Squared) is a circuit board that helps you monitor the air quality around you. This repository is the code for the Air Quality Wing which enables you to monitor humidity, temperature, air particulates, and TVOC in one small package. You can use it with an Adafruit Feather compatible board. [For more information click here.](https://docs.jaredwolff.com/air-quality-wing/index.html)

[To get yours go here.](https://www.jaredwolff.com/store/air-quality-wing/)

*Update 10/27/2019:* This example now uses the Air Quality Wing Library! Search for `AirQualityWing` in ParticleWorkbench to install. It's also available on [Github](https://github.com/jaredwolff/air-quality-wing-library). Have fun!

## Quick Start:

**This quick start assumes you have [Particle Workbench](https://www.particle.io/workbench/) installed.**

1. Clone this repo onto a place on your machine: `git clone git@github.com:circuitdojo/air-quality-wing-code.git`
2. Init the library submodule using `git git submodule update --init --recursive`
3. Open the repo with Visual Code: (`code .` using the command line, or `file`->`open`)
4. Open `/src/AirQualityWing.ino`
5. Select your target device in the lower bar (Options are: `argon` and `boron`)
6. Hit `cmd` + `shift` + `p` to get the command menu
7. Select `Compile application (local)`. You can also choose `Cloud flash` as long as `board.h` has not been modified.
8. Enjoy playing around with your Air Quality Wing!

This example has been tested with DeviceOS 3.1.0. It's recommended to upgrade if you have problems.

## License

See included `LICENSE` file.
