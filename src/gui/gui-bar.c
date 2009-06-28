/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-bar.c: bar functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-window.h"


char *gui_bar_option_string[GUI_BAR_NUM_OPTIONS] =
{ "hidden", "priority", "type", "conditions", "position", "filling_top_bottom",
  "filling_left_right", "size", "size_max", "color_fg", "color_delim",
  "color_bg", "separator", "items" };
char *gui_bar_option_default[GUI_BAR_NUM_OPTIONS] =
{ "0", "0", "0", "", "top", "horizontal",
  "vertical", "0", "0", "default", "default",
  "default", "off", "" };
char *gui_bar_type_string[GUI_BAR_NUM_TYPES] =
{ "root", "window" };
char *gui_bar_position_string[GUI_BAR_NUM_POSITIONS] =
{ "bottom", "top", "left", "right" };
char *gui_bar_filling_string[GUI_BAR_NUM_FILLING] =
{ "horizontal", "vertical", "columns_horizontal", "columns_vertical" };

struct t_gui_bar *gui_bars = NULL;         /* first bar                     */
struct t_gui_bar *last_gui_bar = NULL;     /* last bar                      */

struct t_gui_bar *gui_temp_bars = NULL;    /* bars used when reading config */
struct t_gui_bar *last_gui_temp_bar = NULL;


void gui_bar_free_bar_windows (struct t_gui_bar *bar);


/*
 * gui_bar_valid: check if a bar pointer exists
 *                return 1 if bar exists
 *                       0 if bar is not found
 */

int
gui_bar_valid (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;
    
    if (!bar)
        return 0;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar == bar)
            return 1;
    }
    
    /* bar not found */
    return 0;
}

/*
 * gui_bar_search_option search a bar option name
 *                       return index of option in array
 *                       "gui_bar_option_string", or -1 if not found
 */

int
gui_bar_search_option (const char *option_name)
{
    int i;
    
    if (!option_name)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        if (string_strcasecmp (gui_bar_option_string[i], option_name) == 0)
            return i;
    }
    
    /* bar option not found */
    return -1;
}

/*
 * gui_bar_search_type: search type number with string
 *                      return -1 if type is not found
 */

int
gui_bar_search_type (const char *type)
{
    int i;
    
    if (!type)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        if (string_strcasecmp (gui_bar_type_string[i], type) == 0)
            return i;
    }
    
    /* type not found */
    return -1;
}

/*
 * gui_bar_search_position: search position number with string
 *                          return -1 if type is not found
 */

int
gui_bar_search_position (const char *position)
{
    int i;
    
    if (!position)
        return -1;
    
    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        if (string_strcasecmp (gui_bar_position_string[i], position) == 0)
            return i;
    }
    
    /* position not found */
    return -1;
}

/*
 * gui_bar_get_min_width: return minimum width of a bar window displayed for
 *                        a bar
 *                        for example, if a bar is displayed in 3 windows,
 *                        this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_width (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_width;
    
    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->width;
    
    min_width = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->width < min_width)
                    min_width = ptr_bar_win->width;
            }
        }
    }
    
    if (min_width == INT_MAX)
        return 0;
    
    return min_width;
}

/*
 * gui_bar_get_min_height: return minimum height of a bar window displayed for
 *                         a bar
 *                         for example, if a bar is displayed in 3 windows,
 *                         this function return min width of these 3 bar windows
 */

int
gui_bar_get_min_height (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    int min_height;
    
    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        return bar->bar_window->height;
    
    min_height = INT_MAX;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                if (ptr_bar_win->height < min_height)
                    min_height = ptr_bar_win->height;
            }
        }
    }
    
    if (min_height == INT_MAX)
        return 0;
    
    return min_height;
}

/*
 * gui_bar_check_size_add: check if "add_size" is ok for bar
 *                         return 1 if new size is ok
 *                                0 if new size is too big
 */

int
gui_bar_check_size_add (struct t_gui_bar *bar, int add_size)
{
    struct t_gui_window *ptr_window;
    int sub_width, sub_height;
    
    sub_width = 0;
    sub_height = 0;
    
    switch (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]))
    {
        case GUI_BAR_POSITION_BOTTOM:
        case GUI_BAR_POSITION_TOP:
            sub_height = add_size;
            break;
        case GUI_BAR_POSITION_LEFT:
        case GUI_BAR_POSITION_RIGHT:
            sub_width = add_size;
            break;
        case GUI_BAR_NUM_POSITIONS:
            break;
    }
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
            || (gui_bar_window_search_bar (ptr_window, bar)))
        {
            if ((ptr_window->win_chat_width - sub_width < GUI_WINDOW_CHAT_MIN_WIDTH)
                || (ptr_window->win_chat_height - sub_height < GUI_WINDOW_CHAT_MIN_HEIGHT))
                return 0;
        }
    }
    
    /* new size ok */
    return 1;
}

/*
 * gui_bar_get_filling: return filling option for bar, according to filling
 *                      for current bar position
 */

enum t_gui_bar_filling
gui_bar_get_filling (struct t_gui_bar *bar)
{
    if ((CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
        || (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP))
        return CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM]);
    
    return CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT]);
}

/*
 * gui_bar_get_item_index: return index of item and sub item in bar (position
 *                         of item in items list)
 *                         for example, if items are:
 *                            item1,sub1+sub2+sub3,item3
 *                         index of sub3 is 1, sub index is 2
 *                         return -1 for index and sub index if item is not
 *                         found in bar
 */

void
gui_bar_get_item_index (struct t_gui_bar *bar, const char *item_name,
                        int *index_item, int *index_subitem)
{
    int i, j;
    
    *index_item = -1;
    *index_subitem = -1;
    
    if (!bar || !item_name || !item_name[0])
        return;
    
    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            /* skip non letters chars at beginning (prefix) */
            if (gui_bar_item_string_is_item (bar->items_array[i][j], item_name))
            {
                *index_item = i;
                *index_subitem = j;
                return;
            }
        }
    }
    
    /* item is not in bar */
}

/*
 * gui_bar_find_pos: find position for a bar in list (keeping list sorted
 *                   by priority)
 */

struct t_gui_bar *
gui_bar_find_pos (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_PRIORITY]) >= CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_PRIORITY]))
            return ptr_bar;
    }
    
    /* bar not found, add to end of list */
    return NULL;
}

/*
 * gui_bar_insert: insert a bar to the list (at good position, according to
 *                 priority)
 */

