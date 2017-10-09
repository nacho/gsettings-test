/*
 * Copyright © 2009-10 Sam Thursfield
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
#include "storage-test-enums.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include "utils.h"

#define TEST_TYPE(_s, _t, _f, _k, _d, _i)  { \
  _t value;                                  \
  /* Check default/existing value */         \
  g_settings_get (_s, _k, _f, &value, NULL); \
  g_assert (value == _d);                    \
  /* Set a value and check again */          \
  g_settings_set (_s, _k, _f, (_t)_i, NULL); \
  g_settings_get (_s, _k, _f, &value, NULL); \
  g_assert (value == _i);                    \
  /* Set back again */                       \
  g_settings_set (_s, _k, _f, _d, NULL);    }

static void
simple_test (gconstpointer user_data)
{
  GSettings *settings;
  gchar *string;

  settings = g_settings_new ("org.gsettings.test.storage-test");

  TEST_TYPE (settings, gboolean, "b", "bool", TRUE, FALSE);

  TEST_TYPE (settings, gint32, "i", "int32", 55, -999);
  TEST_TYPE (settings, gint32, "i", "a-5", 666666, 66666666);

  TEST_TYPE (settings, guint64, "x", "qword", 4398046511104, 31313131);
  TEST_TYPE (settings, guint64, "x", "qword", 4398046511104, -1);

  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Hello world");
  g_free (string);

  g_settings_set_string (settings, "string", "I like food");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "I like food");
  g_free (string);

  /* Test resetting - value should go back to default */
  g_settings_set_string (settings, "string", "Monkey Power Trio");
  g_settings_reset (settings, "string");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Hello world");
  g_free (string);

  g_object_unref (settings);
}

/* Things that don't map naturally to registry types */
static void
hard_test (gconstpointer user_data)
{
  GSettings *settings;

  settings = g_settings_new ("org.gsettings.test.storage-test");

  TEST_TYPE (settings, gdouble, "d", "double", 3.1415926535897932, -10000000000.5);

  g_object_unref (settings);
}

/* Things that don't map naturally to anything */
static void
complex_test (gconstpointer user_data)
{
  GSettings *settings;
  GVariant *variant;
  gchar **strv;

  settings = g_settings_new ("org.gsettings.test.storage-test");

  strv = g_settings_get_strv (settings, "strv");
  g_assert_cmpstr (strv[0], == , "Hello world");
  g_assert_cmpstr (strv[1], == , "Pipo");
  g_strfreev (strv);

  /* If these reads go okay, I'm sure the values are fine (unless GVariant is broken) */
  variant = g_settings_get_value (settings, "breakfast");
  g_assert_cmpstr (g_variant_get_type_string (variant), ==, "a{sd}");
  g_settings_set_value (settings, "breakfast", variant);
  g_variant_unref (variant);

  variant = g_settings_get_value (settings, "noughts-and-crosses");
  g_assert_cmpstr (g_variant_get_type_string (variant), ==, "a(iii)");
  g_settings_set_value (settings, "noughts-and-crosses", variant);
  g_variant_unref (variant);

  g_object_unref (settings);
}

/* I don't see how this could not work, but you know ... */
static void
delay_apply_test (gconstpointer user_data)
{
  GSettings *settings;
  gchar *string;

  settings = g_settings_new ("org.gsettings.test.storage-test");
  g_settings_delay (settings);

  /* If these reads go okay, I'm sure the values are fine (unless GVariant is broken) */
  g_settings_set_int (settings, "a-5", 88);
  g_settings_set_string (settings, "string", "I got 99 problems");
  g_settings_set_string (settings, "junk", "but GSettings ain't one");

  g_settings_apply (settings);
  
  g_assert_cmpint (g_settings_get_int (settings, "a-5"), ==, 88);
  g_object_unref (settings);

  settings = g_settings_new ("org.gsettings.test.storage-test");
  g_assert_cmpint (g_settings_get_int (settings, "a-5"), ==, 88);

  string = g_settings_get_string (settings, "junk");
  g_assert_cmpstr (string, ==, "but GSettings ain't one");
  g_free (string);

  g_settings_reset (settings, "a-5");
  g_settings_reset (settings, "junk");
  g_settings_reset (settings, "string");

  g_object_unref (settings);
}

/* Things that don't map naturally to anything */
static void
relocation_test (gconstpointer user_data)
{
  GSettings *settings_1, *settings_2;
  gchar *string;

  settings_1 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                         "/tests/storage/a/maze/of/twisty/little/pathnames/all/different/");
  g_settings_set (settings_1, "marker", "ms", "lamp");

  settings_2 = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                         "/tests/storage/a/maze/of/little/twisty/pathnames/all/different/");
  g_settings_set (settings_2, "marker", "ms", "pirate");

  g_settings_get (settings_1, "marker", "ms", &string);
  g_assert_cmpstr (string, ==, "lamp");

  g_object_unref (settings_2);
  g_object_unref (settings_1);
}

