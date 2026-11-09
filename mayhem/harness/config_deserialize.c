/* GimpConfig text deserializer (GScanner based) over libgimpconfig's GimpColorConfig. */
#include "harness/common.h"
#include "libgimpconfig/gimpconfig.h"

int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
  mayhem_init ();
  return 0;
}

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GimpColorConfig *config = g_object_new (GIMP_TYPE_COLOR_CONFIG, NULL);
  GError          *error  = NULL;
  char            *text   = mayhem_cstr (data, size);
  gboolean         ok;

  ok = gimp_config_deserialize_string (GIMP_CONFIG (config), text, size, NULL, &error);
  if (ok)
    g_free (gimp_config_serialize_to_string (GIMP_CONFIG (config), NULL));

  g_clear_error (&error);
  g_free (text);
  g_object_unref (config);
  return ok ? 0 : -1;
}
