/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN				"XbNode"

#include "config.h"

#include <glib-object.h>
#include <gio/gio.h>

#include "xb-node-private.h"
#include "xb-silo-query-private.h"

struct _XbNode
{
	GObject			 parent_instance;
	XbSilo			*silo;
	XbSiloNode		*sn;
};

G_DEFINE_TYPE (XbNode, xb_node, G_TYPE_OBJECT)

/**
 * xb_node_get_data:
 * @self: a #XbNode
 * @key: a string key, e.g. `fwupd::RemoteId`
 *
 * Gets any data that has been set on the node using xb_node_set_data().
 *
 * Returns: (transfer none): a #GBytes, or %NULL if not found
 *
 * Since: 0.1.0
 **/
GBytes *
xb_node_get_data (XbNode *self, const gchar *key)
{
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	return g_object_get_data (G_OBJECT (self), key);
}

/**
 * xb_node_set_data:
 * @self: a #XbNode
 * @key: a string key, e.g. `fwupd::RemoteId`
 * @data: a #GBytes
 *
 * Sets some data on the node which can be retrieved using xb_node_get_data().
 *
 * Since: 0.1.0
 **/
void
xb_node_set_data (XbNode *self, const gchar *key, GBytes *data)
{
	g_return_if_fail (XB_IS_NODE (self));
	g_return_if_fail (key != NULL);
	g_return_if_fail (data != NULL);
	g_object_set_data_full (G_OBJECT (self), key,
				g_bytes_ref (data),
				(GDestroyNotify) g_bytes_unref);
}

/**
 * xb_node_get_sn: (skip)
 * @self: a #XbNode
 *
 * Gets the #XbSiloNode for the node.
 *
 * Returns: (transfer none): a #XbSiloNode
 *
 * Since: 0.1.0
 **/
XbSiloNode *
xb_node_get_sn (XbNode *self)
{
	return self->sn;
}

/**
 * xb_node_get_silo: (skip)
 * @self: a #XbNode
 *
 * Gets the #XbSilo for the node.
 *
 * Returns: (transfer none): a #XbSilo
 *
 * Since: 0.1.0
 **/
XbSilo *
xb_node_get_silo (XbNode *self)
{
	return self->silo;
}

/**
 * xb_node_get_root:
 * @self: a #XbNode
 *
 * Gets the root node for the node.
 *
 * Returns: (transfer full): a #XbNode, or %NULL
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_get_root (XbNode *self)
{
	XbSiloNode *sn;
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	sn = xb_silo_get_sroot (self->silo);
	if (sn == NULL)
		return NULL;
	return xb_silo_node_create (self->silo, sn);
}

/**
 * xb_node_get_parent:
 * @self: a #XbNode
 *
 * Gets the parent node for the current node.
 *
 * Returns: (transfer full): a #XbNode, or %NULL
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_get_parent (XbNode *self)
{
	XbSiloNode *sn;
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	sn = xb_silo_node_get_parent (self->silo, self->sn);
	if (sn == NULL)
		return NULL;
	return xb_silo_node_create (self->silo, sn);
}

/**
 * xb_node_get_next:
 * @self: a #XbNode
 *
 * Gets the next sibling node for the current node.
 *
 * Returns: (transfer none): a #XbNode, or %NULL
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_get_next (XbNode *self)
{
	XbSiloNode *sn;
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	sn = xb_silo_node_get_next (self->silo, self->sn);
	if (sn == NULL)
		return NULL;
	return xb_silo_node_create (self->silo, sn);
}

/**
 * xb_node_get_child:
 * @self: a #XbNode
 *
 * Gets the first child node for the current node.
 *
 * Returns: (transfer none): a #XbNode, or %NULL
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_get_child (XbNode *self)
{
	XbSiloNode *sn;
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	sn = xb_silo_node_get_child (self->silo, self->sn);
	if (sn == NULL)
		return NULL;
	return xb_silo_node_create (self->silo, sn);
}

/**
 * xb_node_get_children:
 * @self: a #XbNode
 *
 * Gets all the children for the current node.
 *
 * Returns: (transfer container) (element-type XbNode): an array of children
 *
 * Since: 0.1.0
 **/
