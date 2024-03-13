GPIB / HPGL Plotter Emulator
============================

Michael G. Katzmann (NV3Z/VK2BEA/G4NYV)
------------------------------------------------------------------

This is a program to emulate an HPGL plotter for older HP Spectrum and Network Analyzers:  

Once captured, the plot may be printed or saved as PDF, SVG or PNG files.

![HPGL plotter plot](https://github.com/VK2BEA/HPGL-Plotter/assets/3782222/93aae20e-9779-4722-8888-43a0eaf3304f)

To build & install using Linux autotools, install the following required packages & tools:
----------------------------------------------------------------------
* `automake`, `autoconf` and `libtool`  
* To build on Raspberri Pi / Debian: 	`libgs-dev libglib2.0-dev libgtk-4-dev 
* To run on Raspberry Pi / Debian :	`libglib-2, libgtk-4, libgpib, fonts-noto-color-emoji`

Install the GPIB driver: 
See the `GPIB-Linux.driver/installGPIBdriver.on.RPI` file for a script that may work for you to download and install the Linux GPIB driver, otherwise, visit https://linux-gpib.sourceforge.io/ for installation instructions.

The National Instruments GPIB driver *may* also be used, but this has not been tested. The Linux GPIB API is compatable with the NI library.... quote: *"The API of the C library is intended to be compatible with National Instrument's GPIB library."*

Once the prerequisites (as listed above) are installed, install the 'HPGL Plotter' with these commands:

        $ ./autogen.sh
        $ cd build/
        $ ../configure
        $ make all
        $ sudo make install
To run:
        
        $ /usr/local/bin/hp8753

To uninstall:
        
        $ sudo make uninstall

Troubleshooting:
----------------------------------------------------------------------
If problems are encountered, first confirm that correct GPIB communication is occuring. 

Use the `ibtest` and `ibterm` tools distributed with the `linux-gpib` distribution.

The HPGL Plotter logs some information to the journal, the verbosity of which can be set with the `--debug` command line switch.

To enable debugging output to the journal, start the program with the `--debug 7` switch, <em>(Debug levels 0-7)</em>.

If started without the switch, the default logging verbosity is 3.

To view the output (in follow mode) use:

        journalctl -t HPGLplotter -f