void
gui_bar_insert (struct t_gui_bar *bar)
{
    struct t_gui_bar *pos_bar;
    
    if (gui_bars)
    {
        pos_bar = gui_bar_find_pos (bar);
        if (pos_bar)
        {
            /* insert bar into the list (before position found) */
            bar->prev_bar = pos_bar->prev_bar;
            bar->next_bar = pos_bar;
            if (pos_bar->prev_bar)
                (pos_bar->prev_bar)->next_bar = bar;
            else
                gui_bars = bar;
            pos_bar->prev_bar = bar;
        }
        else
        {
            /* add bar to the end */
            bar->prev_bar = last_gui_bar;
            bar->next_bar = NULL;
            last_gui_bar->next_bar = bar;
            last_gui_bar = bar;
        }
    }
    else
    {
        bar->prev_bar = NULL;
        bar->next_bar = NULL;
        gui_bars = bar;
        last_gui_bar = bar;
    }
}

/*
 * gui_bar_check_conditions_for_window: return 1 if bar should be displayed in
 *                                      this window, according to condition(s)
 *                                      on bar
 */

int
gui_bar_check_conditions_for_window (struct t_gui_bar *bar,
                                     struct t_gui_window *window)
{
    int i, rc;
    char str_modifier[256], str_window[128], *str_displayed;

    /* check bar conditions */
    for (i = 0; i < bar->conditions_count; i++)
    {
        if (string_strcasecmp (bar->conditions_array[i], "active") == 0)
        {
            if (gui_current_window && (gui_current_window != window))
                return 0;
        }
        else if (string_strcasecmp (bar->conditions_array[i], "inactive") == 0)
        {
            if (!gui_current_window || (gui_current_window == window))
                return 0;
        }
        else if (string_strcasecmp (bar->conditions_array[i], "nicklist") == 0)
        {
            if (window->buffer && !window->buffer->nicklist)
                return 0;
        }
    }
    
    /* call a modifier that will tell us if bar is displayed or not,
       for example it can be used to display nicklist on some buffers
       only, using a script that implements this modifier and return "1"
       to display bar, "0" to hide it */
    snprintf (str_modifier, sizeof (str_modifier),
              "bar_condition_%s", bar->name);
    snprintf (str_window, sizeof (str_window),
              "0x%lx", (long unsigned int)(window));
    str_displayed = hook_modifier_exec (NULL,
                                        str_modifier,
                                        str_window,
                                        "");
    if (str_displayed && strcmp (str_displayed, "0") == 0)
        rc = 0;
    else
        rc = 1;
    if (str_displayed)
        free (str_displayed);
    
    return rc;
}

/*
 * gui_bar_root_get_size: get total bar size ("root" type) for a position
 */

int
gui_bar_root_get_size (struct t_gui_bar *bar, enum t_gui_bar_position position)
{
    struct t_gui_bar *ptr_bar;
    int total_size;
    
    total_size = 0;
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (bar && (ptr_bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == (int)position))
            {
                total_size += gui_bar_window_get_current_size (ptr_bar->bar_window);
                if (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SEPARATOR]))
                    total_size++;
            }
        }
    }
    return total_size;
}

/*
 * gui_bar_search: search a bar by name
 */

struct t_gui_bar *
gui_bar_search (const char *name)
{
    struct t_gui_bar *ptr_bar;
    
    if (!name || !name[0])
        return NULL;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (strcmp (ptr_bar->name, name) == 0)
            return ptr_bar;
    }
    
    /* bar not found */
    return NULL;
}

/*
 * gui_bar_search_with_option_name: search a bar with name of option
 *                                  (like "uptime.type")
 */

struct t_gui_bar *
gui_bar_search_with_option_name (const char *option_name)
{
    char *bar_name, *pos_option;
    struct t_gui_bar *ptr_bar;
    
    ptr_bar = NULL;
    
    pos_option = strchr (option_name, '.');
    if (pos_option)
    {
        bar_name = string_strndup (option_name, pos_option - option_name);
        if (bar_name)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (strcmp (ptr_bar->name, bar_name) == 0)
                    break;
            }
            free (bar_name);
        }
    }
    
    return ptr_bar;
}

/*
 * gui_bar_content_build_bar_windows: rebuild content of bar windows for a bar
 */

void
gui_bar_content_build_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_window;
    
    if (!bar)
        return;
    
    if (bar->bar_window)
    {
        gui_bar_window_content_build (bar->bar_window, NULL);
    }
    else
    {
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            for (ptr_bar_window = ptr_window->bar_windows; ptr_bar_window;
                 ptr_bar_window = ptr_bar_window->next_bar_window)
            {
                if (ptr_bar_window->bar == bar)
                    gui_bar_window_content_build (ptr_bar_window, ptr_window);
            }
        }
    }
}

/*
 * gui_bar_ask_refresh: ask refresh for bar
 */

void
gui_bar_ask_refresh (struct t_gui_bar *bar)
{
    bar->bar_refresh_needed = 1;
}

/*
 * gui_bar_refresh: ask for bar refresh on screen (for all windows where bar is)
 */

void
gui_bar_refresh (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    
    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        gui_window_ask_refresh (1);
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (gui_bar_window_search_bar (ptr_win, bar))
                ptr_win->refresh_needed = 1;
        }
    }
}

/*
 * gui_bar_draw: draw a bar
 */

void
gui_bar_draw (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    
    if (!CONFIG_BOOLEAN(bar->options[GUI_BAR_OPTION_HIDDEN]))
    {    
        if (bar->bar_window)
        {
            /* root bar */
            gui_bar_window_draw (bar->bar_window, NULL);
        }
        else
        {
            /* bar on each window */
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                     ptr_bar_win = ptr_bar_win->next_bar_window)
                {
                    if (ptr_bar_win->bar == bar)
                    {
                        gui_bar_window_draw (ptr_bar_win, ptr_win);
                    }
                }
            }
        }
    }
    bar->bar_refresh_needed = 0;
}

/*
 * gui_bar_apply_current_size: apply new size for all bar windows of bar
 */

void
gui_bar_apply_current_size (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    
    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
    {
        gui_bar_window_set_current_size (bar->bar_window,
                                         NULL,
                                         CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]));
        gui_window_ask_refresh (1);
    }
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                 ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if (ptr_bar_win->bar == bar)
                {
                    gui_bar_window_set_current_size (ptr_bar_win,
                                                     ptr_win,
                                                     CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]));
                }
            }
        }
    }
}

