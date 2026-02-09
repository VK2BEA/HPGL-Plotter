GPIB / HPGL Plotter Emulator
============================

Michael G. Katzmann (NV3Z/VK2BEA/G4NYV)
------------------------------------------------------------------

This is a program to emulate an HPGL plotter for older HP Spectrum and Network Analyzers:  

Once captured, the plot may be printed or saved as PDF, SVG or PNG files.

![image](https://github.com/user-attachments/assets/7593348b-f31a-4dce-b2b6-bbe72805f48e)

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

Command Line Options (all are optional):
----------------------------------------------------------------------
```
  -h,       --help                        Show help options
  -b [0-7], --debug                       Print diagnostic messages in journal (0-7)
  -s,       --stderrLogging               Send log data to the default device (usually stdout/stderr) rather than the journal
  -n,       --GPIBnoSystemController      Do not enable GPIB interface as a system controller
  -N,       --GPIBuseSystemController     Enable GPIB interface as a system controller when needed
  -l [0,1], --GPIBinitialListener         Force GPIB interface as a listener ('1', 'true' or no argument) or not ('0' or 'false')
  -d,       --GPIBdeviceID                GPIB device ID for HPGL plotter
  -c,       --GPIBcontrollerIndex         GPIB controller board index
  -C,       --GPIBcontrollerName          GPIB controller name (in /etc/gpib.conf)
  -e,       --EOIonLF                     End GPIB read on LF character
  -o,       --offline                     Do not open the GPIB controller on start up
```

Troubleshooting:
----------------------------------------------------------------------
If problems are encountered, first confirm that correct GPIB communication is occuring. 

Use the `ibtest` and `ibterm` tools distributed with the `linux-gpib` distribution.

**Note** that the GPIB interface on the Linux computer must be able to act as a simple listener / talker. Some devices (notably the **Agilent 82357A/B**) can only act as system controllers and will not work with this application. (the National Instruments GPIB-USB-HS does work as it does not have this restriction)

The program requires the `gtk4` library, on the Linux system, to be version 4.10 or later. The build system will automatically check for this requirement during configuration. To manually check your GTK4 version, use: 
`$ pkg-config --modversion gtk4`

On a real plotter the user would change the paper for each new plot. The `Auto Clear` function of the **HPGLplotter** anticipates a new plot by the lack of HPGL data for a period of time. On receipt of an HPGL block following the defined period of inactivity, the display is cleared. 

The duration of non-activity that indicates the end of plot is adjustable as a setting. If you notice that the instrument takes a long time to complete a plot or the plot is incomplete (or even blank), try unsetting `Auto Clear`. If the plot is then shown correctly you will need to increase the `end of plot period` setting (if you wish to use `Auto Clear`).

The program is known to work with:
- HP8753C Network Analyzer
- HP8713B Network Analyzer
- HP8595E Spectrum Analyzer
- HP8568B Spectrum Analyzer
- HP54100 oscilloscope
- HP 4145A Semiconductor Parameter Analyzer
- Rohde & Schwarz UPL Audio Analyzer

<em>(If you use it with other instruments, please report your experience)</em>

The HPGL Plotter logs some information to the journal, the verbosity of which can be set with the `--debug` command line switch.

To enable debugging output to the journal, start the program with the `--debug 7` switch, <em>(Debug levels 0-7)</em>.

If started without the switch, the default logging verbosity is 3.

To view the output (in follow mode) use:

        journalctl -t HPGLplotter -f
