# MqttLoadSimulator

Simulate MQTT load / MQTT stress tester.

# Installation

''git clone https://github.com/halfgaar/MqttLoadSimulator.git
git clone https://github.com/emqx/qmqtt.git
cd qmqtt/
qmake qmqtt.pro
sudo make install
git clone 
cd MqttLoadSimulator/
cd MqttLoadSimulator/
sudo qmake MqttLoadSimulator.pro
make clean
sudo make install
ls
./MqttLoadSimulator --help''

# Requirements

It requires that [QMQTT](https://github.com/emqx/qmqtt) is installed. The project has a `make install` option, which will install the Qt module in the directory of the Qt version you built it, like `~/Qt/5.12.4/gcc_64`.

# Download builds

Builds are provided [here](https://www.flashmq.org/download/mqtt-load-simulator/)
