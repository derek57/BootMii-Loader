# BootMii loader for NANDBoot

This repository contains the public release of the source code for
the BootMii for NANDBoot stub loader.

Included portions:

* ECC tool
* BIN2C tool
* Preloader
* Main BootMii loader
* Memory freeing potion
* MINI as kind of a slightly modified version

Not included:

* BootMii WAD as of NAND blocks 3 & 4
* NANDBoot binary with ECC

MINI itself is based of GitHub (commit fc1234b22df95948d463203823d9ccda9833065a).

Note that the code in this repository may differ from the source code used to
build the official version of MINI.

This code is released with no warranty, and has only been build tested.
If you release this to anyone but yourself without extensive testing, you are an
irresponsible person.

## Build instructions

You need armeb-eabi cross compilers. Same build setup as
[mini](https://github.com/fail0verflow/mini). See
[bootmii-utils](https://github.com/fail0verflow/bootmii-utils) for some outdated
toolchain build scripts. Type `make` to compile. Good luck.

Output is at `BootMii-Loader_blocks_1-4.flash`, which is thought to be flashed from NAND blocks 1 till 4.

## Seriously

Do NOT release this to users unless you've spent months testing
hardware variations.

## License

Unless otherwise noted in an individual file header, all source code in this
repository is released under the terms of the GNU General Public License,
version 2 or later. The full text of the license can be found in the COPYING
file.
