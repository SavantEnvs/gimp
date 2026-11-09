/* GimpMetadata XML deserializer (GMarkup based) in libgimpbase. */
#include "harness/common.h"
#include "libgimpbase/gimpbase.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GimpMetadata *metadata;
  char         *xml;

  if (size < 4)
    return -1;

  xml      = mayhem_cstr (data, size);
  metadata = gimp_metadata_deserialize (xml);
  g_free (xml);

  if (! metadata)
    return -1;

  g_object_unref (metadata);
  return 0;
}
