### **Part 1.** shutdown XBMC for faster compiling and update the feeds:   
`sudo initctl stop xbmc`  
`sudo apt-get update`  

### **Part 2.** install necessary dependencies:   
`sudo apt-get install libusb-1.0 make gcc g++ git libboost-dev libtool`   

### **Part 3.** Clone the optimized source:   
`cd ~`   
`git clone git://github.com/hebaishi/boblightd-for-raspberry.git`   

### **Part 4.** Compile:   
`cd boblightd-for-raspberry/`   
`./configure --prefix=/usr`   
`make;sudo make install`   
Now take a cup of coffee and wait till the compiling is finished.      

**Now you are finished. I hope you enjoy this version.**


**READ THIS: This version needs another lightnames in boblight.conf, only 3 chars not more and not lower**

[light]
name **1** << **This is wrong, it must be XX1 or 001 or 1XX**   
color red ambilight 1   
color green ambilight 2   
color blue ambilight 3   
hscan 30 35   
vscan 90 100   

## That's all folks!!
