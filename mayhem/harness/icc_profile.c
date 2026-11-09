/* ICC profile loading and inspection through GimpColorProfile (libgimpcolor over lcms2). */
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
  GimpColorProfile *profile;
  GError           *error = NULL;
  gsize             len;

  if (size < 128)
    return -1;

  profile = gimp_color_profile_new_from_icc_profile (data, size, &error);
  if (! profile)
    {
      g_clear_error (&error);
      return -1;
    }

  gimp_color_profile_get_description (profile);
  gimp_color_profile_get_manufacturer (profile);
  gimp_color_profile_get_model (profile);
  gimp_color_profile_get_copyright (profile);
  gimp_color_profile_get_label (profile);
  gimp_color_profile_get_summary (profile);
  gimp_color_profile_get_icc_profile (profile, &len);

  gimp_color_profile_is_rgb (profile);
  gimp_color_profile_is_gray (profile);
  gimp_color_profile_is_linear (profile);
  gimp_color_profile_is_cmyk (profile);

  g_object_unref (profile);
  return 0;
}