/*
 * gui_bar_free_items_array: free array with items for a bar
 */

void
gui_bar_free_items_array (struct t_gui_bar *bar)
{
    int i;
    
    for (i = 0; i < bar->items_count; i++)
    {
        if (bar->items_array[i])
            string_free_split (bar->items_array[i]);
    }
    if (bar->items_array)
    {
        free (bar->items_array);
        bar->items_array = NULL;
    }
    if (bar->items_subcount)
    {
        free (bar->items_subcount);
        bar->items_subcount = NULL;
    }
    bar->items_count = 0;
}

/*
 * gui_bar_set_items_array: build array with items for a bar
 */

void
gui_bar_set_items_array (struct t_gui_bar *bar, const char *items)
{
    int i, count;
    char **tmp_array;
    
    gui_bar_free_items_array (bar);
    
    if (items && items[0])
    {
        tmp_array = string_split (items, ",", 0, 0, &count);
        if (count > 0)
        {
            bar->items_count = count;
            bar->items_subcount = malloc (count * sizeof (*bar->items_subcount));
            bar->items_array = malloc (count * sizeof (*bar->items_array));
            for (i = 0; i < count; i++)
            {
                bar->items_array[i] = string_split (tmp_array[i], "+", 0, 0,
                                                    &(bar->items_subcount[i]));
            }
        }
        string_free_split (tmp_array);
    }
}

/*
 * gui_bar_config_check_type: callback for checking bar type before changing it
 */

int
gui_bar_config_check_type (void *data, struct t_config_option *option,
                           const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;

    gui_chat_printf (NULL,
                     _("%sUnable to change bar type: you must delete bar "
                       "and create another to do that"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    return 0;
}

/*
 * gui_bar_config_change_hidden: callback when "hidden" flag is changed
 */

void
gui_bar_config_change_hidden (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        /* free bar windows */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            gui_bar_free_bar_windows (ptr_bar);
        }
        
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
                    && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT))
                {
                    gui_bar_window_new (ptr_bar, ptr_win);
                }
            }
        }
    }
    
    gui_window_ask_refresh (1);
}

/*
 * gui_bar_config_change_priority: callback when priority is changed
 */

void
gui_bar_config_change_priority (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        /* remove bar from list */
        if (ptr_bar == gui_bars)
        {
            gui_bars = ptr_bar->next_bar;
            gui_bars->prev_bar = NULL;
        }
        if (ptr_bar == last_gui_bar)
        {
            last_gui_bar = ptr_bar->prev_bar;
            last_gui_bar->next_bar = NULL;
        }
        if (ptr_bar->prev_bar)
            (ptr_bar->prev_bar)->next_bar = ptr_bar->next_bar;
        if (ptr_bar->next_bar)
            (ptr_bar->next_bar)->prev_bar = ptr_bar->prev_bar;
        
        gui_bar_insert (ptr_bar);
        
        /* free bar windows */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            gui_bar_free_bar_windows (ptr_bar);
        }
        
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
                    && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT))
                {
                    gui_bar_window_new (ptr_bar, ptr_win);
                }
            }
        }
    }
    
    gui_window_ask_refresh (1);
}

/*
 * gui_bar_config_change_conditions: callback when conditions is changed
 */

void
gui_bar_config_change_conditions (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        if (ptr_bar->conditions_array)
            string_free_split (ptr_bar->conditions_array);
        
        if (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS])
            && CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS])[0])
        {
            ptr_bar->conditions_array = string_split (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS]),
                                                      ",", 0, 0,
                                                      &ptr_bar->conditions_count);
        }
        else
        {
            ptr_bar->conditions_count = 0;
            ptr_bar->conditions_array = NULL;
        }
    }
    
    gui_window_ask_refresh (1);
}

/*
 * gui_bar_config_change_position: callback when position is changed
 */

void
gui_bar_config_change_position (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
    
    gui_window_ask_refresh (1);
}

/*
 * gui_bar_config_change_filling: callback when filling is changed
 */

void
gui_bar_config_change_filling (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
    
    gui_window_ask_refresh (1);
}

/*
 * gui_bar_config_check_size: callback for checking bar size before changing it
 */

int
gui_bar_config_check_size (void *data, struct t_config_option *option,
                           const char *value)
{
    struct t_gui_bar *ptr_bar;
    long number;
    char *error;
    int new_value;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        new_value = -1;
        if (strncmp (value, "++", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) + number;
            }
        }
        else if (strncmp (value, "--", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) - number;
            }
        }
        else
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                new_value = number;
            }
        }
        if (new_value < 0)
            return 0;
        
        if ((new_value > 0) &&
            ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) == 0)
             || (new_value > CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]))))
        {
            if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
                && !gui_bar_check_size_add (ptr_bar,
                                            new_value - CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE])))
                return 0;
        }
        
        return 1;
    }
    
    return 0;
}

/*
 * gui_bar_config_change_size: callback when size is changed
 */

void
gui_bar_config_change_size (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        gui_bar_apply_current_size (ptr_bar);
        gui_bar_refresh (ptr_bar);
    }
}

/*
 * gui_bar_config_change_size_max: callback when max size is changed
 */

void
gui_bar_config_change_size_max (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    char value[32];
    
    /* make C compiler happy */
    (void) data;
    (void) option;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]) > 0)
            && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) >
                CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX])))
        {
            snprintf (value, sizeof (value), "%d",
                      CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]));
            config_file_option_set (ptr_bar->options[GUI_BAR_OPTION_SIZE], value, 1);
        }
        gui_window_ask_refresh (1);
    }
}

/*
 * gui_bar_config_change_color: callback when color (fg or bg) is changed
 */

void
gui_bar_config_change_color (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
}

/*
 * gui_bar_config_change_separator: callback when separator is changed
 */

void
gui_bar_config_change_separator (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
}

/*
 * gui_bar_config_change_items: callback when items is changed
 */

void
gui_bar_config_change_items (void *data, struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        gui_bar_set_items_array (ptr_bar, CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
        
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            gui_bar_content_build_bar_windows (ptr_bar);
            gui_bar_ask_refresh (ptr_bar);
        }
    }
}

/*
 * gui_bar_set_name: set name for a bar
 */