GPtrArray *
xb_node_get_children (XbNode *self)
{
	XbNode *n;
	GPtrArray *array = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	/* add all children */
	n = xb_node_get_child (self);
	while (n != NULL) {
		g_ptr_array_add (array, n);
		n = xb_node_get_next (n);
	}
	return array;
}

/**
 * xb_node_get_text:
 * @self: a #XbNode
 *
 * Gets the text data for a specific node.
 *
 * Returns: a string, or %NULL for unset
 *
 * Since: 0.1.0
 **/
const gchar *
xb_node_get_text (XbNode *self)
{
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	return xb_silo_node_get_text (self->silo, self->sn);
}

/**
 * xb_node_get_element:
 * @self: a #XbNode
 *
 * Gets the element name for a specific node.
 *
 * Returns: a string, or %NULL for unset
 *
 * Since: 0.1.0
 **/
const gchar *
xb_node_get_element (XbNode *self)
{
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	return xb_silo_node_get_element (self->silo, self->sn);
}

/**
 * xb_node_get_attr:
 * @self: a #XbNode
 * @name: an attribute name, e.g. "type"
 *
 * Gets some attaribute text data for a specific node.
 *
 * Returns: a string, or %NULL for unset
 *
 * Since: 0.1.0
 **/
const gchar *
xb_node_get_attr (XbNode *self, const gchar *name)
{
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	return xb_silo_node_get_attr (self->silo, self->sn, name);
}

/**
 * xb_node_get_depth:
 * @self: a #XbNode
 *
 * Gets the depth of the node to a root.
 *
 * Returns: a integer, where 0 is the root node iself.
 *
 * Since: 0.1.0
 **/
guint
xb_node_get_depth (XbNode *self)
{
	g_return_val_if_fail (XB_IS_NODE (self), 0);
	return xb_silo_node_get_depth (self->silo, self->sn);
}

/**
 * xb_node_query:
 * @self: a #XbNode
 * @xpath: an XPath, e.g. `id[abe.desktop]`
 * @limit: maximum number of results to return, or 0 for "all"
 * @error: the #GError, or %NULL
 *
 * Searches the silo using an XPath query, returning up to @limit results.
 *
 * Important note: Only a tiny subset of XPath 1.0 is supported.
 *
 * Returns: (transfer container) (element-type XbNode): results, or %NULL if unfound
 *
 * Since: 0.1.0
 **/
GPtrArray *
xb_node_query (XbNode *self, const gchar *xpath, guint limit, GError **error)
{
	g_autofree gchar *xpath2 = NULL;

	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (xpath != NULL, NULL);

	/* nodes don't have to include themselves as part of the query */
	xpath2 = g_strjoin ("/", xb_node_get_element (self), xpath, NULL);
	return xb_silo_query_with_root (xb_node_get_silo (self), self, xpath2, limit, error);
}

/**
 * xb_node_query_first:
 * @self: a #XbNode
 * @xpath: An XPath, e.g. `/components/component[@type=desktop]/id[abe.desktop]`
 * @error: the #GError, or %NULL
 *
 * Searches the node using an XPath query, returning up to one result.
 *
 * Please note: Only a tiny subset of XPath 1.0 is supported.
 *
 * Returns: (transfer full): a #XbNode, or %NULL if unfound
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_query_first (XbNode *self, const gchar *xpath, GError **error)
{
	g_autofree gchar *xpath2 = NULL;
	g_autoptr(GPtrArray) results = NULL;

	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (xpath != NULL, NULL);

	xpath2 = g_strjoin ("/", xb_node_get_element (self), xpath, NULL);
	results = xb_silo_query_with_root (xb_node_get_silo (self), self, xpath2, 1, error);
	if (results == NULL)
		return NULL;
	return g_object_ref (g_ptr_array_index (results, 0));
}

/**
 * xb_node_query_text:
 * @self: a #XbNode
 * @xpath: An XPath, e.g. `/components/component[@type=desktop]/id[abe.desktop]`
 * @error: the #GError, or %NULL
 *
 * Searches the node using an XPath query, returning up to one result.
 *
 * Please note: Only a tiny subset of XPath 1.0 is supported.
 *
 * Returns: (transfer none): a string, or %NULL if unfound
 *
 * Since: 0.1.0
 **/
