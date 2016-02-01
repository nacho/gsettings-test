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

#define g_assert_no_win32_error(_r, _m)  G_STMT_START \
  if ((_r) != ERROR_SUCCESS)                         \
    g_warning_win32_error ((_r), (_m));  G_STMT_END


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
main_iterate ()
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

typedef struct {
  gboolean  change_flag;
  gchar    *key;
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
basic_test (gconstpointer    test_data)
{
  Change change;

  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *settings = g_settings_new ("org.gsettings.test.storage-test");

  gchar *string;
  gint   int32, x, y, z;
  gint64 int64;

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Test simple notifications. */
  change.change_flag = FALSE;
  g_settings_set_string (settings, "string", "Notify me");
  main_iterate ();
  g_assert_cmpstr (change.key, ==, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Notify me");
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set_int (settings, "int32", 1691);
  main_iterate ();
  g_assert_cmpstr (change.key, ==, "int32");
  int32 = g_settings_get_int (settings, "int32");
  g_assert (int32 == 1691);
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set (settings, "qword", "x", (gint64)-778019);
  main_iterate ();
  g_assert_cmpstr (change.key, ==, "qword");
  g_settings_get (settings, "qword", "x", &int64);
  g_assert (int64 == -778019);
  g_free (change.key);

  change.change_flag = FALSE;
  g_settings_set (settings, "box", "(iii)", -1, 99, 11111);
  main_iterate ();
  g_assert_cmpstr (change.key, ==, "box");
  g_settings_get (settings, "box", "(iii)", &x, &y, &z);
  g_assert (x==-1 && y==99 && z==11111);
  g_free (change.key);

  /* Test resettting */
  change.change_flag = FALSE;
  g_settings_reset (settings, "string");
  main_iterate ();
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
  HKEY       hpath;
  LONG       result;
  Change     change;
  gchar     *string;
  gdouble    double_value;

  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *settings = g_settings_new ("org.gsettings.test.storage-test"),
            *s1, *s2;

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Delete a value */
  change.change_flag = FALSE;
  g_settings_set_string (settings, "string", "I'm getting deleted!");
  if (reg_open_path ("tests\\storage", &hpath))
    {
      result = RegDeleteValueA (hpath, "string");
      g_assert_no_win32_error (result, "Error deleting value 'string'");
    }
  RegCloseKey (hpath);
  while (change.change_flag == FALSE)
    main_iterate ();
  g_usleep (10000);
  g_assert_cmpstr (change.key, ==, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Hello world");
  g_free (string);
  g_free (change.key);

  /* Add a value */
  g_settings_reset (settings, "double");
  change.change_flag = FALSE;
  if (reg_open_path ("tests\\storage", &hpath))
    {
      result = RegSetValueExA (hpath, "double", 0, REG_SZ, "2.99e8", 7);
      g_assert_no_win32_error (result, "Error setting value 'double'");
    }
  RegCloseKey (hpath);
  main_iterate ();
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
  if (reg_open_path ("tests\\storage", &hpath))
    {
      HKEY hsubpath1, hsubpath2, hsubpath3;
      result = RegCreateKeyEx (hpath, L"a", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath1, NULL);
      g_assert_no_win32_error (result, "Error ceating a path");
      result = RegCreateKeyEx (hsubpath1, L"twisty", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath2, NULL);
      g_assert_no_win32_error (result, "Error creating a path");

      /* A key that's not in the schema */
      result = RegCreateKeyEx (hsubpath2, L"snake", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hsubpath3, NULL);
      g_assert_no_win32_error (result, "Error creating a path");

      RegCloseKey (hsubpath3);
      RegCloseKey (hsubpath2);
      RegCloseKey (hsubpath1);

      g_assert (change.change_flag == FALSE);

      /* Set a value */
      g_settings_set (s1, "marker", "ms", "lamp");

      /* Now delete the whole thing */
      SHDeleteKey (hpath, L"a");

      RegCloseKey (hpath);
    }
  main_iterate ();

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
  HKEY       hpath;
  LONG       result;
  Change     change;

  gint       i;

  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *settings = g_settings_new ("org.gsettings.test.storage-test");

  g_signal_connect (settings, "changed", G_CALLBACK (single_change_handler), &change);

  /* Add a value not in the schema
   * (I think the registry backend does actually fire a changed notification
   * here and it's GSettings that ignores it, but that's fine, it shouldn't
   * happen really) */
  change.change_flag = FALSE;
  if (reg_open_path ("tests\\storage", &hpath))
    {
      result = RegSetValueExA (hpath, "intruder", 0, REG_SZ, "oh no", 6);
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error setting value 'intruder'");
    }
  RegCloseKey (hpath);
  for (i=0; i<100; i++)
    main_iterate ();
  g_assert (change.change_flag == FALSE);

  g_object_unref (settings);

  g_main_loop_unref (main_loop);
}

static void
nesting_test (gconstpointer test_data)
{
  HKEY       hpath;
  LONG       result;
  Change     change;
  gchar     *string;
  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *s1, *s2, *s3;
  
  s1 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/");
  s2 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/nested/");
  s3 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                 "/tests/storage/nested/even/further/");

  g_signal_connect (s1, "changed", G_CALLBACK (single_change_handler), &change);

  g_settings_set (s3, "marker", "ms", "bird");
 
  if (reg_open_path ("tests\\storage\\nested\\even\\further", &hpath))
    {
      result = RegSetValueExA (hpath, "marker", 0, REG_SZ, "\"tasty food\"", 12);
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error setting value 'intruder'");
    }
  RegCloseKey (hpath);

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
  gint       i, j;
  GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);
  GSettings *settings[100], *s0;

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

  for (i=0; i<100; i++)
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
      const char *string,
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

int
main (int argc, char **argv)
{
  HKEY hparent;
  gint result;

  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/gsettings/notify/Basic", NULL, basic_test);
  g_test_add_data_func ("/gsettings/notify/Manual", NULL, manual_test);
  g_test_add_data_func ("/gsettings/notify/Breakage", NULL, breakage_test);
  g_test_add_data_func ("/gsettings/notify/Nesting", NULL, nesting_test);
  g_test_add_data_func ("/gsettings/notify/Stress", NULL, stress_test);

  result = g_test_run();

  /* If all the tests pass, now we delete the evidence */
  if (reg_open_path (NULL, &hparent))
    {
      wchar_t *subpath = L"tests\\storage";
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
