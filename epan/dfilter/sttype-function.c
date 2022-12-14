/*
 * Copyright (c) 2006 by Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * Wireshark - Network traffic analyzer
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "syntax-tree.h"
#include "sttype-function.h"
#include <wsutil/ws_assert.h>

typedef struct {
	guint32		magic;
	df_func_def_t *funcdef;
	GSList *params;
} function_t;

#define FUNCTION_MAGIC	0xe10f0f99

static gpointer
function_new(gpointer funcdef)
{
	function_t		*stfuncrec;

	ws_assert(funcdef != NULL);

	stfuncrec = g_new(function_t, 1);

	stfuncrec->magic = FUNCTION_MAGIC;
	stfuncrec->funcdef = (df_func_def_t *)funcdef;
	stfuncrec->params = NULL;

	return (gpointer) stfuncrec;
}

static gpointer
function_dup(gconstpointer data)
{
	const function_t *org = (const function_t *)data;
	function_t		 *stfuncrec;
	GSList *p;

	stfuncrec = (function_t *)function_new(org->funcdef);

	for (p = org->params; p; p = p->next) {
		const stnode_t *param = (const stnode_t *)p->data;
		stfuncrec->params = g_slist_append(stfuncrec->params, stnode_dup(param));
	}
	return (gpointer) stfuncrec;
}

static char *
function_tostr(const void *data)
{
	const function_t *stfuncrec = (const function_t *)data;
	const df_func_def_t *def = stfuncrec->funcdef;
	GSList *params = stfuncrec->params;
	GString *repr = g_string_new("");
	char *s;

	ws_assert(def);

	g_string_printf(repr, "%s(", def->name);
	while (params != NULL) {
		ws_assert(params->data);
		s = stnode_tostr(params->data);
		g_string_append(repr, s);
		g_free(s);
		params = params->next;
		if (params != NULL) {
			g_string_append(repr, ", ");
		}
	}
	g_string_append_c(repr, ')');

	return g_string_free(repr, FALSE);
}

static void
slist_stnode_free(gpointer data)
{
	stnode_free((stnode_t *)data);
}

void
st_funcparams_free(GSList *params)
{
	g_slist_free_full(params, slist_stnode_free);
}

static void
function_free(gpointer value)
{
	function_t	*stfuncrec = (function_t*)value;
	ws_assert_magic(stfuncrec, FUNCTION_MAGIC);
	st_funcparams_free(stfuncrec->params);
	g_free(stfuncrec);
}


/* Set the parameters for a function stnode_t. */
void
sttype_function_set_params(stnode_t *node, GSList *params)
{

	function_t	*stfuncrec;

	stfuncrec = (function_t*)stnode_data(node);
	ws_assert_magic(stfuncrec, FUNCTION_MAGIC);

	stfuncrec->params = params;
}

/* Get the function-definition record for a function stnode_t. */
df_func_def_t*
sttype_function_funcdef(stnode_t *node)
{
	function_t	*stfuncrec;

	stfuncrec = (function_t*)stnode_data(node);
	ws_assert_magic(stfuncrec, FUNCTION_MAGIC);
	return stfuncrec->funcdef;
}

/* Get the parameters for a function stnode_t. */
GSList*
sttype_function_params(stnode_t *node)
{
	function_t	*stfuncrec;

	stfuncrec = (function_t*)stnode_data(node);
	ws_assert_magic(stfuncrec, FUNCTION_MAGIC);
	return stfuncrec->params;
}


void
sttype_register_function(void)
{
	static sttype_t function_type = {
		STTYPE_FUNCTION,
		"FUNCTION",
		function_new,
		function_free,
		function_dup,
		function_tostr
	};

	sttype_register(&function_type);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
