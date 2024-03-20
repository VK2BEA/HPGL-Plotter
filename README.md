GPIB / HPGL Plotter Emulator
============================

Michael G. Katzmann (NV3Z/VK2BEA/G4NYV)
------------------------------------------------------------------

This is a program to emulate an HPGL plotter for older HP Spectrum and Network Analyzers:  

Once captured, the plot may be printed or saved as PDF, SVG or PNG files.

![image](https://github.com/VK2BEA/HPGL-Plotter/assets/3782222/69d8b02c-f6bd-464a-8fb1-5b8f8d919c2e)

To install on the Linux Fedora distribution:
-------------------------------------------
        $ sudo dnf -y copr enable vk2bea/GPIB 
        $ sudo dnf -y copr enable vk2bea/HPGLplotter 
        $ sudo dnf -y install HPGLplotter linux-gpib-firmware 

To build & install using Linux autotools, install the following required packages & tools:
----------------------------------------------------------------------
* `automake`, `autoconf` and `libtool`  
* To build on Raspberri Pi / Debian: 	`libglib2.0-dev libgtk-4-dev`
* To run on Raspberry Pi / Debian :	`libglib-2, libgtk-4, libgpib, fonts-noto-color-emoji`

To compile and to run this program the [Linux GPIB](https://linux-gpib.sourceforge.io/) driver must be installed. RPMs are available from the [Copr repositories](https://copr.fedorainfracloud.org/coprs/vk2bea/GPIB/) for Fedora Linux.

The National Instruments GPIB driver *may* also be used, but this has not been tested. The Linux GPIB API is compatable with the NI library.... quote: *"The API of the C library is intended to be compatible with National Instrument's GPIB library."*

Once the prerequisites (as listed above) are installed, install the 'HPGL Plotter' with these commands:

        $ ./autogen.sh
        $ cd build/
        $ ../configure
        $ make all
        $ sudo make install
To run:
        
        $ /usr/local/bin/HPGLplotter

To uninstall:
        
        $ sudo make uninstall

Troubleshooting:
----------------------------------------------------------------------
If problems are encountered, first confirm that correct GPIB communication is occuring. 

**Note** that the GPIB interface on the Linux computer must be able to act as a simple listener / talker. Some devices (notably the **Agilent 82357A/B**) can only act as system controllers and will not work with this application.

The program has been tested with the HP8753C Network Analyzer and the HP8595E Spectrum Analyzer. <em>(If you use it with other instruments, please report your experience)</em>

Use the `ibtest` and `ibterm` tools distributed with the `linux-gpib` distribution.

The HPGL Plotter logs some information to the journal, the verbosity of which can be set with the `--debug` command line switch.

To enable debugging output to the journal, start the program with the `--debug 7` switch, <em>(Debug levels 0-7)</em>.

If started without the switch, the default logging verbosity is 3.

To view the output (in follow mode) use:

        journalctl -t HPGLplotter -f
