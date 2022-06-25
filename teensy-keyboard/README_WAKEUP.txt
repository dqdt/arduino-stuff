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