void
gui_bar_set_name (struct t_gui_bar *bar, const char *name)
{
    int length;
    char *option_name;
    
    if (!name || !name[0])
        return;
    
    length = strlen (name) + 64;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.hidden", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_HIDDEN], option_name);
        snprintf (option_name, length, "%s.priority", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_PRIORITY], option_name);
        snprintf (option_name, length, "%s.type", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_TYPE], option_name);
        snprintf (option_name, length, "%s.conditions", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_CONDITIONS], option_name);
        snprintf (option_name, length, "%s.position", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_POSITION], option_name);
        snprintf (option_name, length, "%s.filling_top_bottom", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM], option_name);
        snprintf (option_name, length, "%s.filling_left_right", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT], option_name);
        snprintf (option_name, length, "%s.size", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_SIZE], option_name);
        snprintf (option_name, length, "%s.size_max", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_SIZE_MAX], option_name);
        snprintf (option_name, length, "%s.color_fg", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_COLOR_FG], option_name);
        snprintf (option_name, length, "%s.color_delim", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_COLOR_DELIM], option_name);
        snprintf (option_name, length, "%s.color_bg", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_COLOR_BG], option_name);
        snprintf (option_name, length, "%s.separator", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_SEPARATOR], option_name);
        snprintf (option_name, length, "%s.items", name);
        config_file_option_rename (bar->options[GUI_BAR_OPTION_ITEMS], option_name);
        
        if (bar->name)
            free (bar->name);
        bar->name = strdup (name);
        
        free (option_name);
    }
}

/*
 * gui_bar_set: set a property for a bar
 *              return: 1 if ok, 0 if error
 */

int
gui_bar_set (struct t_gui_bar *bar, const char *property, const char *value)
{
    if (!bar || !property || !value)
        return 0;
    
    if (string_strcasecmp (property, "name") == 0)
    {
        gui_bar_set_name (bar, value);
        return 1;
    }
    else if (string_strcasecmp (property, "hidden") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_HIDDEN], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "priority") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_PRIORITY], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "conditions") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_CONDITIONS], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "position") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_POSITION], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "filling_top_bottom") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "filling_left_right") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "size") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SIZE], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "size_max") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SIZE_MAX], value, 1);
        return 1;
    }
    else if (string_strcasecmp (property, "color_fg") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_FG], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "color_delim") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_DELIM], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "color_bg") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_BG], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "separator") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SEPARATOR],
                                (strcmp (value, "1") == 0) ? "on" : "off",
                                1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (string_strcasecmp (property, "items") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_ITEMS], value, 1);
        gui_bar_draw (bar);
        return 1;
    }
    
    return 0;
}

/*
 * gui_bar_create_option: create an option for a bar
 */

struct t_config_option *
gui_bar_create_option (const char *bar_name, int index_option, const char *value)
{
    struct t_config_option *ptr_option;
    int length;
    char *option_name;
    
    ptr_option = NULL;
    
    length = strlen (bar_name) + 1 +
        strlen (gui_bar_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s",
                  bar_name, gui_bar_option_string[index_option]);
        
        switch (index_option)
        {
            case GUI_BAR_OPTION_HIDDEN:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "boolean",
                    N_("true if bar is hidden, false if it is displayed"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_hidden, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_PRIORITY:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar priority (high number means bar displayed first)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_priority, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_TYPE:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar type (root, window, window_active, window_inactive)"),
                    "root|window|window_active|window_inactive", 0, 0, value, NULL, 0,
                    &gui_bar_config_check_type, NULL, NULL, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_CONDITIONS:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "string",
                    N_("condition(s) for displaying bar (for bars of type "
                       "\"window\")"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_conditions, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_POSITION:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar position (bottom, top, left, right)"),
                    "bottom|top|left|right", 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_position, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_FILLING_TOP_BOTTOM:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar filling direction (\"horizontal\" (from left to "
                       "right) or \"vertical\" (from top to bottom)) when bar "
                       "position is top or bottom"),
                    "horizontal|vertical|columns_horizontal|columns_vertical",
                    0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_filling, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_FILLING_LEFT_RIGHT:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar filling direction (\"horizontal\" (from left to "
                       "right) or \"vertical\" (from top to bottom)) when bar "
                       "position is left or right"),
                    "horizontal|vertical|columns_horizontal|columns_vertical",
                    0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_filling, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_SIZE:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("bar size in chars (0 = auto size)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    &gui_bar_config_check_size, NULL,
                    &gui_bar_config_change_size, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_SIZE_MAX:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "integer",
                    N_("max bar size in chars (0 = no limit)"),
                    NULL, 0, INT_MAX, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_size_max, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_FG:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default text color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_DELIM:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default delimiter color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_COLOR_BG:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "color",
                    N_("default background color for bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL,
                    &gui_bar_config_change_color, NULL,
                    NULL, NULL);
                break;
            case GUI_BAR_OPTION_SEPARATOR:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "boolean",
                    N_("separator line between bar and other bars/windows"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_separator, NULL, NULL, NULL);
                break;
            case GUI_BAR_OPTION_ITEMS:
                ptr_option = config_file_new_option (
                    weechat_config_file, weechat_config_section_bar,
                    option_name, "string",
                    N_("items of bar"),
                    NULL, 0, 0, value, NULL, 0,
                    NULL, NULL, &gui_bar_config_change_items, NULL, NULL, NULL);
                break;
            case GUI_BAR_NUM_OPTIONS:
                break;
        }
        free (option_name);
    }
    
    return ptr_option;
}

/*
 * gui_bar_create_option_temp: create option for a temporary bar (when reading
 *                             config file)
 */

void
gui_bar_create_option_temp (struct t_gui_bar *temp_bar, int index_option,
                            const char *value)
{
    struct t_config_option *new_option;
    
    new_option = gui_bar_create_option (temp_bar->name,
                                        index_option,
                                        value);
    if (new_option)
    {
        switch (index_option)
        {
            case GUI_BAR_OPTION_HIDDEN:
                temp_bar->options[GUI_BAR_OPTION_HIDDEN] = new_option;
                break;
            case GUI_BAR_OPTION_PRIORITY:
                temp_bar->options[GUI_BAR_OPTION_PRIORITY] = new_option;
                break;
            case GUI_BAR_OPTION_TYPE:
                temp_bar->options[GUI_BAR_OPTION_TYPE] = new_option;
                break;
            case GUI_BAR_OPTION_CONDITIONS:
                temp_bar->options[GUI_BAR_OPTION_CONDITIONS] = new_option;
                break;
            case GUI_BAR_OPTION_POSITION:
                temp_bar->options[GUI_BAR_OPTION_POSITION] = new_option;
                break;
            case GUI_BAR_OPTION_FILLING_TOP_BOTTOM:
                temp_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM] = new_option;
                break;
            case GUI_BAR_OPTION_FILLING_LEFT_RIGHT:
                temp_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT] = new_option;
                break;
            case GUI_BAR_OPTION_SIZE:
                temp_bar->options[GUI_BAR_OPTION_SIZE] = new_option;
                break;
            case GUI_BAR_OPTION_SIZE_MAX:
                temp_bar->options[GUI_BAR_OPTION_SIZE_MAX] = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_FG:
                temp_bar->options[GUI_BAR_OPTION_COLOR_FG] = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_DELIM:
                temp_bar->options[GUI_BAR_OPTION_COLOR_DELIM] = new_option;
                break;
            case GUI_BAR_OPTION_COLOR_BG:
                temp_bar->options[GUI_BAR_OPTION_COLOR_BG] = new_option;
                break;
            case GUI_BAR_OPTION_SEPARATOR:
                temp_bar->options[GUI_BAR_OPTION_SEPARATOR] = new_option;
                break;
            case GUI_BAR_OPTION_ITEMS:
                temp_bar->options[GUI_BAR_OPTION_ITEMS] = new_option;
                break;
        }
    }
}

