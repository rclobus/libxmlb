/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <gio/gio.h>

#include "xb-builder.h"
#include "xb-silo-export.h"
#include "xb-silo-query.h"
#include "xb-node.h"

typedef struct {
	GPtrArray		*cmd_array;
	gboolean		 force;
} XbToolPrivate;

static void
xb_tool_private_free (XbToolPrivate *priv)
{
	if (priv == NULL)
		return;
	if (priv->cmd_array != NULL)
		g_ptr_array_unref (priv->cmd_array);
	g_free (priv);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
G_DEFINE_AUTOPTR_CLEANUP_FUNC(XbToolPrivate, xb_tool_private_free)
#pragma clang diagnostic pop

typedef gboolean (*FuUtilPrivateCb)	(XbToolPrivate	*util,
					 gchar		**values,
					 GError		**error);

typedef struct {
	gchar		*name;
	gchar		*arguments;
	gchar		*description;
	FuUtilPrivateCb	 callback;
} FuUtilItem;

static void
xb_tool_item_free (FuUtilItem *item)
{
	g_free (item->name);
	g_free (item->arguments);
	g_free (item->description);
	g_free (item);
}

static gint
xb_tool_sort_command_name_cb (FuUtilItem **item1, FuUtilItem **item2)
{
	return g_strcmp0 ((*item1)->name, (*item2)->name);
}

static void
xb_tool_add (GPtrArray *array,
	     const gchar *name,
	     const gchar *arguments,
	     const gchar *description,
	     FuUtilPrivateCb callback)
{
	g_auto(GStrv) names = NULL;

	g_return_if_fail (name != NULL);
	g_return_if_fail (description != NULL);
	g_return_if_fail (callback != NULL);

	/* add each one */
	names = g_strsplit (name, ",", -1);
	for (guint i = 0; names[i] != NULL; i++) {
		FuUtilItem *item = g_new0 (FuUtilItem, 1);
		item->name = g_strdup (names[i]);
		if (i == 0) {
			item->description = g_strdup (description);
		} else {
			/* TRANSLATORS: this is a command alias, e.g. 'get-devices' */
			item->description = g_strdup_printf ("Alias to %s",
							     names[0]);
		}
		item->arguments = g_strdup (arguments);
		item->callback = callback;
		g_ptr_array_add (array, item);
	}
}

static gchar *
xb_tool_get_descriptions (GPtrArray *array)
{
	gsize len;
	const gsize max_len = 31;
	FuUtilItem *item;
	GString *string;

	/* print each command */
	string = g_string_new ("");
	for (guint i = 0; i < array->len; i++) {
		item = g_ptr_array_index (array, i);
		g_string_append (string, "  ");
		g_string_append (string, item->name);
		len = strlen (item->name) + 2;
		if (item->arguments != NULL) {
			g_string_append (string, " ");
			g_string_append (string, item->arguments);
			len += strlen (item->arguments) + 1;
		}
		if (len < max_len) {
			for (guint j = len; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		} else {
			g_string_append_c (string, '\n');
			for (guint j = 0; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		}
	}

	/* remove trailing newline */
	if (string->len > 0)
		g_string_set_size (string, string->len - 1);

	return g_string_free (string, FALSE);
}

static gboolean
xb_tool_run (XbToolPrivate *priv,
	     const gchar *command,
	     gchar **values,
	     GError **error)
{
	/* find command */
	for (guint i = 0; i < priv->cmd_array->len; i++) {
		FuUtilItem *item = g_ptr_array_index (priv->cmd_array, i);
		if (g_strcmp0 (item->name, command) == 0)
			return item->callback (priv, values, error);
	}

	/* not found */
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Command not found");
	return FALSE;
}

static gboolean
xb_tool_dump (XbToolPrivate *priv, gchar **values, GError **error)
{
	XbSiloLoadFlags flags = XB_SILO_LOAD_FLAG_NONE;

	/* check args */
	if (g_strv_length (values) < 1) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Invalid arguments, expected "
				     "FILENAME"
				     " -- e.g. `example.xmlb`");
		return FALSE;
	}

	/* don't check the magic to make fuzzing easier */
	if (priv->force)
		flags |= XB_SILO_LOAD_FLAG_NO_MAGIC;

	/* load blobs */
	for (guint i = 0; values[i] != NULL; i++) {
		g_autofree gchar *str = NULL;
		g_autoptr(GFile) file = g_file_new_for_path (values[0]);
		g_autoptr(XbSilo) silo = xb_silo_new ();
		if (!xb_silo_load_from_file (silo, file, flags, error))
			return FALSE;
		str = xb_silo_to_string (silo, error);
		if (str == NULL)
			return FALSE;
		g_print ("%s", str);
	}
	return TRUE;
}

static gboolean
xb_tool_export (XbToolPrivate *priv, gchar **values, GError **error)
{
	XbSiloLoadFlags flags = XB_SILO_LOAD_FLAG_NONE;

	/* check args */
	if (g_strv_length (values) < 1) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Invalid arguments, expected "
				     "FILENAME"
				     " -- e.g. `example.xmlb`");
		return FALSE;
	}

	/* don't check the magic to make fuzzing easier */
	if (priv->force)
		flags |= XB_SILO_LOAD_FLAG_NO_MAGIC;

	/* load blobs */
	for (guint i = 0; values[i] != NULL; i++) {
		g_autofree gchar *str = NULL;
		g_autoptr(GFile) file = g_file_new_for_path (values[0]);
		g_autoptr(XbSilo) silo = xb_silo_new ();
		if (!xb_silo_load_from_file (silo, file, flags, error))
			return FALSE;
		str = xb_silo_export (silo,
				      XB_NODE_EXPORT_FLAG_ADD_HEADER |
				      XB_NODE_EXPORT_FLAG_FORMAT_MULTILINE |
				      XB_NODE_EXPORT_FLAG_FORMAT_INDENT |
				      XB_NODE_EXPORT_FLAG_INCLUDE_SIBLINGS,
				      error);
		if (str == NULL)
			return FALSE;
		g_print ("%s", str);
	}
	return TRUE;
}

