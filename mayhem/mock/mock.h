#ifndef MAYHEM_MOCK_H
#define MAYHEM_MOCK_H

/* Stand-in for the slice of libgimp's wire-protocol calls that the plug-in file loaders make while
 * decoding. Real libgimp would forward each of them to the GIMP core over a pipe; these keep the image and
 * layers in process (GEGL buffers included) so the loader's own parsing code runs unmodified. */

void mock_begin (void);
void mock_end   (void);

#endif
