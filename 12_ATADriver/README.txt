Chapter 12 - ATA Driver

An ATA PIO driver allows your OS to read from Hard Drives. 
Altough PIO mode can be a bit slow, it is pretty easy to
implement, and can make adding HD support a breeze.

FAQ:
    1) Why do you reset the drives during the ATA initialization?
        Because I can.
    2) Why is there a function for reading/writing from/to non-sector aligned addresses?
        Altough most filesystems are always aligned, it's good to have.
