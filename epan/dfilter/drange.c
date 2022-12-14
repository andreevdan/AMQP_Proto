/* drange.c
 * Routines for providing general range support to the dfilter library
 *
 * Copyright (c) 2000 by Ed Warnicke <hagbard@physics.rutgers.edu>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs
 * Copyright 1999 Gerald Combs
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "drange.h"
#include <wsutil/ws_assert.h>

/* drange_node constructor */
drange_node*
drange_node_new(void)
{
    drange_node* new_range_node;

    new_range_node = g_new(drange_node,1);
    new_range_node->start_offset = 0;
    new_range_node->length = 0;
    new_range_node->end_offset = 0;
    new_range_node->ending = DRANGE_NODE_END_T_UNINITIALIZED;
    return new_range_node;
}

static drange_node*
drange_node_dup(drange_node *org)
{
    drange_node *new_range_node;

    if (!org)
        return NULL;

    new_range_node = g_new(drange_node,1);
    new_range_node->start_offset = org->start_offset;
    new_range_node->length = org->length;
    new_range_node->end_offset = org->end_offset;
    new_range_node->ending = org->ending;
    return new_range_node;
}

/* drange_node destructor */
void
drange_node_free(drange_node* drnode)
{
    g_free(drnode);
}

/* drange_node accessors */
gint
drange_node_get_start_offset(drange_node* drnode)
{
    ws_assert(drnode->ending != DRANGE_NODE_END_T_UNINITIALIZED);
    return drnode->start_offset;
}

gint
drange_node_get_length(drange_node* drnode)
{
    ws_assert(drnode->ending == DRANGE_NODE_END_T_LENGTH);
    return drnode->length;
}

gint
drange_node_get_end_offset(drange_node* drnode)
{
    ws_assert(drnode->ending == DRANGE_NODE_END_T_OFFSET);
    return drnode->end_offset;
}

drange_node_end_t
drange_node_get_ending(drange_node* drnode)
{
    ws_assert(drnode->ending != DRANGE_NODE_END_T_UNINITIALIZED);
    return drnode->ending;
}

/* drange_node mutators */
void
drange_node_set_start_offset(drange_node* drnode, gint offset)
{
    drnode->start_offset = offset;
}

void
drange_node_set_length(drange_node* drnode, gint length)
{
    drnode->length = length;
    drnode->ending = DRANGE_NODE_END_T_LENGTH;
}

void
drange_node_set_end_offset(drange_node* drnode, gint offset)
{
    drnode->end_offset = offset;
    drnode->ending = DRANGE_NODE_END_T_OFFSET;
}


void
drange_node_set_to_the_end(drange_node* drnode)
{
    drnode->ending = DRANGE_NODE_END_T_TO_THE_END;
}

/* drange constructor */
drange_t *
drange_new(void)
{
    drange_t * new_drange;
    new_drange = g_new(drange_t,1);
    new_drange->range_list = NULL;
    new_drange->has_total_length = TRUE;
    new_drange->total_length = 0;
    new_drange->min_start_offset = G_MAXINT;
    new_drange->max_start_offset = G_MININT;
    return new_drange;
}

static void
drange_append_wrapper(gpointer data, gpointer user_data)
{
    drange_node *drnode = (drange_node *)data;
    drange_t    *dr             = (drange_t *)user_data;

    drange_append_drange_node(dr, drnode);
}

drange_t *
drange_new_from_list(GSList *list)
{
    drange_t    *new_drange;

    new_drange = drange_new();
    g_slist_foreach(list, drange_append_wrapper, new_drange);
    return new_drange;
}

drange_t *
drange_dup(drange_t *org)
{
    drange_t *new_drange;
    GSList *p;

    if (!org)
        return NULL;

    new_drange = drange_new();
    for (p = org->range_list; p; p = p->next) {
        drange_node *drnode = (drange_node *)p->data;
        drange_append_drange_node(new_drange, drange_node_dup(drnode));
    }
    return new_drange;
}


/* drange destructor */
void
drange_free(drange_t * dr)
{
    drange_node_free_list(dr->range_list);
    g_free(dr);
}

/* Call drange_node destructor on all list items */
void
drange_node_free_list(GSList* list)
{
    g_slist_free_full(list, g_free);
}

/* drange accessors */
gboolean drange_has_total_length(drange_t * dr) { return dr->has_total_length; }
gint drange_get_total_length(drange_t * dr)     { return dr->total_length; }
gint drange_get_min_start_offset(drange_t * dr) { return dr->min_start_offset; }
gint drange_get_max_start_offset(drange_t * dr) { return dr->max_start_offset; }

static void
update_drange_with_node(drange_t *dr, drange_node *drnode)
{
    if(drnode->ending == DRANGE_NODE_END_T_TO_THE_END){
        dr->has_total_length = FALSE;
    }
    else if(dr->has_total_length){
        dr->total_length += drnode->length;
    }
    if(drnode->start_offset < dr->min_start_offset){
        dr->min_start_offset = drnode->start_offset;
    }
    if(drnode->start_offset > dr->max_start_offset){
        dr->max_start_offset = drnode->start_offset;
    }
}

/* drange mutators */
void
drange_prepend_drange_node(drange_t * dr, drange_node* drnode)
{
    if(drnode != NULL){
        dr->range_list = g_slist_prepend(dr->range_list,drnode);
        update_drange_with_node(dr, drnode);
    }
}

void
drange_append_drange_node(drange_t * dr, drange_node* drnode)
{
    if(drnode != NULL){
        dr->range_list = g_slist_append(dr->range_list,drnode);
        update_drange_with_node(dr, drnode);
    }
}

void
drange_foreach_drange_node(drange_t * dr, GFunc func, gpointer funcdata)
{
    g_slist_foreach(dr->range_list,func,funcdata);
}

static void
_sprint_drange_node(GString *repr, drange_node *rn)
{
    if (rn->ending == DRANGE_NODE_END_T_TO_THE_END)
        g_string_append_printf(repr, "%d:",
                            rn->start_offset);
    else if(rn->ending == DRANGE_NODE_END_T_OFFSET)
        g_string_append_printf(repr, "%d-%d",
                            rn->start_offset, rn->end_offset);
    else if (rn->ending == DRANGE_NODE_END_T_LENGTH)
        g_string_append_printf(repr, "%d:%d",
                            rn->start_offset, rn->length);
    else
        g_string_append_printf(repr, "%d/%d/%d/U",
                            rn->start_offset, rn->length, rn->end_offset);
}

char *
drange_tostr(drange_t *dr)
{
    GString *repr = g_string_new("");
    GSList *range_list = dr->range_list;

    while (range_list) {
        _sprint_drange_node(repr, (drange_node *)(range_list->data));
        range_list = g_slist_next(range_list);
        if (range_list != NULL) {
            g_string_append_c(repr, ',');
        }
    }

    return g_string_free(repr, FALSE);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
