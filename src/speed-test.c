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

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

/* g_warning including a windows error message. */
static void
g_warning_win32_error (DWORD result_code,
                       const gchar *format,
                      ...)
{
  va_list va;
  gint pos;
  gchar win32_message[1024];

  if (result_code == 0)
    result_code = GetLastError ();

  va_start (va, format);
  pos = g_vsnprintf (win32_message, 512, format, va);

  win32_message[pos++] = ':'; win32_message[pos++] = ' ';
  
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, result_code, 0, (LPTSTR)(win32_message+pos),
                1023 - pos, NULL);
  g_warning (win32_message);
};


static gboolean
reg_open_path (gchar *key_name, HKEY *hkey)
{
  gchar *path;
  LONG   result;
 
  path = g_build_path ("\\", "Software\\GSettings", key_name, NULL);

  result = RegOpenKeyExA (HKEY_CURRENT_USER, path, 0, KEY_ALL_ACCESS, hkey);
  if (result != ERROR_SUCCESS)
    g_warning_win32_error (result, "Error opening registry path %s", path);

  g_free (path);
  return (result == ERROR_SUCCESS);
}



static void
basic_test (gpointer *data)
{
  GTimer *timer = g_timer_new ();
  gdouble time;
  gint    i;
  gchar  *string;

  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *settings = g_settings_new ("org.gsettings.test.storage-test");

  g_timer_reset (timer);
  for (i=0; i<10000; i++)
    {
      char string[32];
      g_snprintf (string, 31, "testing %i", i);
      g_settings_set_string (settings, "string", string);
    }
  time = g_timer_elapsed (timer, NULL);
  fprintf (stderr, "Write distinct strings: %f\n", time);

  g_timer_reset (timer);
  for (i=0; i<10000; i++)
    g_settings_set_string (settings, "string", "Testing");
  time = g_timer_elapsed (timer, NULL);
  fprintf (stderr, "Write identical strings: %f\n", time);

  g_timer_reset (timer);
  for (i=0; i<10000; i++)
    string = g_settings_get_string (settings, "string");
  time = g_timer_elapsed (timer, NULL);
  fprintf (stderr, "Read string: %f\n", time);


  g_object_unref (settings);
  g_main_loop_unref (main_loop);
}

int
main (int argc, char **argv)
{
  gint result;
  HKEY hparent;

  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/gsettings/speed/Basic", NULL, basic_test);

  result = g_test_run();

  /* If all the tests pass, now we delete the evidence */
  if (reg_open_path (NULL, &hparent))
    {
      const char *subpath = "tests\\storage";
      LONG result = SHDeleteKey (hparent, subpath);
      if (result != ERROR_SUCCESS)
        {
          g_warning_win32_error (result, "Error deleting test keys - couldn't delete %s:", 
                                 subpath);
          return 1;
        }
    }
  RegCloseKey (hparent);

  return result;
};