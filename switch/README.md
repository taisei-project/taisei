Taisei Switch Port
==================

<p align="center"><img src="icon.jpg"></p>

## Installation

### Grabbing Binaries

Download the latest release,
and extract the archive in the `/switch` folder on your SD Card. 
Then, run the game from the [hbmenu](https://github.com/switchbrew/nx-hbmenu)
using [hbl](https://github.com/switchbrew/nx-hbloader).

### Build dependencies

For building, you need the devkitA64 from devkitPro setup, along with switch portlibs and libnx. 
Documentation to setup that can be found [here](https://switchbrew.org/wiki/Setting_up_Development_Environment).

Other dependencies common to the main targets include:

    * meson >= 0.45.0 (build system; >=0.48.0 recommended)
    * Python >= 3.5
    * ninja

### Compiling from source

Run one of the following commands from the project root:

```
# release build
./switch/build.sh

# debug build
./switch/build.sh debug
```

NRO and assets will be in the `switch/out` folder.