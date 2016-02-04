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

#include <glib.h>
#include <windows.h>

#ifndef __UTILS_H__
#define __UTILS_H__

G_BEGIN_DECLS

void g_warning_win32_error (DWORD        result_code,
                            const gchar *format,
                            ...);

#define g_assert_no_win32_error(_r, _m)  G_STMT_START \
  if ((_r) != ERROR_SUCCESS)                         \
    g_warning_win32_error ((_r), (_m));  G_STMT_END

gboolean util_registry_open_path (const gchar *key_name,
                                  HKEY        *hkey);

void util_main_iterate (void);

G_END_DECLS

#endif /* __UTILS_H__ */
