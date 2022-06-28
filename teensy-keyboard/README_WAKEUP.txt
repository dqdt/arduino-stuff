Back to: How can a Teensy wake the computer from sleep?


According to the USB HID spec, a device can be configued to have "Remote Wakeup"
  modify the configuration descriptor -- set a bit
  (then, we can see in Device Manager that it has Power Management -> Remote Wakeup)

TIL:
  https://forum.pjrc.com/threads/26649-I-woke-my-Mac-from-sleep-with-a-teensy-3-0

On the Teensy LC, 
  in the USBx_CTL register is a RESUME bit
    software sets it to 1 for some time, and the clears it

  RESUME = upstream (device tells the hub)
  RESET = downstream (hub tells devices)

The wakeup code works -- but the keyboard freezes for a sec when the RESUME SIGNAL is sent
  
  I think it should only send the RESUME SIGNAL when it knows the computer is asleep...

  Normal keyboards seem to turn off when the computer is sleeping (lights off)
    but it's probably a [fake?] low-power mode, as they can sill send the RESUME.
  How do they know when the computer went to sleep? Must be a signal...

  https://microchipdeveloper.com/usb:reset-suspend-resume
    SUSPEND?

USBx_ISTAT register has a SLEEP and RESUME bit
	https://www.nxp.com/docs/en/application-note/AN5385.pdf


OK, i enabled both the SLEEP and RESUME interrupts, but now the keyboard freezes...
	USB0_INTEN |= USB_INTEN_RESUMEEN | USB_INTEN_SLEEPEN;
	...
	if (USB0_ISTAT & USB_ISTAT_SLEEP)
    {
        r = 237;
    }
    if (USB0_ISTAT & USB_ISTAT_RESUME)
    {
        g = 237;
    }

If the keyboard receives SLEEP or RESUME signal, it's indicated on the LEDs.

How about just SLEEP?
	USB0_INTEN |= USB_INTEN_SLEEPEN;
	The teensy has gotten a RESUME signal. 
	  but the teensy library is using that?

    IRQ_USBOTG
    usb_isr() function is there...

Guess:
	USBx_ISTAT register calls the usb_isr() to service the interrupt
	we have to clear all bits in the register or else the ISR will be called again

	RESUME was not included in the usb_isr()
	  so when i enabled it, the isr got stuck

	SLEEP was included in usb_isr(), but it also cleared the bit so I couldn't see it
	  outside, in my main function...

Fortunately, the USBx_ISTAT RESUME and SLEEP bits are still set, even when their
  interrupts are not enabled. But the bits should be cleared in case the events
  happen again.


So:
	Remove the lines related to USB_INTEN_SLEEPEN and USB_ISTAT_SLEEP in usb_dev.c





Actually,
  the Teensy detects SLEEP when powered on, 
    (? because the computer's USB HUB is not yet ready)
    and there is no RESUME when the computer is booted (only when waking)

Weird, if the Teensy tries to RESUME when the usb bus is normal (not sleeping)
  then it gets put into a SLEEP state...

  (sometimes the computer thinks the device is malfunctioning)
  (if there are too many RESUME signals coming from the keyboard for a few seconds?)
  (at this point, it stops showing up in Device Manager)

SO:
  The keyboard can detect when the bus is SLEEPing through interrupts.
  If RESUME is detected, we know the computer isn't asleep anymore.

  If sleeping, and a key is pressed, try to RESUME?
  You only get one shot to RESUME?

    My mouse seems to behave this way. If I put the computer to sleep and then
      wiggle the mouse constantly, then wake the computer (with the keyboard, since
      the mouse is not functional anymore), the mouse "stops working".
      (Probably because the MOUSE was sending lots of RESUME signals?)

  If the computer is waking from sleep, we get a RESUME
  If the computer is booting, there is no RESUME
    Could try to RESUME once, then, internally, leave the "sleep state"
    
  There must be some indicator?


Found it!
  https://forum.arduino.cc/t/leonardo-as-keyboard-does-not-wake-windows-7-from-sleep/146624/5

  usb 2.0 spec
  The host sends SET_FEATURE(DEVICE_REMOTE_WAKEUP) before suspending the port
    Feature Selector: DEVICE_REMOTE_WAKEUP
           Recipient: Device
               Value: 1

in usb_dev.c, in usb_setup(void),

  union {
   struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
   };
    uint16_t wRequestAndType;
  };

  -> the upper 8 bits are bmRequest, and lower 8 bits are bmRequestType

now change:

  switch(setup.wRequestAndType)

    case 0x0080: // GET_STATUS (device)
    // reply_buffer[0] = 0;
       reply_buffer[0] = 2;  // usb_20.pdf, page 255, GET_STATUS, bit 1 indicates the ability of the device to signal remote wakeup.


    case 
      bmRequestType = 0 (Device?)
      bRequest = SET_FEATURE = 3

      setup.wValue = DEVICE_REMOTE_WAKEUP = 1


    // added
    case 0x0300: // SET_FEATURE device
        if (setup.wValue == 1) {
      device_remote_wakeup_feature = 1;
    } else {
      endpoint0_stall();
    }
        break;
    // end

OK:
  When the keyboard is plugged in,
    Computer not turned on:  suspended
    Computer is on:          not suspended

  When the computer goes to sleep:
    suspended, but also sends DEVICE_REMOTE_WAKEUP
    use the remote_wakeup to put it in a "suspended" state

  If the computer shuts down:
    suspended, but ... also sends a DEVICE_REMOTE_WAKEUP.

  Restart computer:
    suspended because of the DEVICE_REMOTE_WAKEUP and no way to automatically
      get it out of suspended state. hmm...

There is a falling edge when sleep or turn off
There is a rising edge when waking, but not when turned on



changes in usb_dev.c:

extern these variables in usb_dev.h, usb_keyboard.h so it can be used in main
uint8_t suspended = 0;
uint8_t device_remote_wakeup_feature = 0;

enable USB_INTEN_SLEEPEN, USB_INTEN_RESUMEEN
  // added
  
  suspended = 0;  // ?? any activity gets it out of SUSPEND

  if ((status & USB_ISTAT_SLEEP)) {
    
    suspended = 1;
    USB0_ISTAT = USB_ISTAT_SLEEP;
  }

  if ((status & USB_ISTAT_RESUME)) {
    
    suspended = 0;
    device_remote_wakeup_feature = 0;  // don't accidentally send
    USB0_ISTAT = USB_ISTAT_RESUME;
  }
  // end
 

in main:
if (suspended && device_remote_wakeup_feature)
{
    if (state_changed[0] || state_changed[1])
    {
        device_remote_wakeup_feature = 0;  // one shot at sending RESUME
        wakeup();
        continue;
    }
}