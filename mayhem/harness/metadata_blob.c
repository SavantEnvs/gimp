/* Raw Exif / IPTC / XMP blobs handed to GimpMetadata (libgimpbase over gexiv2/exiv2), then GIMP's own
 * accessors over whatever tags the blob produced. */
#include "harness/common.h"
#include "libgimpbase/gimpbase.h"

static const char *mime_types[] = { "image/jpeg", "image/png", "image/tiff", "image/webp" };

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GimpMetadata *metadata;
  GError       *error = NULL;
  gboolean      ok;

  if (size < 4)
    return -1;

  metadata = gimp_metadata_new ();

  switch (data[0] % 3)
    {
    case 0:  ok = gimp_metadata_set_from_exif (metadata, data + 1, (gint) size - 1, &error); break;
    case 1:  ok = gimp_metadata_set_from_iptc (metadata, data + 1, (gint) size - 1, &error); break;
    default: ok = gimp_metadata_set_from_xmp  (metadata, data + 1, (gint) size - 1, &error); break;
    }

  g_clear_error (&error);

  if (ok)
    {
      gdouble                 xres, yres;
      GimpUnit               *unit;
      GimpMetadataColorspace  cs;
      gchar                 **tags;
      gint                    i, j;

      gimp_metadata_get_resolution (metadata, &xres, &yres, &unit);
      cs = gimp_metadata_get_colorspace (metadata);
      gimp_metadata_set_colorspace (metadata, cs);
      gimp_metadata_set_pixel_size (metadata, 64, 48);
      gimp_metadata_set_bits_per_sample (metadata, 8);
      gimp_metadata_set_resolution (metadata, 72.0, 72.0, gimp_unit_inch ());

      for (i = 0; i < 3; i++)
        {
          tags = i == 0 ? gexiv2_metadata_get_exif_tags (GEXIV2_METADATA (metadata)) :
                 i == 1 ? gexiv2_metadata_get_xmp_tags  (GEXIV2_METADATA (metadata)) :
                          gexiv2_metadata_get_iptc_tags (GEXIV2_METADATA (metadata));
          for (j = 0; tags && tags[j]; j++)
            {
              gint k;

              for (k = 0; k < G_N_ELEMENTS (mime_types); k++)
                gimp_metadata_is_tag_supported (tags[j], mime_types[k]);
            }
          g_strfreev (tags);
        }
    }

  g_object_unref (metadata);
  return ok ? 0 : -1;
}