/*
 * gui_bar_alloc: allocate and initialize new bar structure
 */

struct t_gui_bar *
gui_bar_alloc (const char *name)
{
    struct t_gui_bar *new_bar;
    int i;
    
    new_bar = malloc (sizeof (*new_bar));
    if (new_bar)
    {
        new_bar->name = strdup (name);
        for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
        {
            new_bar->options[i] = NULL;
        }
        new_bar->conditions_count = 0;
        new_bar->conditions_array = NULL;
        new_bar->items_count = 0;
        new_bar->items_array = NULL;
        new_bar->bar_window = NULL;
        new_bar->bar_refresh_needed = 0;
        new_bar->prev_bar = NULL;
        new_bar->next_bar = NULL;
    }
    
    return new_bar;
}

/*
 * gui_bar_new_with_options: create a new bar with options
 */

struct t_gui_bar *
gui_bar_new_with_options (const char *name,
                          struct t_config_option *hidden,
                          struct t_config_option *priority,
                          struct t_config_option *type,
                          struct t_config_option *conditions,
                          struct t_config_option *position,
                          struct t_config_option *filling_top_bottom,
                          struct t_config_option *filling_left_right,
                          struct t_config_option *size,
                          struct t_config_option *size_max,
                          struct t_config_option *color_fg,
                          struct t_config_option *color_delim,
                          struct t_config_option *color_bg,
                          struct t_config_option *separator,
                          struct t_config_option *items)
{
    struct t_gui_bar *new_bar;
    struct t_gui_window *ptr_win;
    
    /* create bar */
    new_bar = gui_bar_alloc (name);
    if (new_bar)
    {
        new_bar->options[GUI_BAR_OPTION_HIDDEN] = hidden;
        new_bar->options[GUI_BAR_OPTION_PRIORITY] = priority;
        new_bar->options[GUI_BAR_OPTION_TYPE] = type;
        new_bar->options[GUI_BAR_OPTION_CONDITIONS] = conditions;
        if (CONFIG_STRING(conditions) && CONFIG_STRING(conditions)[0])
        {
            new_bar->conditions_array = string_split (CONFIG_STRING(conditions),
                                                      ",", 0, 0,
                                                      &new_bar->conditions_count);
        }
        else
        {
            new_bar->conditions_count = 0;
            new_bar->conditions_array = NULL;
        }
        new_bar->options[GUI_BAR_OPTION_POSITION] = position;
        new_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM] = filling_top_bottom;
        new_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT] = filling_left_right;
        new_bar->options[GUI_BAR_OPTION_SIZE] = size;
        new_bar->options[GUI_BAR_OPTION_SIZE_MAX] = size_max;
        new_bar->options[GUI_BAR_OPTION_COLOR_FG] = color_fg;
        new_bar->options[GUI_BAR_OPTION_COLOR_DELIM] = color_delim;
        new_bar->options[GUI_BAR_OPTION_COLOR_BG] = color_bg;
        new_bar->options[GUI_BAR_OPTION_SEPARATOR] = separator;
        new_bar->options[GUI_BAR_OPTION_ITEMS] = items;
        new_bar->items_count = 0;
        new_bar->items_subcount = NULL;
        new_bar->items_array = NULL;
        gui_bar_set_items_array (new_bar, CONFIG_STRING(items));
        new_bar->bar_window = NULL;
        new_bar->bar_refresh_needed = 1;
        
        /* add bar to bars list */
        gui_bar_insert (new_bar);
        
        /* add window bar */
        if (CONFIG_INTEGER(new_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        {
            /* create only one window for bar */
            gui_bar_window_new (new_bar, NULL);
            gui_window_ask_refresh (1);
        }
        else
        {
            /* create bar window for all opened windows */
            for (ptr_win = gui_windows; ptr_win;
                 ptr_win = ptr_win->next_window)
            {
                gui_bar_window_new (new_bar, ptr_win);
            }
        }
    }
    
    return new_bar;
}

/*
 * gui_bar_new: create a new bar
 */

