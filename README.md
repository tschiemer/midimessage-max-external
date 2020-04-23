# midimessage-max-external

https://github.com/tschiemer/midimessage-max-external

MidiMessage external for Max/MSP offering conversion from a raw MIDI stream to a human readable, command-like format. Such as:

```
note on 1 2 3
cc 1 2 3
nrpn 1 512 4325
sysex rt 1 mcc stop
```
Also see example patcher in folder `examples`.

## Objects

### midimessage.gen

Generates MIDI byte sequences from given command; for command format also see https://github.com/tschiemer/midimessage

*runningstatus [0,1]* turns on or off running status, ie messages are generating conforming to the MIDI running status feature.


### midimessage.parse

Attempts to parse incoming byte sequences (single or lists of integers) and output the corresponding command (as above).

*runningstatus [0,1]* turns on or off running status, ie messages conforming to the MIDI running status feature are accepted.

*outputdiscardedbytes [0,1]* turns on or off the output of discarded bytes (through the second output).

*nrpnfilter [0,1]* turns on or off the detection/filtering of NRPN type CC sequences. Please note: imcomplete CC sequences similiar to 99-98-6-38, 99-98-96, 99-98-97 might get swallowed silently without you noticing.


## Building

```shell
git clone --recursive https://github.com/tschiemer/midimessage-max-external
cd midimessage-max-external/
cmake .
make
```

The external object files are generated in the folder `externals`, ie the files `externals/midimessage.gen.mxo` and `externals/midimessage.parse.mxo`, which have to be included in any Max search paths.


## License: MIT
