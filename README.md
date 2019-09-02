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


## compiliation
```
g++ -c -std=c++11 -o main.o main.cpp
```

