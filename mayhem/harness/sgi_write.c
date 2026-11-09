/* SGI exporter: the plug-in's own sgi-lib.c write path (sgiOpen SGI_WRITE, sgiPutRow, sgiClose),
 * driven the way file-sgi/sgi.c export_image() drives it for an image of fuzzer-chosen shape and pixels. */
#include <stdio.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include "harness/common.h"
#include "plug-ins/file-sgi/sgi-lib.c"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  /* data[0..3]: compression (none/rle/arle, the plug-in's three choices), channels 1-4 (the plug-in's
   * GRAY..RGBA), width and height 1-32; the rest is pixel data, 0 past its end. */
  int             comp, zsize, width, height, i, j, x, y;
  const uint8_t  *pix;
  size_t          npix, pos = 0;
  gushort       **rows;
  sgi_t          *sgip;
  gchar          *path;
  gint            fd;

  if (size < 4)
    return -1;

  comp   = data[0] % 3;
  zsize  = 1 + data[1] % 4;
  width  = 1 + data[2] % 32;
  height = 1 + data[3] % 32;
  pix    = data + 4;
  npix   = size - 4;

  path = g_build_filename (g_get_tmp_dir (), "sgi-write-XXXXXX", NULL);
  fd   = g_mkstemp (path);
  if (fd < 0)
    {
      g_free (path);
      return -1;
    }
  close (fd);

  sgip = sgiOpen (path, SGI_WRITE, comp, 1, width, height, zsize);
  if (! sgip)
    {
      g_unlink (path);
      g_free (path);
      return -1;
    }

  rows    = g_new (gushort *, sgip->zsize);
  rows[0] = g_new (gushort, ((gsize) sgip->xsize) * sgip->zsize);
  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  for (y = 0; y < height; y ++)
    {
      for (x = 0; x < width; x ++)
        for (j = 0; j < zsize; j ++, pos ++)
          rows[j][x] = pos < npix ? pix[pos] : 0;

      for (j = 0; j < zsize; j ++)
        sgiPutRow (sgip, rows[j], height - 1 - y, j);
    }

  sgiClose (sgip);

  g_free (rows[0]);
  g_free (rows);
  g_unlink (path);
  g_free (path);
  return 0;
}