/* Break things as a dumb user might */
static void
breakage_test (gconstpointer user_data)
{
  GSettings *settings;
  HKEY hpath;
  gchar *string;
  gint32 int32;
  gint x, y, z;

  settings = g_settings_new ("org.gsettings.test.storage-test");

  /* Delete a value */
  g_settings_set_string (settings, "string", "Calm down");

  //printf ("Will it notify again ???\n");
  g_usleep (10000);

  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      LONG result = RegDeleteValueW (hpath, L"string");
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error deleting 'string'");

      RegCloseKey (hpath);
    }

  /* Give the change a chance to propagate. It's not a problem that it takes
   * time to propagate because in the real world because 'changed' will be
   * emitted after the read function returns
   */
  g_usleep (10000);

  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, ==, "Hello world");
  g_free (string);

  /* Change the type of a key */
  g_settings_set_int (settings, "int32", 666);

  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      LONG result = RegSetValueExW (hpath, L"int32", 0, REG_SZ,
                                    "I am not a number!!", 20);
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error breaking 'int32'");
  
      RegCloseKey (hpath);
    }

  g_usleep (10000);
  int32 = g_settings_get_int (settings, "int32");
  g_assert (int32 == 55);

  /* Break a literal */
  g_settings_set (settings, "box", "(iii)", 11, 19, 86);

  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      LONG result = RegSetValueExW (hpath, L"box", 0, REG_SZ,
                                    "£6%^*$£ Ésta GVáriant es la peor ^$^&*", 46);
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error breaking 'box'");
  
      RegCloseKey (hpath);
    }

  g_usleep (10000);

  g_settings_get (settings, "box", "(iii)", &x, &y, &z);
  g_assert (x == 20 && y == 30 && z == 30);

  /* Set a string to 0 length - this needs special handling to be treated as ""
   * rather than NULL */
  g_settings_set_string (settings, "string", "");

  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      LONG result = RegSetValueExW (hpath, L"string", 0, REG_SZ, "", 0);
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error breaking 'box'");
  
      RegCloseKey (hpath);
    }

  g_usleep (10000);

  g_assert_cmpstr (g_settings_get_string (settings, "string"), ==, "");

  g_object_unref (settings);


  /* Delete an entire key */
  settings = g_settings_new_with_path ("org.gsettings.test.storage-test.long-path",
                                       "/tests/storage/long-path/");
  g_settings_set (settings, "marker", "ms", "maybe... maybe not");

  if (util_registry_open_path ("tests\\storage", &hpath))
    {
      LONG result = RegDeleteKeyW (hpath, L"long-path");
      if (result != ERROR_SUCCESS)
        g_warning_win32_error (result, "Error breaking 'long-path'");
    
      RegCloseKey (hpath);
    }

  g_usleep (10000);
  g_settings_get (settings, "marker", "ms", &string);

  g_assert_cmpstr (string, ==, NULL);

  g_object_unref (settings);
}

static void
escape_test (gconstpointer user_data)
{
  GSettings *settings;
  gchar *string;
  gchar **strv;
  const gchar *value[] = { "foo\\.bar", "\\pipo\\.bar", NULL };

  settings = g_settings_new ("org.gsettings.test.storage-test");

  g_settings_set_string (settings, "string", "foo\\.bar");
  string = g_settings_get_string (settings, "string");
  g_assert_cmpstr (string, == , "foo\\.bar");
  g_free (string);

  g_settings_set_strv (settings, "strv", value);
  strv = g_settings_get_strv (settings, "strv");
  g_assert_cmpstr (strv[0], == , "foo\\.bar");
  g_assert_cmpstr (strv[1], == , "\\pipo\\.bar");
  g_strfreev (strv);

  g_object_unref (settings);
}

/* Test support for key longer than 32 characters */
static void
long_key_test(gconstpointer user_data)
{
    GSettings *settings;

    settings = g_settings_new("org.gsettings.test.storage-test");
    g_settings_delay(settings);

    g_settings_set_int(settings, "k12345678901234567890123456789012", 88);

    g_settings_apply(settings);

    g_assert_cmpint(g_settings_get_int(settings, "k12345678901234567890123456789012"), == , 88);
    g_object_unref(settings);

    settings = g_settings_new("org.gsettings.test.storage-test");
    g_assert_cmpint(g_settings_get_int(settings, "k12345678901234567890123456789012"), == , 88);

    g_settings_reset(settings, "k12345678901234567890123456789012");

    g_object_unref(settings);
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
main (int    argc,
      char **argv)
{
  gint test_result;

  g_test_init (&argc, &argv, NULL);

  delete_old_keys ();

  g_test_add_data_func ("/gsettings/Simple Types", NULL, simple_test);
  g_test_add_data_func ("/gsettings/Hard Types", NULL, hard_test);
  g_test_add_data_func ("/gsettings/Complex Types", NULL, complex_test);
  g_test_add_data_func ("/gsettings/Delay apply", NULL, delay_apply_test);
  g_test_add_data_func ("/gsettings/Relocation", NULL, relocation_test);
  g_test_add_data_func ("/gsettings/Breakage", NULL, breakage_test);
  g_test_add_data_func ("/gsettings/Escapes", NULL, escape_test);
  g_test_add_data_func ("/gsettings/Long Key", NULL, long_key_test);

  test_result = g_test_run ();

  delete_old_keys ();

  return test_result;
}
