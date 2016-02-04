/*
 * Copyright © 2009 Sam Thursfield
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Sam Thursfield <ssssam@gmail.com>
 */

#include "utils.h"

void
g_warning_win32_error (DWORD        result_code,
                       const gchar *format,
                       ...)
{
  va_list va;
  gchar *message;
  gchar *win32_error;
  gchar *win32_message;

  g_return_if_fail (result_code != 0);

  va_start (va, format);
  message = g_strdup_vprintf (format, va);
  win32_error = g_win32_error_message (result_code);
  win32_message = g_strdup_printf ("%s: %s", message, win32_error);
  g_free (message);
  g_free (win32_error);

  g_warning ("%s", win32_message);

  g_free (win32_message);
}

#define g_assert_no_win32_error(_r, _m)  G_STMT_START \
  if ((_r) != ERROR_SUCCESS)                         \
    g_warning_win32_error ((_r), (_m));  G_STMT_END

gboolean
util_registry_open_path (const gchar *key_name,
                         HKEY        *hkey)
{
  gchar *path;
  gunichar2 *pathw;
  LONG result;
 
  path = g_build_path ("\\", "Software\\GSettings", key_name, NULL);
  pathw = g_utf8_to_utf16 (path, -1, NULL, NULL, NULL);

  result = RegOpenKeyExW (HKEY_CURRENT_USER, pathw, 0, KEY_ALL_ACCESS, hkey);
  g_free (pathw);

  if (result != ERROR_SUCCESS)
    g_warning_win32_error (result, "Error opening registry path %s", path);

  g_free (path);

  return (result == ERROR_SUCCESS);
}

void
util_main_iterate (void)
{
  gint i;

  /* We iterate the main loop for a while, even after we have received the
   * change notification, to catch if we are getting more than one when only
   * one thing has changed.
   */
  for (i = 0; i < 1000; i++)
    {
      g_main_context_iteration (NULL, FALSE);
      g_usleep (100);
    }
}
