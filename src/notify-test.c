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

#include "utils.h"

typedef struct {
  gboolean  change_flag;
  gchar *key;
} Change;

static void
single_change_handler (GSettings   *settings,
                       const gchar *key,
                       Change      *change)
{
  //printf ("Got change: %s\n", key); fflush (stdout);

  /* Make sure previous change was acknowledged and
   * we are not getting spurious duplicates*/
  g_return_if_fail (change->change_flag == FALSE);

  change->change_flag = TRUE;
  change->key = g_strdup (key);
}

static void
basic_test (gconstpointer test_data)
{
  Change change;
  GMainLoop *main_loop;
  GSettings *settings;
  gchar *string;
  gint int32, x, y, z;
  gint64 int64;

  main_loop = g_main_loop_new (NULL, FALSE);
  settings = g_settings_new ("org.gsettings.test.storage-test");

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Test simple notifications. */
  change.change_flag = FALSE;
  g_settings_set_string (settings, "string", "Notify me");
  util_main_iterate ();
  g_assert_cmpstr (change.key, ==, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Notify me");
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set_int (settings, "int32", 1691);
  util_main_iterate ();
  g_assert_cmpstr (change.key, ==, "int32");
  int32 = g_settings_get_int (settings, "int32");
  g_assert (int32 == 1691);
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set (settings, "qword", "x", (gint64)-778019);
  util_main_iterate ();
  g_assert_cmpstr (change.key, ==, "qword");
  g_settings_get (settings, "qword", "x", &int64);
  g_assert (int64 == -778019);
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set (settings, "box", "(iii)", -1, 99, 11111);
  util_main_iterate ();
  g_assert_cmpstr (change.key, ==, "box");
  g_settings_get (settings, "box", "(iii)", &x, &y, &z);
  g_assert (x == -1 && y == 99 && z == 11111);
  g_free (change.key);

  /* Test resettting */
  change.change_flag = FALSE;
  g_settings_reset (settings, "string");
  util_main_iterate ();
  g_assert_cmpstr (change.key, ==, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Hello world");
  g_free (change.key);

  g_object_unref (settings);
  g_main_loop_unref (main_loop);
}

static void
manual_test (gconstpointer test_data)
{
  HKEY hpath;
  LONG result;
  Change change;
  gchar *string;
  gdouble double_value;
  GMainLoop *main_loop;
  GSettings *settings, *s1, *s2;

  main_loop = g_main_loop_new (NULL, FALSE);
  settings = g_settings_new ("org.gsettings.test.storage-test");

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Delete a value */
  change.change_flag = FALSE;
  g_settings_set_string (settings, "string", "I'm getting deleted!");

  while (change.change_flag == FALSE)
    util_main_iterate ();

  g_assert_cmpstr (change.key, ==, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr(string, == , "I'm getting deleted!");
  g_free (string);
  g_free (change.key);

  /* We should receive at this point a changed signal if the key
   * is externally changed
   */
  change.change_flag = FALSE;
  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      result = RegDeleteValueW (hpath, L"string");
      g_assert_no_win32_error (result, "Error deleting value 'string'");
      
      RegCloseKey (hpath);
    }

  while (change.change_flag == FALSE)
    util_main_iterate();

  g_assert_cmpstr(change.key, == , "string");
  string = g_settings_get_string(settings, "string");
  g_assert_cmpstr(string, == , "Hello world");
  g_free(string);
  g_free(change.key);

  /* Add a value */
  change.change_flag = FALSE;
  g_settings_reset (settings, "double");
  util_main_iterate();

  change.change_flag = FALSE;
  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      result = RegSetValueExW (hpath, L"double", 0, REG_SZ, L"2.99e8", 7 * sizeof (gunichar2));
      g_assert_no_win32_error (result, "Error setting value 'double'");
      
      RegCloseKey (hpath);
    }

  while (change.change_flag == FALSE)
    util_main_iterate ();

  g_assert (change.change_flag == TRUE);
  g_assert_cmpstr (change.key, ==, "double");
  double_value = g_settings_get_double (settings, "double");
  g_assert_cmpfloat (double_value, ==, 299000000.0);
  g_free (change.key);

  /* the same but using the ansi version to modify the registry */
  change.change_flag = FALSE;
  g_settings_reset(settings, "double");
  util_main_iterate();

  change.change_flag = FALSE;
  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      result = RegSetValueExA (hpath, "double", 0, REG_SZ, "2.99e8", 7);
      g_assert_no_win32_error (result, "Error setting value 'double'");

      RegCloseKey (hpath);
    }

  while (change.change_flag == FALSE)
    util_main_iterate ();

  g_assert (change.change_flag == TRUE);
  g_assert_cmpstr (change.key, ==, "double");
  double_value = g_settings_get_double (settings, "double");
  g_assert_cmpfloat (double_value, ==, 299000000.0);
  g_free (change.key);

  s1 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/a/twisty/little/maze/of/pathnames/all/alike/");
  s2 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/a/twisty/maze/of/little/pathnames/all/alike/");

  /* Add some keys */
  change.change_flag = FALSE;
  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      HKEY hsubpath1, hsubpath2, hsubpath3;

      result = RegCreateKeyExW (hpath, L"a", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath1, NULL);
      g_assert_no_win32_error (result, "Error ceating a path");

      result = RegCreateKeyExW (hsubpath1, L"twisty", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath2, NULL);
      g_assert_no_win32_error (result, "Error creating a path");

      /* A key that's not in the schema */
      result = RegCreateKeyExW (hsubpath2, L"snake", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath3, NULL);
      g_assert_no_win32_error (result, "Error creating a path");

      RegCloseKey (hsubpath3);
      RegCloseKey (hsubpath2);
      RegCloseKey (hsubpath1);

      g_assert (change.change_flag == FALSE);

      /* Set a value */
      g_settings_set (s1, "marker", "ms", "lamp");

      /* Now delete the whole thing */
      SHDeleteKeyW (hpath, L"a");

      RegCloseKey (hpath);
    }
  util_main_iterate ();

  g_settings_get (s2, "marker", "ms", &string);
  g_assert (string == NULL);

  g_object_unref (settings);
  g_object_unref (s2);
  g_object_unref (s1);

  g_main_loop_unref (main_loop);
}

