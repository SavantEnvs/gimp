/* SGI (.rgb/.bw/.sgi) reader: the plug-in's own sgi-lib.c, header + RLE tables + rows. */
#include <stdio.h>
#include "harness/common.h"
#include "plug-ins/file-sgi/sgi-lib.c"

#define MAX_ROWS 256

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  FILE           *fp;
  sgi_t          *sgip;
  unsigned short *row;
  guint32         sum = 0;
  int             y, z, rows = 0;
  uint8_t        *copy;

  if (size < 12)
    return -1;

  copy = g_memdup2 (data, size);
  fp   = fmemopen (copy, size, "rb");
  if (! fp)
    {
      g_free (copy);
      return -1;
    }

  sgip = sgiOpenFile (fp, SGI_READ, 0, 0, 0, 0, 0);
  if (! sgip)
    {
      fclose (fp);
      g_free (copy);
      return -1;
    }

  row = g_new0 (unsigned short, sgip->xsize ? sgip->xsize : 1);
  for (z = 0; z < sgip->zsize && rows < MAX_ROWS; z++)
    for (y = 0; y < sgip->ysize && rows < MAX_ROWS; y++, rows++)
      if (sgiGetRow (sgip, row, y, z) >= 0)
        if (sgip->xsize)
          sum += row[0] + row[sgip->xsize - 1];

  g_free (row);
  sgiClose (sgip);
  g_free (copy);
  return sum == 0xFFFFFFFFu ? 1 : 0;
}
