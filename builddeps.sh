#!/bin/bash

sudo apt-get install scons python-dev swig python-smbus python-pil python-pyaudio
git clone https://github.com/jgarff/rpi_ws281x.git
pushd rpi_ws281x
scons
cd python
sudo python ./setup.py install
popd
#TODO: CLone and fix OE pin?  Only if we can get the ws2811 to run together
git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
pushd rpi-rgb-led-matrix
make
pushd rpi-rgb-led-matrix/bindings/python
sudo make install-python
