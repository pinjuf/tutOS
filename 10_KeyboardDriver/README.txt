Chapter 10 - Keyboard driver

Everything has IO. Life, the universe, and
your PC too. And sometimes, they just don't "O"
42, they also need some "I". For this, we will
use the PS/2 interface. First, we set up the 8042
PS/2 controller, and then, we can just use keyboard
interrupts! Later on, we should be able to easily add
mouse support!

FAQ:
    1) Why do you use the older Scan Code Set 1?
        Altough SCS1 is older, translation makes sure it's
        available even when the keyboard uses SCS2. Data
        mangling should not be a problem, as all configuration
        happens before this could become relevant.
    2) Why is there a kbd_bitmap?
        Although most programs will only be interested in
        the last key that was pressed's ASCII value, some
        might want to know which key is currently being pressed
        down. And keeping track of both normal and extended/special
        keycodes only costs us 64 bytes!