static void
breakage_test (gconstpointer test_data)
{
  HKEY hpath;
  LONG result;
  Change change;
  gint i;
  GMainLoop *main_loop;
  GSettings *settings;

  main_loop = g_main_loop_new (NULL, FALSE);
  settings = g_settings_new ("org.gsettings.test.storage-test");

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Add a value not in the schema
   * (I think the registry backend does actually fire a changed notification
   * here and it's GSettings that ignores it, but that's fine, it shouldn't
   * happen really) */
  change.change_flag = FALSE;
  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      result = RegSetValueExW (hpath, L"intruder", 0, REG_SZ, L"oh no", 6 * sizeof (gunichar2));
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error setting value 'intruder'");
      
      RegCloseKey (hpath);
    }

  for (i = 0; i < 100; i++)
    util_main_iterate ();

  g_assert (change.change_flag == FALSE);

  g_object_unref (settings);

  g_main_loop_unref (main_loop);
}

static void
nesting_test (gconstpointer test_data)
{
  HKEY hpath;
  LONG result;
  Change change;
  gchar *string;
  GMainLoop *main_loop;
  GSettings *s1, *s2, *s3;

  main_loop = g_main_loop_new (NULL, FALSE);
  
  s1 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/");
  s2 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/nested/");
  s3 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/nested/even/further/");

  g_signal_connect (s1, "changed", G_CALLBACK (single_change_handler), &change);

  g_settings_set (s3, "marker", "ms", "bird");
 
  if (util_registry_open_path ("tests\\storage\\nested\\even\\further", &hpath))
    {
      result = RegSetValueExW (hpath, L"marker", 0, REG_SZ, L"\"tasty food\"", 12 * sizeof (gunichar2));
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error setting value 'intruder'");
      
      RegCloseKey (hpath);
    }

  g_usleep (10000);

  g_settings_get (s3, "marker", "ms", &string);
  g_assert_cmpstr (string, ==, "tasty food");
  g_free (string);

  g_object_unref (s3);
  g_object_unref (s2);
  g_object_unref (s1);

  g_main_loop_unref (main_loop);
}

static void
stress_test (gconstpointer test_data)
{
  GMainLoop *main_loop;
  GSettings *settings[100], *s0;
  gint i, j;

  main_loop = g_main_loop_new (NULL, FALSE);

  /* 63 is the maximum but this shouldn't fail, because the backend shouldn't
   * watch the same prefix twice */
  for (i = 0; i < 100; i++)
    settings[i] = g_settings_new ("org.gsettings.test.storage-test");

  for (i = 0; i < 1000; i++)
    {
      j = g_test_rand_int_range (0, 62);
      g_settings_set_int (settings[j], "a-5", i);
      j = g_test_rand_int_range (0, 62);
      g_assert_cmpint (g_settings_get_int (settings[j], "a-5"), ==, i);
    }

  for (i = 0; i < 100; i++)
    g_object_unref (settings[i]);

  /* Now watch 63 different paths. */

  s0 = g_settings_new ("org.gsettings.test.storage-test");

  for (i = 0; i < 62; i++)
    {
      char buffer[256];
      g_snprintf (buffer, 255, "/tests/storage/prefix%i/", i);
      settings[i] = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                              buffer);
    }

  for (i = 0; i < 1000; i++)
    {
      const gchar *string,
                  *nonsense[10] = {
        "Leeds", "London", "Manchester", "Bristol", "Birmingham",
        "Shrewsbury", "Swansea", "Harrogate", "Llanyfyllin", "Perth"
      };

      j = g_test_rand_int_range (0, 10);
      g_settings_set (settings[j], "marker", "ms", nonsense[i % 10]);
      g_settings_get (settings[j], "marker", "ms", &string);
      g_assert_cmpstr (string, ==, nonsense[i % 10]);
    }

  g_object_unref (s0);
  for (i = 0; i < 62; i++)
    g_object_unref (settings[i]);
  
  g_main_loop_unref (main_loop);
}

static void
delete_old_keys (void)
{
  HKEY hparent;

  /* If all the tests pass, now we delete the evidence */
  if (util_registry_open_path (NULL, &hparent))
    {
      SHDeleteKeyW(hparent, L"tests\\storage");
      RegCloseKey (hparent);
    }
}

int
main (int argc, char **argv)
{
  gint result;

  g_test_init (&argc, &argv, NULL);

  delete_old_keys ();

  g_test_add_data_func ("/gsettings/notify/Basic", NULL, basic_test);
  g_test_add_data_func ("/gsettings/notify/Manual", NULL, manual_test);
  g_test_add_data_func ("/gsettings/notify/Breakage", NULL, breakage_test);
  g_test_add_data_func ("/gsettings/notify/Nesting", NULL, nesting_test);
  g_test_add_data_func ("/gsettings/notify/Stress", NULL, stress_test);

  result = g_test_run ();

  delete_old_keys ();

  return result;
}
