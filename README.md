# MqttLoadSimulator

Simulate MQTT load / MQTT stress tester.

# Installation
To compile the program you can do the following

```sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install qt5-default

git clone https://github.com/halfgaar/MqttLoadSimulator.git
git clone https://github.com/emqx/qmqtt.git
cd qmqtt/
qmake qmqtt.pro
sudo make install
git clone 
cd MqttLoadSimulator/
sudo qmake MqttLoadSimulator.pro
make clean
sudo make install
ls
./MqttLoadSimulator --help```

# Requirements

It requires that [QMQTT](https://github.com/emqx/qmqtt) is installed. The project has a `make install` option, which will install the Qt module in the directory of the Qt version you built it, like `~/Qt/5.12.4/gcc_64`.

# Download builds

Builds are provided [here](https://www.flashmq.org/download/mqtt-load-simulator/)
