# install latest Arduino IDE and required libraries on Linux

cd $HOME
sudo -v
sudo apt update && sudo apt full-upgrade -y && sudo apt autoremove -y
sudo apt install build-essential openssh-server git ubuntu-make -y
sudo usermod -a -G dialout $USER
umake ide arduino
mkdir Arduino && cd Arduino
mkdir hardware && cd hardware
mkdir esp8266com && cd esp8266com
git clone https://github.com/esp8266/Arduino.git esp8266
cd esp8266
./tools/boards.txt.py --nofloat --allgen
cd tools
python get.py
cd ~/Arduino
git clone https://github.com/dragondaud/myClock.git
mkdir libraries
cd libraries
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arcao/Syslog.git
git clone https://github.com/adafruit/Adafruit-GFX-Library.git
git clone https://github.com/2dom/PxMatrix.git
git clone https://github.com/tzapu/WiFiManager.git
cd ~/Arduino/myClock