const gchar *
xb_node_query_text (XbNode *self, const gchar *xpath, GError **error)
{
	const gchar *tmp;
	g_autoptr(XbNode) n = NULL;

	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (xpath != NULL, NULL);

	n = xb_node_query_first (self, xpath, error);
	if (n == NULL)
		return NULL;
	tmp = xb_node_get_text (n);
	if (tmp == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_NOT_FOUND,
				     "no text data");
		return NULL;
	}
	return tmp;
}

/**
 * xb_node_query_export:
 * @self: a #XbNode
 * @xpath: An XPath, e.g. `/components/component[@type=desktop]/id[abe.desktop]`
 * @error: the #GError, or %NULL
 *
 * Searches the node using an XPath query, returning an XML string of the
 * result and any children.
 *
 * Please note: Only a tiny subset of XPath 1.0 is supported.
 *
 * Returns: (transfer none): a string, or %NULL if unfound
 *
 * Since: 0.1.0
 **/
gchar *
xb_node_query_export (XbNode *self, const gchar *xpath, GError **error)
{
	g_autoptr(XbNode) n = NULL;

	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	g_return_val_if_fail (xpath != NULL, NULL);

	n = xb_node_query_first (self, xpath, error);
	if (n == NULL)
		return NULL;
	return xb_node_export (n, XB_NODE_EXPORT_FLAG_NONE, error);
}

/**
 * xb_node_query_text_as_uint:
 * @self: a #XbNode
 * @xpath: An XPath, e.g. `/components/component[@type=desktop]/id[abe.desktop]`
 * @error: the #GError, or %NULL
 *
 * Searches the node using an XPath query, returning up to one result.
 *
 * Please note: Only a tiny subset of XPath 1.0 is supported.
 *
 * Returns: a guint64, or %G_MAXUINT64 if unfound
 *
 * Since: 0.1.0
 **/
guint64
xb_node_query_text_as_uint (XbNode *self, const gchar *xpath, GError **error)
{
	const gchar *tmp;
	g_autoptr(XbNode) n = NULL;

	g_return_val_if_fail (XB_IS_NODE (self), G_MAXUINT64);
	g_return_val_if_fail (xpath != NULL, G_MAXUINT64);

	n = xb_node_query_first (self, xpath, error);
	if (n == NULL)
		return G_MAXUINT64;
	tmp = xb_node_get_text (n);
	if (tmp == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_NOT_FOUND,
				     "no text data");
		return G_MAXUINT64;
	}
	if (g_str_has_prefix (tmp, "0x"))
		return g_ascii_strtoull (tmp + 2, NULL, 16);
	return g_ascii_strtoull (tmp, NULL, 10);
}

/**
 * xb_node_export:
 * @self: a #XbNode
 * @flags: some #XbNodeExportFlags, e.g. #XB_NODE_EXPORT_FLAG_NONE
 * @error: the #GError, or %NULL
 *
 * Exports the node back to XML.
 *
 * Returns: XML data, or %NULL for an error
 *
 * Since: 0.1.0
 **/
gchar *
xb_node_export (XbNode *self, XbNodeExportFlags flags, GError **error)
{
	g_return_val_if_fail (XB_IS_NODE (self), NULL);
	return xb_silo_export_with_root (xb_node_get_silo (self), self, flags, error);
}

static void
xb_node_init (XbNode *self)
{
}

static void
xb_node_finalize (GObject *obj)
{
	G_OBJECT_CLASS (xb_node_parent_class)->finalize (obj);
}

static void
xb_node_class_init (XbNodeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = xb_node_finalize;
}

/**
 * xb_node_new: (skip)
 * @silo: A #XbSilo
 * @sn: A #XbSiloNode
 *
 * Creates a new node.
 *
 * Returns: a new #XbNode
 *
 * Since: 0.1.0
 **/
XbNode *
xb_node_new (XbSilo *silo, XbSiloNode *sn)
{
	XbNode *n = g_object_new (XB_TYPE_NODE, NULL);
	n->silo = silo;
	n->sn = sn;
	return n;
}