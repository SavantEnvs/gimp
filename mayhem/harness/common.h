#ifndef MAYHEM_COMMON_H
#define MAYHEM_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

/* One-time GEGL/babl setup shared by every harness. GEGL_SWAP=RAM keeps
 * GEGL from creating swap files, so each input stays independent. */
static inline void
mayhem_init (void)
{
  g_setenv ("GEGL_SWAP", "RAM", TRUE);
  gegl_init (NULL, NULL);
}

/* NUL-terminated heap copy of the input. */
static inline char *
mayhem_cstr (const uint8_t *data, size_t size)
{
  char *s = g_malloc (size + 1);

  if (size)
    memcpy (s, data, size);
  s[size] = '\0';
  return s;
}

#endif