struct t_gui_bar *
gui_bar_new (const char *name, const char *hidden, const char *priority,
             const char *type, const char *conditions, const char *position,
             const char *filling_top_bottom, const char *filling_left_right,
             const char *size, const char *size_max,
             const char *color_fg, const char *color_delim,
             const char *color_bg, const char *separators, const char *items)
{
    struct t_config_option *option_hidden, *option_priority, *option_type;
    struct t_config_option *option_conditions, *option_position;
    struct t_config_option *option_filling_top_bottom, *option_filling_left_right;
    struct t_config_option *option_size, *option_size_max;
    struct t_config_option *option_color_fg, *option_color_delim;
    struct t_config_option *option_color_bg, *option_separator;
    struct t_config_option *option_items;
    struct t_gui_bar *new_bar;
    
    if (!name || !name[0])
        return NULL;
    
    /* it's not possible to create 2 bars with same name */
    if (gui_bar_search (name))
        return NULL;
    
    /* look for type */
    if (gui_bar_search_type (type) < 0)
        return NULL;
    
    /* look for position */
    if (gui_bar_search_position (position) < 0)
        return NULL;
    
    option_hidden = gui_bar_create_option (name, GUI_BAR_OPTION_HIDDEN,
                                           hidden);
    option_priority = gui_bar_create_option (name, GUI_BAR_OPTION_PRIORITY,
                                             priority);
    option_type = gui_bar_create_option (name, GUI_BAR_OPTION_TYPE,
                                         type);
    option_conditions = gui_bar_create_option (name, GUI_BAR_OPTION_CONDITIONS,
                                               conditions);
    option_position = gui_bar_create_option (name, GUI_BAR_OPTION_POSITION,
                                             position);
    option_filling_top_bottom = gui_bar_create_option (name, GUI_BAR_OPTION_FILLING_TOP_BOTTOM,
                                                       filling_top_bottom);
    option_filling_left_right = gui_bar_create_option (name, GUI_BAR_OPTION_FILLING_LEFT_RIGHT,
                                                       filling_left_right);
    option_size = gui_bar_create_option (name, GUI_BAR_OPTION_SIZE,
                                         size);
    option_size_max = gui_bar_create_option (name, GUI_BAR_OPTION_SIZE_MAX,
                                             size_max);
    option_color_fg = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_FG,
                                             color_fg);
    option_color_delim = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_DELIM,
                                                color_delim);
    option_color_bg = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_BG,
                                             color_bg);
    option_separator = gui_bar_create_option (name, GUI_BAR_OPTION_SEPARATOR,
                                              (config_file_string_to_boolean (separators)) ?
                                               "on" : "off");
    option_items = gui_bar_create_option (name, GUI_BAR_OPTION_ITEMS,
                                          items);
    new_bar = gui_bar_new_with_options (name, option_hidden,
                                        option_priority, option_type,
                                        option_conditions, option_position,
                                        option_filling_top_bottom,
                                        option_filling_left_right,
                                        option_size, option_size_max,
                                        option_color_fg, option_color_delim,
                                        option_color_bg, option_separator,
                                        option_items);
    if (!new_bar)
    {
        if (option_hidden)
            config_file_option_free (option_hidden);
        if (option_priority)
            config_file_option_free (option_priority);
        if (option_type)
            config_file_option_free (option_type);
        if (option_conditions)
            config_file_option_free (option_conditions);
        if (option_position)
            config_file_option_free (option_position);
        if (option_filling_top_bottom)
            config_file_option_free (option_filling_top_bottom);
        if (option_filling_left_right)
            config_file_option_free (option_filling_left_right);
        if (option_size)
            config_file_option_free (option_size);
        if (option_size_max)
            config_file_option_free (option_size_max);
        if (option_color_fg)
            config_file_option_free (option_color_fg);
        if (option_color_delim)
            config_file_option_free (option_color_delim);
        if (option_color_bg)
            config_file_option_free (option_color_bg);
        if (option_separator)
            config_file_option_free (option_separator);
        if (option_items)
            config_file_option_free (option_items);
    }
    
    return new_bar;
}

/*
 * gui_bar_use_temp_bars: use temp bars (created by reading config file)
 */

void
gui_bar_use_temp_bars ()
{
    struct t_gui_bar *ptr_temp_bar, *next_temp_bar;
    int i, num_options_ok;
    
    for (ptr_temp_bar = gui_temp_bars; ptr_temp_bar;
         ptr_temp_bar = ptr_temp_bar->next_bar)
    {
        num_options_ok = 0;
        for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_bar->options[i])
            {
                ptr_temp_bar->options[i] = gui_bar_create_option (ptr_temp_bar->name,
                                                                  i,
                                                                  gui_bar_option_default[i]);
            }
            if (ptr_temp_bar->options[i])
                num_options_ok++;
        }
        
        if (num_options_ok == GUI_BAR_NUM_OPTIONS)
        {
            gui_bar_new_with_options (ptr_temp_bar->name,
                                      ptr_temp_bar->options[GUI_BAR_OPTION_HIDDEN],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_PRIORITY],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_TYPE],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_CONDITIONS],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_POSITION],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_SIZE],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_SIZE_MAX],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_FG],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_DELIM],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_BG],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_SEPARATOR],
                                      ptr_temp_bar->options[GUI_BAR_OPTION_ITEMS]);
        }
        else
        {
            for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
            {
                if (ptr_temp_bar->options[i])
                {
                    config_file_option_free (ptr_temp_bar->options[i]);
                    ptr_temp_bar->options[i] = NULL;
                }
            }
        }
    }
    
    /* free all temp bars */
    while (gui_temp_bars)
    {
        next_temp_bar = gui_temp_bars->next_bar;
        
        if (gui_temp_bars->name)
            free (gui_temp_bars->name);
        free (gui_temp_bars);
        
        gui_temp_bars = next_temp_bar;
    }
    last_gui_temp_bar = NULL;
}

/*
 * gui_bar_create_default_input: create default input bar if it does not exist
 */

void
gui_bar_create_default_input ()
{
    struct t_gui_bar *ptr_bar;
    int length;
    char *buf;
    
    /* search an input_text item */
    if (!gui_bar_item_used_in_a_bar (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT], 1))
    {
        ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_INPUT);
        if (ptr_bar)
        {
            /* add item "input_text" to input bar */
            length = 1;
            if (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]))
                length += strlen (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
            length += 1; /* "," */
            length += strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "%s,%s",
                          (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS])) ?
                          CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]) : "",
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
                config_file_option_set (ptr_bar->options[GUI_BAR_OPTION_ITEMS], buf, 1);
                gui_chat_printf (NULL, _("Bar \"%s\" updated"),
                                 GUI_BAR_DEFAULT_NAME_INPUT);
                gui_bar_draw (ptr_bar);
                free (buf);
            }
        }
        else
        {
            /* create input bar */
            length = 1 /* "[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT])
                + 3 /* "]+(" */
                + 4 /* "away" */
                + 3 /* "),[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH])
                + 3 /* "],[" */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE])
                + 2 /* "]," */
                + strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT])
                + 1 /* \0 */;
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "[%s]+(away),[%s],[%s],%s",
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_PROMPT],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_SEARCH],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_PASTE],
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
                if (gui_bar_new (GUI_BAR_DEFAULT_NAME_INPUT,
                                 "0",          /* hidden */
                                 "1000",       /* priority */
                                 "window",     /* type */
                                 "",           /* conditions */
                                 "bottom",     /* position */
                                 "horizontal", /* filling_top_bottom */
                                 "vertical",   /* filling_left_right */
                                 "1",          /* size */
                                 "0",          /* size_max */
                                 "default",    /* color fg */
                                 "cyan",       /* color delim */
                                 "default",    /* color bg */
                                 "0",          /* separators */
                                 buf))         /* items */
                {
                    gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                     GUI_BAR_DEFAULT_NAME_INPUT);
                }
                free (buf);
            }
        }
    }
}

