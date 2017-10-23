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

See ``renderfall -h`` for options:

    $ renderfall -h
    Usage: renderfall [OPTIONS] <in>
    Render a waterfall spectrum from raw IQ samples.

    Options:
      -n, --fftsize <fftsize>	FFT size (power of 2)
      -f, --format  <format>	Input format: uint8, int16, float64, etc.
      -w, --window  <window>	Windowing function: hann, gaussian, square, blackmanharris, hamming, kaiser, parzen
      -o, --outfile <outfile>	Output file path (defaults to <infile>.png)
      -s, --offset  <offset>	Start at specified byte offset
      -l, --overlap <overlap>	Overlap N samples per frame (defaults to 0)
      -c, --clip <clip>	Read only the first N samples from the file
      -v, --verbose 		Print verbose debugging output

Here's an example using a ``.cf32`` file of complex 32-bit floats:

    $ renderfall -f float32 -n 2048 -w hann data.cf32

Currently, only raw sequential samples are supported. To use a .wav file, you
can strip the header, and use the ``int16`` input format.

### Gallery

FM Band, from 87.9MHz to 107.9MHz, with a Hann window

![](/gallery/fm-band-hann.png "FM Band")


A MURS Radio

![](/gallery/murs-radio.png "MURS")

Stay tuned, more coming soon...
