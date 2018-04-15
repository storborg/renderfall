import sys
import argparse
import json
import os.path
import subprocess


def get_sigmf_paths(path):
    assert path.endswith('.sigmf-meta') or path.endswith('.sigmf-data')
    base = path.rsplit('-', 1)[0]
    return (base + '-meta'), (base + '-data')


class SigMFFormat(object):
    def __init__(self, s):
        self.complex = s[0] == 'c'
        base, self.endianness = s[1:].split('_')
        self.basetype = base[0]
        self.precision = int(base[1:])

    @property
    def size(self):
        if self.complex:
            return 2 * (self.precision / 8)
        else:
            return self.precision / 8

    @property
    def renderfall_format(self):
        assert self.complex, \
            "Only complex supported for now, sorry"
        assert self.endianness == 'le', \
            "Only little-endian supported for now, sorry"
        name = {
            'u': 'uint',
            'i': 'int',
            'f': 'float',
        }[self.basetype]
        return "%s%d" % (name, self.precision)


svg_preamble = '<?xml version="1.0" encoding="utf-8" standalone="no"?>'
svg_header = '''<svg width="{width}" height="{height}"
    viewBox="0 0 {width} {height}"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink">'''


class TagGenerator(object):
    def __getattr__(self, name):
        def tag(*children, **attrs):
            inner = ''.join(children)
            if attrs:
                attrs = ' '.join(['%s="%s"' % (key.replace('_', '-'), val)
                                  for key, val in attrs.items()])
                return '<%s %s>%s</%s>' % (name, attrs, inner, name)
            else:
                return '<%s>%s</%s>' % (name, inner, name)
        return tag


SVG = TagGenerator()


def write_sigmf_svg_wrapper(meta, verbose, samp_count, fftsize, overlap,
                            out_svg_path, out_png_path):
    # Compute size.
    width = fftsize
    height = samp_count / (fftsize - overlap)

    # XXX assumes that center frequency doesn't change, for now
    center_freq = float(meta['captures'][0]['core:frequency'])
    samp_rate = float(meta['global']['core:sample_rate'])

    def index_to_y(index):
        return (float(index) / float(samp_count)) * height

    def freq_to_x(freq):
        start_freq = center_freq - (samp_rate / 2.0)
        norm_x = (freq - start_freq) / samp_rate
        return norm_x * float(width)

    doc = []
    doc.append(svg_header.format(width=width, height=height))

    # Include the actual spectrogram first so that it's at the bottom of the z
    # stack
    doc.append(SVG.image(height=height,
                         width=width,
                         style="opacity:0.7",
                         **{'xlink:href': os.path.basename(out_png_path)}))

    # Draw a solid line at the beginning of each capture segment except the
    # first.
    for segment in meta['captures'][1:]:
        # figure out y coord from sample_start
        index_start = segment['core:sample_start']
        if verbose:
            print("Drawing capture segment boundary at index %d" %
                  index_start)
        if index_start <= samp_count:
            y = index_to_y(index_start)
            doc.append(SVG.line(
                x1=0, x2=width,
                y1=y, y2=y,
                style='stroke:#0000c6;stroke-width:3.0'))
        else:
            if verbose:
                print("Out of bounds, skipping.")

    # Draw a box for each annotation.
    for annotation in meta.get('annotations', []):
        # figure out which frame start and end are in to get y coords
        index_start = annotation['core:sample_start']
        if verbose:
            print("Drawing annotation box at index %d" % index_start)
        if index_start <= samp_count:
            y = index_to_y(index_start)
            h = index_to_y(annotation['core:sample_count'])

            # map freq lower and upper edge to get x coords
            if 'core:freq_lower_edge' in annotation:
                x = freq_to_x(annotation['core:freq_lower_edge'])
            else:
                x = 0
            if 'core:freq_upper_edge' in annotation:
                x_end = freq_to_x(annotation['core:freq_upper_edge'])
            else:
                x_end = width
            assert x_end - x, "Can't have freq lower edge above upper edge"
            w = x_end - x
            doc.append(SVG.rect(
                x=x, y=y, width=w, height=h,
                style='stroke:#bd0000; fill:#bd0000; fill-opacity:0.4'))
        else:
            if verbose:
                print("Out of bounds, skipping.")

    doc.append('</svg>\n')

    with open(out_svg_path, 'w') as f:
        f.write('\n'.join(doc))


def main(argv=sys.argv):
    p = argparse.ArgumentParser(
        description='Render a SigMF-annotated spectrogram.')

    p.add_argument('-n', '--fftsize', default=2048, type=int,
                   help='FFT size')
    p.add_argument('-w', '--window', default='blackman',
                   help='Windowing function: hann, gaussian, square, '
                   'blackman, blackmanharris, hamming, kaiser, parzen')
    p.add_argument('-o', '--outfile',
                   help='Output file path (defaults to <infile>.svg)')
    p.add_argument('-l', '--overlap', default=0, type=int,
                   help='Overlap N samples per frame')
    p.add_argument('-c', '--clip', type=int,
                   help='Render only the first N samples from the dataset')
    p.add_argument('-v', '--verbose', action='store_true',
                   help="Print verbose debugging output")
    p.add_argument('infile', help='SigMF file to render')

    opts = p.parse_args(argv[1:])

    # Prepare filenames for output renderings.
    in_base_path = opts.infile.rsplit('.', 1)[0]
    out_svg_path = opts.outfile or (in_base_path + '.svg')
    out_base_path = out_svg_path.rsplit('.', 1)[0]
    out_png_path = out_base_path + '.png'

    # Note: we don't currently handle archives.
    meta_path, data_path = get_sigmf_paths(opts.infile)
    meta = json.load(open(meta_path, 'r'))

    # Get sample format.
    byte_count = os.path.getsize(data_path)
    format = SigMFFormat(meta['global']['core:datatype'])
    samp_count = byte_count / format.size
    if opts.clip:
        samp_count = min(samp_count, opts.clip)

    # Generate an SVG with annotations.
    if opts.verbose:
        print("Writing SVG wrapper to %s..." % out_svg_path)
    write_sigmf_svg_wrapper(meta, opts.verbose, samp_count, opts.fftsize,
                            opts.overlap, out_svg_path, out_png_path)

    # Get file offset to pass to renderfall.
    # XXX

    # Call renderfall.
    cmd = [
        'renderfall',
        '-n', str(opts.fftsize),
        '-f', format.renderfall_format,
        '-w', opts.window,
        '-o', out_png_path,
        '-s', '0',  # XXX
        '-l', str(opts.overlap or 0),
        '-c', str(opts.clip or 0),
    ]
    if opts.verbose:
        cmd.append('-v')
    cmd.append(data_path)
    if opts.verbose:
        print("Calling renderfall with: %r" % cmd)
    subprocess.check_call(cmd)


if __name__ == '__main__':
    main()
