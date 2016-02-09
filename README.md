## Renderfall

Inspired by the excellent [baudline](http://www.baudline.com/) and
[gr-fosphor](https://sdr.osmocom.org/trac/wiki/fosphor), ``renderfall`` seeks
to render beautiful and informative spectrographs of audio, RF samples, and any
other data you care to throw at it.

The goal is to be able to render poster-scale visualizations of large (hundreds
of GB) input samples, so it places a large emphasis on performance, and avoids
loading the entire input stream into memory.

Features:

- Basically None

Known Bugs:

- So Many

### Installation

    $ mkdir build
    $ cmake ..
    $ make
    $ make install

### Basic Usage

Currently, only raw sequential samples are supported. To use a .wav file, you
can strip the header, and use the ``int16`` input format.

    $ renderfall data.samples

For more options, see:

    $ renderfall -h

### Gallery

FM Band, from 87.9MHz to 107.9MHz, with a Hann window

![](/gallery/fm-band-hann.png "FM Band")


A MURS Radio

![](/gallery/murs-radio.png "MURS")

Stay tuned, more coming soon...
