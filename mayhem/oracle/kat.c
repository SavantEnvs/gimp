/* Known-answer probe for the code the harnesses fuzz. Each mode prints a deterministic summary of what
 * the real parser produced for a fixed input; mayhem/test.sh compares it to the expected text. */
#include <stdio.h>
#include <string.h>
#include "harness/common.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include <gexiv2/gexiv2.h>
#include "plug-ins/file-sgi/sgi-lib.c"

static int
kat_sgi (const char *path)
{
  sgi_t          *s = sgiOpen (path, SGI_READ, 0, 0, 0, 0, 0);
  unsigned short *row;
  int             x, y, z;

  if (! s)
    return 1;
  printf ("sgi %dx%dx%d bpp=%d\n", s->xsize, s->ysize, s->zsize, s->bpp);
  row = g_new0 (unsigned short, s->xsize);
  for (z = 0; z < s->zsize; z++)
    for (y = 0; y < s->ysize; y++)
      {
        if (sgiGetRow (s, row, y, z) < 0)
          return 1;
        printf ("z%d y%d:", z, y);
        for (x = 0; x < s->xsize; x++)
          printf (" %u", row[x]);
        printf ("\n");
      }
  sgiClose (s);
  return 0;
}

static int
kat_color (const char *css)
{
  GeglColor *c = gimp_color_parse_css (css);
  guint8     px[4];

  if (! c)
    return 1;
  gegl_color_get_pixel (c, babl_format ("R'G'B'A u8"), px);
  printf ("color %u %u %u %u\n", px[0], px[1], px[2], px[3]);
  g_object_unref (c);
  return 0;
}

static int
kat_config (const char *text)
{
  GimpColorConfig *cfg = g_object_new (GIMP_TYPE_COLOR_CONFIG, NULL);
  GError          *err = NULL;
  gint             mode;
  gboolean         hsv, rgb_u8;

  if (! gimp_config_deserialize_string (GIMP_CONFIG (cfg), text, -1, NULL, &err))
    return 1;
  g_object_get (cfg, "mode", &mode, "show-hsv", &hsv, "show-rgb-u8", &rgb_u8, NULL);
  printf ("config mode=%d show-hsv=%d show-rgb-u8=%d\n", mode, hsv, rgb_u8);
  g_object_unref (cfg);
  return 0;
}

static int
kat_icc (const char *path)
{
  gchar            *data;
  gsize             len;
  GimpColorProfile *p;
  GError           *err = NULL;

  if (! g_file_get_contents (path, &data, &len, &err))
    return 1;
  p = gimp_color_profile_new_from_icc_profile ((const guint8 *) data, len, &err);
  if (! p)
    return 1;
  printf ("icc rgb=%d gray=%d cmyk=%d linear=%d desc=%s\n",
          gimp_color_profile_is_rgb (p), gimp_color_profile_is_gray (p),
          gimp_color_profile_is_cmyk (p), gimp_color_profile_is_linear (p),
          gimp_color_profile_get_description (p));
  g_object_unref (p);
  return 0;
}

static int
kat_meta (const char *xml)
{
  GimpMetadata *m = gimp_metadata_deserialize (xml);
  gchar        *make, *model;

  if (! m)
    return 1;
  make  = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (m), "Exif.Image.Make", NULL);
  model = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (m), "Exif.Image.Model", NULL);
  printf ("meta make=%s model=%s\n", make ? make : "(null)", model ? model : "(null)");
  g_object_unref (m);
  return 0;
}

int
main (int argc, char **argv)
{
  if (argc != 3)
    return 2;
  mayhem_init ();
  if (! strcmp (argv[1], "sgi"))    return kat_sgi (argv[2]);
  if (! strcmp (argv[1], "color"))  return kat_color (argv[2]);
  if (! strcmp (argv[1], "config")) return kat_config (argv[2]);
  if (! strcmp (argv[1], "icc"))    return kat_icc (argv[2]);
  if (! strcmp (argv[1], "meta"))   return kat_meta (argv[2]);
  return 2;
}
