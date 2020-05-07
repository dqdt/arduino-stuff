Current issues:
* How does Teensy know if the computer went to sleep?
  * Teensy is still powered when the computer is sleeping.
* How to get the Teensy keyboard to wake up the computer?

  * https://forum.arduino.cc/index.php?topic=150157.0
  * In the configuration descriptor, `bmAttributes` bit 5 is the Remote Wakeup bit.
    * It didn't work. There's more?
  * https://forum.pjrc.com/threads/55044-Teensy-2-0-keyboard-sleep-and-wake-PC?highlight=remote+wakeup
    * no replies
  * https://forum.pjrc.com/threads/26388-Can-a-Teensy-3-x-detect-when-my-computer-sleeps-and-wakes
    * There's a "messageType" (?) for "SystemWillSleep" or "SystemWillPowerOn"?
      * these might be specific to apple...
  * https://forum.pjrc.com/threads/58325-Question-about-Arduino-Keyboard-library-and-USB-sleeping
    * There is USB Selective Suspend setting on windows:
      * https://answers.microsoft.com/en-us/windows/forum/windows_vista-hardware/what-does-usb-selective-suspend-mean/71fa747b-914f-4d17-b476-cf4bb1bb783c
      * A USB device driver can send a message to windows telling it to idle the device.  This puts the device in a low-power state (the suspend state).  When a USB device is suspended, windows does not wait for it to respond before entering a sleep or hibernate mode.  If you disable this feature, the system will simply return a failure to the driver when the driver attempts to enter suspend state.  If the driver complies with Microsoft guidelines, it will simply retry the idle request at every expiration of its idle timer.  
  * https://beyondlogic.org/usbnutshell/usb6.shtml
    * WAKEUP is a device request?
    * Set/Clear feature for device: DEVICE_REMOTE_WAKEUP or TEST_MODE
    * Set/Clear feature for interface: 
    * Set/Clear feature for endpoints: ENDPOINT_HALT