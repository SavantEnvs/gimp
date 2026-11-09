#include <string.h>
#include <gegl.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "mock/mock.h"

#define MOCK_MAX_SIZE GIMP_MAX_IMAGE_SIZE

static GPtrArray *objects = NULL;

static gpointer
track (gpointer obj)
{
  if (! objects)
    objects = g_ptr_array_new ();
  g_ptr_array_add (objects, obj);
  return obj;
}

void
mock_begin (void)
{
  mock_end ();
}

void
mock_end (void)
{
  guint i;

  if (! objects)
    return;
  for (i = objects->len; i > 0; i--)
    g_object_unref (g_ptr_array_index (objects, i - 1));
  g_ptr_array_set_size (objects, 0);
}

/* The babl format a real layer of this type and image precision uses. Loaders pass plain pixel
 * arrays to gegl_buffer_set() with a NULL format, so the sample type must match the real one. */
static const Babl *
format_for_layer (GimpImageType type, GimpPrecision precision)
{
  const gchar *comp, *model, *suffix;
  gboolean     alpha = FALSE;
  gchar       *name;
  const Babl  *format;

  switch (precision)
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:     comp = "u8";     break;
    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_NON_LINEAR:
    case GIMP_PRECISION_U16_PERCEPTUAL:    comp = "u16";    break;
    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_NON_LINEAR:
    case GIMP_PRECISION_U32_PERCEPTUAL:    comp = "u32";    break;
    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_NON_LINEAR:
    case GIMP_PRECISION_HALF_PERCEPTUAL:   comp = "half";   break;
    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_NON_LINEAR:
    case GIMP_PRECISION_DOUBLE_PERCEPTUAL: comp = "double"; break;
    default:                               comp = "float";  break;
    }

  switch (precision)
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_DOUBLE_LINEAR:     suffix = ""; break;
    default:                               suffix = "'"; break;
    }

  switch (type)
    {
    case GIMP_RGBA_IMAGE:     alpha = TRUE; /* fall through */
    case GIMP_RGB_IMAGE:      model = "RGB"; break;
    case GIMP_GRAYA_IMAGE:    alpha = TRUE; /* fall through */
    case GIMP_GRAY_IMAGE:     model = "Y";   break;
    case GIMP_INDEXEDA_IMAGE: alpha = TRUE; /* fall through */
    default:                  model = "Y";   comp = "u8"; suffix = "'"; break;
    }

  if (! strcmp (model, "RGB"))
    name = g_strdup_printf ("R%sG%sB%s%s %s", suffix, suffix, suffix, alpha ? "A" : "", comp);
  else
    name = g_strdup_printf ("Y%s%s %s", suffix, alpha ? "A" : "", comp);

  format = babl_format (name);
  g_free (name);
  return format;
}

GimpImage *
gimp_image_new (gint width, gint height, GimpImageBaseType type)
{
  GimpImage *image;

  if (width < 1 || height < 1 || width > MOCK_MAX_SIZE || height > MOCK_MAX_SIZE)
    return NULL;
  image = track (g_object_new (GIMP_TYPE_IMAGE, NULL));
  g_object_set_data (G_OBJECT (image), "mock-width",  GINT_TO_POINTER (width));
  g_object_set_data (G_OBJECT (image), "mock-height", GINT_TO_POINTER (height));
  g_object_set_data (G_OBJECT (image), "mock-type",   GINT_TO_POINTER (type));
  g_object_set_data (G_OBJECT (image), "mock-precision", GINT_TO_POINTER (GIMP_PRECISION_U8_NON_LINEAR));
  return image;
}

GimpImage *
gimp_image_new_with_precision (gint width, gint height, GimpImageBaseType type, GimpPrecision precision)
{
  GimpImage *image = gimp_image_new (width, height, type);

  if (image)
    g_object_set_data (G_OBJECT (image), "mock-precision", GINT_TO_POINTER (precision));
  return image;
}

gboolean
gimp_image_delete (GimpImage *image)
{
  return TRUE;
}

GimpLayerMode
gimp_image_get_default_new_layer_mode (GimpImage *image)
{
  return GIMP_LAYER_MODE_NORMAL;
}

gboolean
gimp_image_insert_layer (GimpImage *image, GimpLayer *layer, GimpLayer *parent, gint position)
{
  return TRUE;
}

GimpLayer *
gimp_layer_new (GimpImage *image, const gchar *name, gint width, gint height,
                GimpImageType type, gdouble opacity, GimpLayerMode mode)
{
  GimpLayer *layer;
  GeglBuffer *buffer;

  if (! G_IS_OBJECT (image))
    return NULL;
  if (width < 1 || height < 1 || width > MOCK_MAX_SIZE || height > MOCK_MAX_SIZE)
    return NULL;
  layer  = track (g_object_new (GIMP_TYPE_LAYER, NULL));
  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                            format_for_layer (type, GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image), "mock-precision"))));
  g_object_set_data_full (G_OBJECT (layer), "mock-buffer", buffer, g_object_unref);
  return layer;
}

GeglBuffer *
gimp_drawable_get_buffer (GimpDrawable *drawable)
{
  GeglBuffer *buffer;

  if (! G_IS_OBJECT (drawable))
    return NULL;
  buffer = g_object_get_data (G_OBJECT (drawable), "mock-buffer");

  return buffer ? g_object_ref (buffer) : NULL;
}

gboolean
gimp_progress_init (const gchar *message)
{
  return TRUE;
}

gboolean
gimp_progress_init_printf (const gchar *format, ...)
{
  return TRUE;
}

gboolean
gimp_progress_update (gdouble percentage)
{
  return TRUE;
}