static gboolean
xb_tool_query (XbToolPrivate *priv, gchar **values, GError **error)
{
	g_autoptr(GFile) file = NULL;
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbSilo) silo = xb_silo_new ();

	/* check args */
	if (g_strv_length (values) != 2) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Invalid arguments, expected "
				     "FILENAME QUERY"
				     " -- e.g. `example.xmlb`");
		return FALSE;
	}

	/* load blob */
	file = g_file_new_for_path (values[0]);
	if (!xb_silo_load_from_file (silo, file, XB_SILO_LOAD_FLAG_NONE, error))
		return FALSE;

	/* query */
	n = xb_silo_query_first (silo, values[1], error);
	if (n == NULL)
		return FALSE;

	g_print ("RESULT: %s\n", xb_node_get_text (n));
	return TRUE;
}

static gboolean
xb_tool_compile (XbToolPrivate *priv, gchar **values, GError **error)
{
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	g_autoptr(GFile) file_dst = NULL;

	/* check args */
	if (g_strv_length (values) < 2) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Invalid arguments, expected "
				     "FILE-IN FILE-OUT"
				     " -- e.g. `example.xml example.xmlb`");
		return FALSE;
	}

	/* load file */
	for (guint i = 1; values[i] != NULL; i++) {
		g_autoptr(GFile) file = g_file_new_for_path (values[i]);
		if (!xb_builder_import_file (builder, file, NULL, error))
			return FALSE;
	}
	file_dst = g_file_new_for_path (values[0]);
	silo = xb_builder_ensure (builder, file_dst,
				  XB_BUILDER_COMPILE_FLAG_LITERAL_TEXT |
				  XB_BUILDER_COMPILE_FLAG_NATIVE_LANGS,
				  NULL, error);
	if (silo == NULL)
		return FALSE;

	/* success */
	return TRUE;
}

int
main (int argc, char *argv[])
{
	gboolean ret;
	gboolean verbose = FALSE;
	g_autofree gchar *cmd_descriptions = NULL;
	g_autoptr(XbToolPrivate) priv = g_new0 (XbToolPrivate, 1);
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = NULL;
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			"Print verbose debug statements", NULL },
		{ "force", 'v', 0, G_OPTION_ARG_NONE, &priv->force,
			"Force parsing of invalid files", NULL },
		{ NULL}
	};

	/* add commands */
	priv->cmd_array = g_ptr_array_new_with_free_func ((GDestroyNotify) xb_tool_item_free);
	xb_tool_add (priv->cmd_array,
		     "dump",
		     "FILENAME",
		     /* TRANSLATORS: command description */
		     "Dumps a XMLb file",
		     xb_tool_dump);
	xb_tool_add (priv->cmd_array,
		     "export",
		     "FILENAME",
		     /* TRANSLATORS: command description */
		     "Exports a XMLb file",
		     xb_tool_export);
	xb_tool_add (priv->cmd_array,
		     "query",
		     "FILENAME",
		     /* TRANSLATORS: command description */
		     "Queries a XMLb file",
		     xb_tool_query);
	xb_tool_add (priv->cmd_array,
		     "compile",
		     "FILENAME-FROM FILENAME-TO",
		     /* TRANSLATORS: command description */
		     "Compile XML to XMLb",
		     xb_tool_compile);

	/* sort by command name */
	g_ptr_array_sort (priv->cmd_array,
			  (GCompareFunc) xb_tool_sort_command_name_cb);

	/* get a list of the commands */
	context = g_option_context_new (NULL);
	cmd_descriptions = xb_tool_get_descriptions (priv->cmd_array);
	g_option_context_set_summary (context, cmd_descriptions);

	/* TRANSLATORS: DFU stands for device firmware update */
	g_set_application_name ("Binary XML Utility");
	g_option_context_add_main_entries (context, options, NULL);
	ret = g_option_context_parse (context, &argc, &argv, &error);
	if (!ret) {
		g_print ("%s: %s\n", "Failed to parse arguments", error->message);
		return EXIT_FAILURE;
	}

	/* set verbose? */
	if (verbose)
		g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);

	/* run the specified command */
	ret = xb_tool_run (priv, argv[1], (gchar**) &argv[2], &error);
	if (!ret) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_FAILED)) {
			g_autofree gchar *tmp = NULL;
			tmp = g_option_context_get_help (context, TRUE, NULL);
			g_print ("%s\n\n%s", error->message, tmp);
		} else {
			g_print ("%s\n", error->message);
		}
		return EXIT_FAILURE;
	}

	/* success/ */
	return EXIT_SUCCESS;
}