/*
 * gui_bar_create_default_title: create default title bar if it does not exist
 */

void
gui_bar_create_default_title ()
{
    struct t_gui_bar *ptr_bar;
    
    /* search title bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_TITLE);
    if (!ptr_bar)
    {
        /* create title bar */
        if (gui_bar_new (GUI_BAR_DEFAULT_NAME_TITLE,
                         "0",          /* hidden */
                         "500",        /* priority */
                         "window",     /* type */
                         "",           /* conditions */
                         "top",        /* position */
                         "horizontal", /* filling_top_bottom */
                         "vertical"  , /* filling_left_right */
                         "1",          /* size */
                         "0",          /* size_max */
                         "default",    /* color fg */
                         "cyan",       /* color delim */
                         "blue",       /* color bg */
                         "0",          /* separators */
                         gui_bar_item_names[GUI_BAR_ITEM_BUFFER_TITLE])) /* items */
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             GUI_BAR_DEFAULT_NAME_TITLE);
        }
    }
}

/*
 * gui_bar_create_default_status: create default status bar if it does not exist
 */

void
gui_bar_create_default_status ()
{
    struct t_gui_bar *ptr_bar;
    int length;
    char *buf;
    
    /* search status bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_STATUS);
    if (!ptr_bar)
    {
        /* create status bar */
        length = strlen (gui_bar_item_names[GUI_BAR_ITEM_TIME])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER])
            + 1 /* ":" */
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER])
            + 3 /* "lag" */
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_HOTLIST])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_COMPLETION])
            + strlen (gui_bar_item_names[GUI_BAR_ITEM_SCROLL])
            + (12 * 4) /* all items delimiters: ",:+[]()" */
            + 1;
        buf = malloc (length);
        if (buf)
        {
            snprintf (buf, length, "[%s],[%s],[%s],%s+:+%s+{%s}+%s,[lag],[%s],%s,%s",
                      gui_bar_item_names[GUI_BAR_ITEM_TIME],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_COUNT],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_PLUGIN],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NUMBER],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NAME],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT],
                      gui_bar_item_names[GUI_BAR_ITEM_BUFFER_FILTER],
                      gui_bar_item_names[GUI_BAR_ITEM_HOTLIST],
                      gui_bar_item_names[GUI_BAR_ITEM_COMPLETION],
                      gui_bar_item_names[GUI_BAR_ITEM_SCROLL]);
            if (gui_bar_new (GUI_BAR_DEFAULT_NAME_STATUS,
                             "0",          /* hidden */
                             "500",        /* priority */
                             "window",     /* type */
                             "",           /* conditions */
                             "bottom",     /* position */
                             "horizontal", /* filling_top_bottom */
                             "vertical",   /* filling_left_right */
                             "1",          /* size */
                             "0",          /* size_max */
                             "default",    /* color fg */
                             "cyan",       /* color delim */
                             "blue",       /* color bg */
                             "0",          /* separators */
                             buf))         /* items */
            {
                gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                 GUI_BAR_DEFAULT_NAME_STATUS);
            }
            free (buf);
        }
    }
}

/*
 * gui_bar_create_default_nicklist: create default nicklist bar if it does not exist
 */

void
gui_bar_create_default_nicklist ()
{
    struct t_gui_bar *ptr_bar;
    
    /* search nicklist bar */
    ptr_bar = gui_bar_search (GUI_BAR_DEFAULT_NAME_NICKLIST);
    if (!ptr_bar)
    {
        /* create nicklist bar */
        if (gui_bar_new (GUI_BAR_DEFAULT_NAME_NICKLIST,
                         "0",                /* hidden */
                         "200",              /* priority */
                         "window",           /* type */
                         "nicklist",         /* conditions */
                         "right",            /* position */
                         "columns_vertical", /* filling_top_bottom */
                         "vertical",         /* filling_left_right */
                         "0",                /* size */
                         "0",                /* size_max */
                         "default",          /* color fg */
                         "cyan",             /* color delim */
                         "default",          /* color bg */
                         "1",                /* separators */
                         gui_bar_item_names[GUI_BAR_ITEM_BUFFER_NICKLIST])) /* items */
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             GUI_BAR_DEFAULT_NAME_NICKLIST);
        }
    }
}

/*
 * gui_bar_create_default: create default bars if they do not exist
 */

void
gui_bar_create_default ()
{
    gui_bar_create_default_input ();
    gui_bar_create_default_title ();
    gui_bar_create_default_status ();
    gui_bar_create_default_nicklist ();
}

/*
 * gui_bar_update: update a bar on screen
 */

void
gui_bar_update (const char *name)
{
    struct t_gui_bar *ptr_bar;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
            && (strcmp (ptr_bar->name, name) == 0))
        {
            gui_bar_ask_refresh (ptr_bar);
        }
    }
}

/*
 * gui_bar_scroll: scroll a bar for a buffer
 *                 return 1 if scroll is ok, 0 if error
 */

int
gui_bar_scroll (struct t_gui_bar *bar, struct t_gui_buffer *buffer,
                const char *scroll)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;
    long number;
    char *str, *error;
    int length, add_x, add, percent, scroll_beginning, scroll_end;
    
    if (!bar)
        return 0;
    
    if (CONFIG_BOOLEAN(bar->options[GUI_BAR_OPTION_HIDDEN]))
        return 1;
    
    add_x = 0;
    str = NULL;
    number = 0;
    add = 0;
    percent = 0;
    scroll_beginning = 0;
    scroll_end = 0;
    
    if ((scroll[0] == 'x') || (scroll[0] == 'X'))
    {
        add_x = 1;
        scroll++;
    }
    else if ((scroll[0] == 'y') || (scroll[0] == 'Y'))
    {
        scroll++;
    }
    else
        return 0;
    
    if ((scroll[0] == 'b') || (scroll[0] == 'B'))
    {
        scroll_beginning = 1;
    }
    else if ((scroll[0] == 'e') || (scroll[0] == 'E'))
    {
        scroll_end = 1;
    }
    else
    {
        if (scroll[0] == '+')
        {
            add = 1;
            scroll++;
        }
        else if (scroll[0] == '-')
        {
            scroll++;
        }
        else
            return 0;
        
        length = strlen (scroll);
        if (length == 0)
            return 0;
    
        if (scroll[length - 1] == '%')
        {
            str = string_strndup (scroll, length - 1);
            percent = 1;
        }
        else
            str = strdup (scroll);
        if (!str)
            return 0;
        
        error = NULL;
        number = strtol (str, &error, 10);
        
        if (!error || error[0] || (number <= 0))
        {
            free (str);
            return 0;
        }
    }
    
    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        gui_bar_window_scroll (bar->bar_window, NULL,
                               add_x, scroll_beginning, scroll_end,
                               add, percent, number);
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (ptr_win->buffer == buffer)
            {
                for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                     ptr_bar_win = ptr_bar_win->next_bar_window)
                {
                    if (ptr_bar_win->bar == bar)
                    {
                        gui_bar_window_scroll (ptr_bar_win, ptr_win,
                                               add_x, scroll_beginning, scroll_end,
                                               add, percent, number);
                    }
                }
            }
        }
    }
    
    free (str);
    
    return 1;
}

