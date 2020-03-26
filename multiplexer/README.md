HC4067 16-Channel Analog Multiplexer/Demultiplexer
* 4 select bits. Connects one of the 16 channels to the common input/output pin.
  * The signal can go both ways? (How does the switching work internally?)
* Active-low enable
  * When disabled, it's floating.

This multiplexer uses "Break-Before-Make" switching.
* https://www.digikey.com/en/articles/switch-tutorial

* "make-before-break", or "shorting"
  * When the switch moves from one position to another, there is never an open circuit. There are times when two outputs are shorted.
  
* "break-before-make", or "non-shorting"
  * The circuit is broken (open) while the switch is moving to the new position.
