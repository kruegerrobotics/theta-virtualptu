# virtual ptu

## Scope

This project is experimental in the domain of robotics. To look around with a (remote operated) robot we have often mounted the camera on a pan and tilt unit (ptu). This units consists of 2 motors, mechanics software to control etc. The idea behind this project is to use a 360 degree camera (here the ThetaV from RICOH) to have a single unit mounted on a robot and via software we could select which segement is streamed to the user - we can not stream the the full 4K surround (even compressed) since we have often to do with bandwith restricted connections. This way we would not need a motorized unit

This also has to be fully automated with no user interaction. For the ThetaV we need a program that:
1. wakes the device up
2. switches to live-streaming mode
3. decodes the 4K h264 coming from the camera
4. based on parameters selects a sub-section of the image
5. encodes the sub-section and sends/displays it to the remote operator

In this experimental project we want to investigate steps 1-4 and display the image locally. Especially to get *a feeling* for the latency of aquisition, decoding (with multiple decoders), and display since for remote operated robotics this is essential.

**WARNING**: this is work in progress, certain elements are hardcoded and not all works robust. We also know alreadt that with the current firmware version 3.70 of the thetaV wake up seems to be unlikely - nevertheless we want to give it a shoot and ideas etc. are welcome. 

### Demo executable

The generete executable takes input from a joystick and pans and tils a 640x480 image segment from the livestream and displays it and similar to pre-recorded 360 pictures or videos the user can look around in the livestream

## How to build

This project depends on other projects. In this stage we are also lacking perfect package finding, debug and release libs (also for some dependencies) but it is on the TODO list

### pre-reqrusites

#### gstreamer

This is quite a dependency, it might be good to start with, but probaly needs also the "ugly plugins" for the h264 decoder

``` bash
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

#### libusb

On Debian systems

``` bash
sudo apt install libusb
```

#### libuvc-theta

This [libuvc-fork](https://github.com/ricohapi/libuvc-theta) is essential for the videostreaming from the theta. It is important to check that this program gets linked against this libuvc-theta, otherwise it will not work.

``` bash 
git clone https://github.com/ricohapi/libuvc-theta.git
cd libuvc-theta
mkdir build
cd build
cmake ..
make && sudo make install
```

#### libthetacontrol and its dependecies

This [library](https://github.com/kruegerrobotics/libthetacontrol) was created (initially part of this project) to control the theta more convinietly. It comes with dependencies on its own. Please follow the instructions there as well... But in brief the would be

##### libptp2 fork

``` bash
git clone https://github.com/kruegerrobotics/libptp2
cd libptp2
mkdir build
cd build
cmake ../
make 
sudo make install
```

##### libthetacontrol

``` bash
git clone https://github.com/kruegerrobotics/libthetacontrol.git
cd libthetacontrol
mkdir build
cd build
cmake ../
make
sudo make install
```



