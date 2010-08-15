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
#include <windows.h>
#include <glib.h>

#include <gio/gio.h>

static void
main_iterate ()
{
  int i;
  for (i=0; i<10; i++)
    {
      g_main_context_iteration (NULL, FALSE);
      g_usleep (100);
    }
}

/* test_unsubscribe:
 *   Do something after the settings object has been destroyed, to make sure the storage isn't still
 *   giving it change notifications.
 */
/*static void
test_unsubscribe (gconstpointer test_data)
{
  GTree *tree;
  GVariant *value;
  GSettings *settings = g_settings_new ("org.gsettings.storage-test");

  value = g_variant_new ("s", "");
  tree = g_settings_storage_new_tree ();
  g_tree_insert (tree, g_strdup (""), g_variant_ref_sink (value));
  g_settings_storage_write (storage, "/test/gsettings/storage/junk", tree, NULL);
  g_tree_unref (tree);

  g_object_unref (settings);

  value = g_variant_new ("s", "Modern life is rubbish");
  tree = g_settings_storage_new_tree ();
  g_tree_insert (tree, g_strdup (""), g_variant_ref_sink (value));
  g_settings_storage_write (storage, "/test/gsettings/storage/junk", tree, NULL);
  g_tree_unref (tree);

  main_iterate ();

  g_object_unref (storage);
}*/

typedef struct {
  /* Need a main loop for notifications to make their way back to the main thread */
  GMainLoop *main_loop;

  GSettings *settings;
} RegistryFixture;

void
registry_fixture_setup (RegistryFixture *fixture,
                        gconstpointer    test_data)
{
  fixture->main_loop = g_main_loop_new (NULL, FALSE);
  fixture->settings = g_settings_new ("org.gsettings.storage-test");
}

void
registry_fixture_teardown (RegistryFixture *fixture,
                           gconstpointer    test_data)
{
  g_object_unref (fixture->settings);
  g_main_loop_unref (fixture->main_loop);
}

static void
test_rn_1_changed_handler (GSettings   *settings,
                           const gchar *key,
                           gboolean    *flag)
{
  printf ("test registry notify 1: changed: %s\n", key);
  if (strcmp (key, "string") == 0)
    *flag = TRUE;
}

static void
test_registry_notify_1 (RegistryFixture *fixture,
                        gconstpointer    test_data)
{
  char *string;
  HKEY root_key;
  LONG result;
  gboolean change_received = FALSE;

  g_signal_connect (fixture->settings, "changed", G_CALLBACK (test_rn_1_changed_handler),
                    &change_received);

  g_settings_set_string (fixture->settings, "string", "Notify me");

  main_iterate ();
};


int
main (int argc, char **argv)
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  //g_test_add_data_func ("/gsettings/notify/Unsubscribe", NULL,
  //                      test_unsubscribe);
  g_test_add ("/gsettings/notify/Windows Registry 1", RegistryFixture, 0,
              registry_fixture_setup, test_registry_notify_1, registry_fixture_teardown);

  return g_test_run();
};
