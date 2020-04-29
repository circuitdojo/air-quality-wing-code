# Air Quality Wing for Particle Mesh Code!

![Render](images/render.png)

The Air Quality Wing for Particle Mesh (Previously Particle Squared) is a circuit board that helps you monitor the air quality around you. This repository is the code for the Air Quality Wing which enables you to monitor humidity, temperature, air particulates, eC02 and TVOCs in one small package. You can use it with an Adafruit Feather compatible board or the newest mesh enabled boards from Particle. [For more information click here.](https://www.jaredwolff.com/particle-squared-air-quality-sensor/)

[To get yours go here.](https://www.jaredwolff.com/store/air-quality-wing/)

*Update 10/27/2019:* This example now uses the Air Quality Wing Library! Search for `AirQualityWing` in ParticleWorkbench to install. It's also available on [Github](https://github.com/jaredwolff/air-quality-wing-library). Have fun!

## Quick Start:

1. Clone this repo onto a place on your machine: `git clone git@github.com:jaredwolff/air-quality-wing-code.git`
2. Open the repo with Visual Code: (`code .` using the command line, or `file`->`open`)
3. Open `/src/AirQualityWing.ino`
4. Select your target device in the lower bar (Options are `xenon`, `argon`, `boron`)
5. Hit `cmd` + `shift` + `p` to get the command menu
6. Select `Compile application (local)`. You can also choose `Cloud flash` as long as `board.h` has not been modified.
7. Enjoy playing around with your Air Quality Wing!

## Getting the BSEC library included with this project

If you're using the BME680 you'll have to include the static library provided by Bosch. [Download the goods here first.](https://www.bosch-sensortec.com/bst/products/all_products/bsec)

For this project (on any Particle Mesh board) you will want the `libalgobsec.a` in `BSEC_1.4.7.3_Generic_Release/algo/bin/Lite_version/gcc/Cortex_M4F`

Place `libalgobsec.a` in `.particle/toolchains/deviceOS/<verison>/firmware-<verison>/build/target/user/platform-12-m/pm25`. Depending on the platform you may have to change `platform-12-m/` to `platform-14-m` etc.

Add these to the `makefile` in `.particle/toolchains/deviceOS/<version>/firmware-<version>/modules/xenon/user-part` for the Particle stuff to work.

```
LIB_DEPS += $(USER_LIB_DIR)/libalgobsec.a
LDFLAGS += -Wl,--whole-archive $(USER_LIB_DIR)/libalgobsec.a -Wl,--no-whole-archive
```

## Particle Basics

#### ```/src``` folder:
This is the source folder that contains the firmware files for your project. It should *not* be renamed.
Anything that is in this folder when you compile your project will be sent to our compile service and compiled into a firmware binary for the Particle device that you have targeted.

If your application contains multiple files, they should all be included in the `src` folder. If your firmware depends on Particle libraries, those dependencies are specified in the `project.properties` file referenced below.

#### ```.ino``` file:
This file is the firmware that will run as the primary application on your Particle device. It contains a `setup()` and `loop()` function, and can be written in Wiring or C/C++. For more information about using the Particle firmware API to create firmware for your Particle device, refer to the [Firmware Reference](https://docs.particle.io/reference/firmware/) section of the Particle documentation.

#### ```project.properties``` file:
This is the file that specifies the name and version number of the libraries that your project depends on. Dependencies are added automatically to your `project.properties` file when you add a library to a project using the `particle library add` command in the CLI or add a library in the Desktop IDE.

## Adding additional files to your project

#### Projects with multiple sources
If you would like add additional files to your application, they should be added to the `/src` folder. All files in the `/src` folder will be sent to the Particle Cloud to produce a compiled binary.

#### Projects with external libraries
If your project includes a library that has not been registered in the Particle libraries system, you should create a new folder named `/lib/<libraryname>/src` under `/<project dir>` and add the `.h`, `.cpp` & `library.properties` files for your library there. Read the [Firmware Libraries guide](https://docs.particle.io/guide/tools-and-features/libraries/) for more details on how to develop libraries. Note that all contents of the `/lib` folder and subfolders will also be sent to the Cloud for compilation.

## Compiling your project

When you're ready to compile your project, make sure you have the correct Particle device target selected and run `particle compile <platform>` in the CLI or click the Compile button in the Desktop IDE. The following files in your project folder will be sent to the compile service:

- Everything in the `/src` folder, including your `.ino` application file
- The `project.properties` file for your project
- Any libraries stored under `lib/<libraryname>/src`

## License

See included `LICENSE` file.
