## PREEMPT_RT Kernel 4.19.59
```
ubuntu 16.04
cd /usr/src
wget https://www.kernel.org/pub/linux/kernel/v4.x/linux-4.19.59.tar.xz
wget https://www.kernel.org/pub/linxu/kernel/projects/rt/4.19/patch-4.19.59-rt24.patch.xz
xz -cd linux-4.19.59.tar.xz | tar xvf -
cd linux-4.19.59
xzcat ../patch-4.19.59-rt24.patch.xz | patch -p1
cp -v /boot/config-$(uname -r) .config
sudo apt-get install build-essential libncurses-dev bison flex libssl-dev libelf-dev bc
make menuconfig
CONFIG_PREEMPT_RT_FULL
make -j 5
sudo make modules_install
sudo make install
sudo vi /etc/default/grub
GRUB_DEFAULT='4.19.59-rt24'
sudo update-grub
sudo reboot
```


## OpenCV 3.4.6
```
sudo apt-get install build-essential
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
git clone -b 3.4 https://github.com/opencv/opencv_contrib.git
wget https://github.com/opencv/opencv/archive/3.4.6.zip
unzip 3.4.6.zip
cd opencv-3.4.6
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local -D OPENCV_EXTRA_MODULES_PATH=/home/rubis/opencv_contrib/modules ..
make -j7
sudo make install
echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib" >> ~/.bashrc
source ~/.bashrc
```

### Lane detection
```
cd ~
cd opencv_tests
g++ -o track tracker.cpp -L/usr/local/lib `pkg-config --libs --cflags opencv` -std=c++11 -fopenmp
./track lane
```


## Darknet

```
git clone https://github.com/pjreddie/darknet.git
cd darknet
make
wget https://pjreddie.com/media/files/yolov3-tiny.weights
```

### Object detection
```
./darknet detect cfg/yolov3-tiny.cfg yolov3-tiny.weights data/dog.jpg
feh predictions.jpg
```


## compiliation
```
g++ -c -std=c++11 -o main.o main.cpp
```

