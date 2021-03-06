# Jade Firmware

To build you can use the docker image (see Dockerfile) or install the esp-idf toolchain and repo following the commands in this readme.

# Use docker

Note the supplied docker-compose.yml assumes the Jade device is at
dev/ttyUSB0.

```
(local)$ docker-compose up -d
(local)$ docker-compose exec dev bash
(docker)$ cp configs/sdkconfig_jade.defaults sdkconfig.defaults
(docker)$ idf.py flash
```

The docker-compose.yml also mounts the local git repo so that it is the
origin of the repo in the docker.

# Set up the environment

Jade requires the esp-idf sdk.

More information is available in the [Espressif official guide](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/get-started/index.html).

Get the esp-idf sdk and required tools:

```
cd ~/esp
git clone -b v4.2 --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf && git checkout c40f2590bf759ff60ef122afa79b4ec04e7633d2 && ./install.sh
```

Set up the environmental variables:

```
. $HOME/esp/esp-idf/export.sh
```

# Build the firmware

```
mkdir $HOME/jade
git clone --recursive https://github.com/Blockstream/Jade.git $HOME/jade
cd $HOME/jade
cp configs/sdkconfig_jade.defaults sdkconfig.defaults
idf.py flash monitor
```

# Build configurations

There are various build configurations used by the CI in the configs/ directory, which may be required for specific builds eg. without BLE radio, with the screen enabled (or disabled, as with the CI tests), or for specific hardware (eg. the m5-fire).

The menuconfig tool can also be used to adjust the build settings.

```
idf.py menuconfig
```
Note: for any but the simplest CI-like build with no GUI, no camera, no user-interaction etc. it is recommended that PSRAM is available and enabled.  ( Component Config -> ESP-32 specific -> Support external SPI connected RAM )

# Run the tests

```
cd $HOME/jade
virtualenv -p python3 venv3
source venv3/bin/activate
pip install -r requirements.txt

python test_jade.py

deactivate
```

# License

The collection is subject to gpl3 but individual source components can be used under their specific licenses.
