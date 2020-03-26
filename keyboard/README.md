There are many blog posts on how to make a keyboard. I think I get it...
* http://blog.komar.be/how-to-make-a-keyboard-the-matrix/
* https://www.baldengineer.com/arduino-keyboard-matrix-tutorial.html
* Some like to re-explain electrical theory. Some try to avoid discussing any theory. It's annoying...

### Keyboard matrix
Keyboard keys are wired in a matrix because fewer pins are needed to interface to a matrix. It looks very similar to methods used in LED matrices.

Here's a re-explanation (with shitty schematics) for my sake. (The blog posts go over this with better pictures.)

![](2x2_no_diodes.png)

#### Time-division multiplexing
Take turns turning on the columns. Suppose only `Col1` is `HIGH`. Then if a switch in `Col1` is pressed, the corresponding row will go `HIGH`. Otherwise, the row will be drained to `LOW` by the pulldown resistor.

#### Ghosting
If a key is not pressed, it's possible to press other keys so that the row reads `HIGH`:

![](ghosting.png)

The solution is to use diodes.
