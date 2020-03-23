### "KY-040" rotary encoder
* https://www.aliexpress.com/item/32991511891.html?spm=a2g0s.9042311.0.0.51284c4d3Oq38L

### How *this* rotary encoder works
See this [PDF](Rotary%20Encoder.pdf).

I tried to google a teardown but couldn't find much.
* http://practicalusage.com/side-project-understanding-cheap-rotary-encoders/

However, I think the rotary encoder in my mouse's scroll wheel uses the same mechanism.

![](deathadder_rotary_encoder.png)

The rotary encoder has three external pins: `pinA`, `pinB`, and `GND`. Internally, there is a circular track with metal pads; some pads connect to `pinA`, some to `pinB`, or `GND`. There's also a disk with metal leaves/legs and these legs may touch certain pads as the disk rotates. The disk acts as a "switch" that can pull `pinA` and/or `pinB` to `GND` when "closed". When the "switch" is open, the pin is at `V+`. 

* Both pins have a `10K` pullup resistor so it doesn't short-circuit when pulled down.
  * when both pins are pulled to `GND`, the resistors are in parallel (`5K`)

When you turn the knob, there's a *bump* that provides some resistance, but this bump also aligns the disk so that both "switches" are both open or closed (i.e. the same state). In the transition between two "resting spots", one of the pins will change state before the other pin, depending on if the knob was turned clockwise or counterclockwise. Reading the pin states allows us to detect rotation.

* A lot of tutorials describe the rotary encoder as outputting two square wave signals (or pulses) and being "quadrature" signals (90 degrees out of phase), but I think it's confusing to think about it that way. (It makes sense if it was rotating at a constant speed...) It's just two signals making a `LOW` to `HIGH` (or vice versa) transition, but one signal is delayed. The delay between edges depends on how slowly you turn the knob. You could also "cancel" it by not fully turning the knob through the bump.

### Breadboard
![](breadboard.jpg)

* The program has a counter (int) storing the number of steps turned.
* Two LEDs show the state of the two pins of the rotary encoder.
* Four LEDs show the lowest four bits of the counter value.

### Detecting rotation in code
Suppose that, when rotating clockwise, `pinA` changes before `pinB`. The pin readings will look like:
```
         begin turning knob, has some initial resistance
        |
        |  the disk stops touching the pad connected to pinA
        | |
        | |  the knob is "pulled" to the next orientation
        | | |
        | | |  the disk stops touching a pad connected to pinB
        | | | |   
        v v v v
pinA  0   1   1 1 1 1 0 0 0 0
pinB  0   0   1 1 1 1 1 0 0 0
                     ^ ^
                     | |
                     |  knob is pulled to next slot
                     |
                      begin turning knob
```

So if it's turning clockwise, the pin states are: `AA` -> `BA` -> `BB` -> `AB` -> `AA`.

Some people concatenate the previous and current states into a 4-bit integer, and use that to index into a lookup table (to avoid `if` statements).
* [How to use a quadrature encoder (pdf)](How%20to%20use%20a%20quadrature%20encoder.pdf)
* https://makeatronics.blogspot.com/2013/02/efficiently-reading-quadrature-with.html

The knob resting positions force both pins to be the same state. We can't have `AB` or `BA` as "resting states", only `AA` or `BB`. (But in other rotary encoders, I assume the disk could rotate freely.) It's also mentioned that we could detect many types of edge events (for higher-speed applications):
```
AA -> AB
   -> BA
   
AB -> AA
   -> BB
   
BA -> BB
   -> AA

BB -> BA
   -> AB
```

### Debouncing
Using mechanical keyboard switches as an example (these are the ones I've used):
* Linear switches (MX red) can have "key chattering" (caused by debouncing problems?)
  * You can't feel where the actuation point is, so it's easy to slide between `ON` and `OFF`
* Heavy tactile switches (MX clear) usually don't have debouncing problems.
  * The actuation point is below the bump. Once your finger moves past the bump, it has enough momentum to move even further down and past the actuation point. So it's less likely to slide between `ON` and `OFF`.

For this rotary encoder:
  * When the knob is rotated past the bump and pulled into the next slot, the knob's rotation will oscillate a little bit before settling down (like an underdamped system), or that's how I imagine it. This is where it quickly moves between `ON/OFF`.
  * When the knob begins turning away from a resting position, it's just your fingers gripping the knob and there's nothing that causes oscillations. On the other hand, when knob when is pulled into the next slot, your fingers' grip is not enough to dampen the motion.
  
There are eight edge events we can detect. Suppose we could sample infinitely fast (and detect all edge events)...
* increment counter: `00` -> `10` -> `11` -> `01` -> `00`
* decrement counter: `00` <- `10` <- `11` <- `01` <- `00`

With noise, it'd be a problem if interrupts were triggered on edge events because the motion didn't settle down yet. But we have __infinite speed__, so we wouldn't miss any events, right...?
* I think it's more robust if one pin remains constant at the rotation where the other pin could oscillate (depends on pad width).


stuff
* poll the pin states?
* pin change interrupt?

* check first edge? (i.e. departing from a resting state `AA` -> `AB`)
* check second edge? (i.e. arriving at a resting state `AB` -> `AA`) (oscillations!!)
