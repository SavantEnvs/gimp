/* PowerVR (.pvr) texture loader: the plug-in's own load_image() running over the in-process libgimp mock. */
#include <stdio.h>
#include "harness/common.h"
#include "mock/mock.h"

#define main pvr_plugin_main
#include "plug-ins/common/file-pvr.c"
#undef main

int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
  mayhem_init ();
  return 0;
}

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GError    *error = NULL;
  GFile     *file;
  GimpImage *image;
  FILE      *fp;
  uint8_t   *copy;

  if (size < 16)
    return -1;

  copy = g_memdup2 (data, size);
  fp   = fmemopen (copy, size, "rb");
  file = g_file_new_for_path ("fuzz.pvr");

  mock_begin ();
  image = load_image (file, NULL, GIMP_RUN_NONINTERACTIVE, fp, &error);
  mock_end ();

  fclose (fp);
  g_free (copy);
  g_object_unref (file);
  g_clear_error (&error);
  return image ? 0 : -1;
}
