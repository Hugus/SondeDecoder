# SondeDecoder
This is a windows version of [https://github.com/rs1729/RS].
Currently, only M10 sonde is supported, I do not plan to implement all radio sondes.
It has been tested on windows 7 and windows 10.

## Installation
Release folder contains an executable that is a command line program.
You may need to install **Microsoft Visual C++ 2015 Redistributable**. It is available from Microsoft website.

## Use
### Setup
You have to stream FM demodulated audio into the program. You can either :
- use your microphone input : set it as default recording device in control panel
- pipe audio from one program (for instance SDR#) to SondeDecoder using a program such as **virtual cable**, virtual cable output should be set as default recording device

### Execution
Start the program by double clicking it. It first outputs the list of recording devices and then some information about the audio stream.

When a sonde packet is correctly decoded, lines starting with [OK] should appear in the terminal.

Lines starting with [NOT OK] are partially decoded packets, some of the data might be correct though.

### Audio samples
You can use an audio sample to test you setup. See Sample folder for more information.
