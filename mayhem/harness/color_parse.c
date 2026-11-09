/* CSS / hex / named-colour parsers in libgimpcolor. First input byte picks the entry point. */
#include "harness/common.h"
#include "libgimpcolor/gimpcolor.h"

int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
  mayhem_init ();
  return 0;
}

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GeglColor *color;
  char      *text;
  int        sel;

  if (size < 2)
    return -1;

  sel  = data[0] % 5;
  text = mayhem_cstr (data + 1, size - 1);

  switch (sel)
    {
    case 0:  color = gimp_color_parse_css (text);  break;
    case 1:  color = gimp_color_parse_hex (text);  break;
    case 2:  color = gimp_color_parse_name (text); break;
    case 3:  color = gimp_color_parse_css_substring (text, (gint) (size - 1));  break;
    default: color = gimp_color_parse_hex_substring (text, (gint) (size - 1));  break;
    }

  g_free (text);
  if (! color)
    return -1;

  {
    gdouble rgba[4];

    gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), rgba);
  }
  g_object_unref (color);
  return 0;
}
