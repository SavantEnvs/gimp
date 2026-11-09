/* X Window Dump (.xwd) loader: the plug-in's own load_image() running over the in-process libgimp mock. */
#include <stdio.h>
#include <unistd.h>
#include "harness/common.h"
#include "mock/mock.h"

#define main xwd_plugin_main
#include "plug-ins/common/file-xwd.c"
#undef main

int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
  mayhem_init ();
  return 0;
}

/* load_image() opens its input by path (g_fopen), so each input goes through a scratch file. */
int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  GError      *error = NULL;
  GFile       *file;
  GimpImage   *image;
  const gchar *tmpdir = g_getenv ("TMPDIR");
  gchar       *path;
  int          fd;

  path = g_build_filename (tmpdir && *tmpdir ? tmpdir : "/tmp", "mayhem-xwd-XXXXXX", NULL);
  fd   = mkstemp (path);
  if (fd < 0)
    {
      g_free (path);
      return -1;
    }
  if (size && write (fd, data, size) != (ssize_t) size)
    {
      close (fd);
      unlink (path);
      g_free (path);
      return -1;
    }
  close (fd);

  file = g_file_new_for_path (path);

  mock_begin ();
  image = load_image (file, &error);
  mock_end ();

  unlink (path);
  g_free (path);
  g_object_unref (file);
  g_clear_error (&error);
  return image ? 0 : -1;
}