/*
 * gui_bar_free: delete a bar
 */

void
gui_bar_free (struct t_gui_bar *bar)
{
    int i;
    
    if (!bar)
        return;
    
    /* remove bar window(s) */
    if (bar->bar_window)
    {
        gui_bar_window_free (bar->bar_window, NULL);
        gui_window_ask_refresh (1);
    }
    else
        gui_bar_free_bar_windows (bar);
    
    /* remove bar from bars list */
    if (bar->prev_bar)
        (bar->prev_bar)->next_bar = bar->next_bar;
    if (bar->next_bar)
        (bar->next_bar)->prev_bar = bar->prev_bar;
    if (gui_bars == bar)
        gui_bars = bar->next_bar;
    if (last_gui_bar == bar)
        last_gui_bar = bar->prev_bar;
    
    /* free data */
    if (bar->name)
        free (bar->name);
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        if (bar->options[i])
            config_file_option_free (bar->options[i]);
    }
    if (bar->conditions_array)
        string_free_split (bar->conditions_array);
    gui_bar_free_items_array (bar);
    
    free (bar);
}

/*
 * gui_bar_free_all: delete all bars
 */

void
gui_bar_free_all ()
{
    while (gui_bars)
    {
        gui_bar_free (gui_bars);
    }
}

/*
 * gui_bar_free_bar_windows: free bar windows for a bar
 */

void
gui_bar_free_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_bar_win = ptr_win->bar_windows;
        while (ptr_bar_win)
        {
            next_bar_win = ptr_bar_win->next_bar_window;
            
            if (ptr_bar_win->bar == bar)
                gui_bar_window_free (ptr_bar_win, ptr_win);
            
            ptr_bar_win = next_bar_win;
        }
    }
}

/*
 * gui_bar_add_to_infolist: add a bar in an infolist
 *                          return 1 if ok, 0 if error
 */

int
gui_bar_add_to_infolist (struct t_infolist *infolist,
                         struct t_gui_bar *bar)
{
    struct t_infolist_item *ptr_item;
    int i, j;
    char option_name[64];
    
    if (!infolist || !bar)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "name", bar->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "hidden", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_HIDDEN])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "priority", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_PRIORITY])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "conditions", CONFIG_STRING(bar->options[GUI_BAR_OPTION_CONDITIONS])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "conditions_count", bar->conditions_count))
        return 0;
    for (i = 0; i < bar->conditions_count; i++)
    {
        snprintf (option_name, sizeof (option_name),
                  "conditions_array_%05d", i + 1);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      bar->conditions_array[i]))
            return 0;
    }
    if (!infolist_new_var_integer (ptr_item, "position", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_top_bottom", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_left_right", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size_max", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE_MAX])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_fg", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_delim", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_bg", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_BG]))))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "separator", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SEPARATOR])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "items", CONFIG_STRING(bar->options[GUI_BAR_OPTION_ITEMS])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "items_count", bar->items_count))
        return 0;
    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            snprintf (option_name, sizeof (option_name),
                      "items_array_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_array[i][j]))
                return 0;
        }
    }
    if (!infolist_new_var_pointer (ptr_item, "bar_window", bar->bar_window))
        return 0;
    
    return 1;
}

/*
 * gui_bar_print_log: print bar infos in log (usually for crash dump)
 */

void
gui_bar_print_log ()
{
    struct t_gui_bar *ptr_bar;
    int i, j;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        log_printf ("");
        log_printf ("[bar (addr:0x%lx)]", ptr_bar);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_bar->name);
        log_printf ("  hidden . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]));
        log_printf ("  priority . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_PRIORITY]));
        log_printf ("  type . . . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]),
                    gui_bar_type_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE])]);
        log_printf ("  conditions . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS]));
        log_printf ("  conditions_count . . . : %d",    ptr_bar->conditions_count);
        log_printf ("  conditions_array . . . : 0x%lx", ptr_bar->conditions_array);
        log_printf ("  position . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]),
                    gui_bar_position_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION])]);
        log_printf ("  filling_top_bottom . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM]),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM])]);
        log_printf ("  filling_left_right . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT]),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT])]);
        log_printf ("  size . . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]));
        log_printf ("  size_max . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]));
        log_printf ("  color_fg . . . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_FG]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_FG])));
        log_printf ("  color_delim. . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_DELIM]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_DELIM])));
        log_printf ("  color_bg . . . . . . . : %d",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG])));
        log_printf ("  separator. . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SEPARATOR]));
        log_printf ("  items. . . . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
        log_printf ("  items_count. . . . . . : %d",    ptr_bar->items_count);
        for (i = 0; i < ptr_bar->items_count; i++)
        {
            log_printf ("    items_subcount[%03d]. : %d",
                        i, ptr_bar->items_subcount[i]);
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                log_printf ("    items_array[%03d][%03d]: '%s'",
                            i, j, ptr_bar->items_array[i][j]);
            }
        }
        log_printf ("  bar_window . . . . . . : 0x%lx", ptr_bar->bar_window);
        log_printf ("  prev_bar . . . . . . . : 0x%lx", ptr_bar->prev_bar);
        log_printf ("  next_bar . . . . . . . : 0x%lx", ptr_bar->next_bar);
        
        if (ptr_bar->bar_window)
            gui_bar_window_print_log (ptr_bar->bar_window);
    }
}
