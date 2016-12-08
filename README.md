# DoorLockCode
The arduino sketch to control the RFID door lock

# IDE

* [Download Energia Arduino IDE](http://energia.nu/download/) - Download a compiled application for Linux, Mac OS X, or Windows.

# Configuration

* Copy the rules file for the programmer
```
sudo cp 71-ti-permissions.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo service udev restart
```

* In Arduino IDE under Board: Set the Board to Launchpad w/ msp430g2553 (16 mhz)  

# Hardware

The Jumper from the programmer to the MSP430 labled RST needs to be connected during programming, then removed after programming. The system also needs to be power cycled after programming. 

# Adding/Removing cards

Cards are added and removed from this list:

https://github.com/Technocopia/DoorLockCode/blob/master/DoorLockCode.ino#L6 

And after the list is updated, the door must be reprogrammed. 
