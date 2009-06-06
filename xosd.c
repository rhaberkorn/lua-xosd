/* 
 * Copyright (c) 2009 Scott Vokes <scott@silentbicycle.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <lua.h>
#include <lauxlib.h>
#include "lxosd.h"


/* Get an int field from the table on top of the stack, with default. */
static int get_optint_field(lua_State *L, const char* key, int def) {
        int res;
        lua_getfield(L, -1, key);
        res = luaL_optint(L, -1, def);
        /* printf("%s -> %d (default %d)\n", key, res, def); */
        lua_pop(L, 1);
        return res;
}


/* Get a string field from the table on top of the stack, with default. */
static const char*
get_optstring_field(lua_State *L, const char* key, const char* def) {
        const char* res;
        lua_getfield(L, -1, key);

        if (lua_isstring(L, -1)) {
                res = lua_tostring(L, -1);
                lua_pop(L, 1);
        } else {
                res = def;
                lua_pop(L, 1);
        }
        /* printf("%s -> %s (default %s)\n", key, res, def); */
        return res;
}


/**************
 * Allocation *
 **************/

/* Create a new Lua XOSD object.
 * Optional argument: a table of settings. */
static int lx_new(lua_State *L) {
        int lines;
        LuaXOSD* d;
        xosd* x;

        if (lua_gettop(L) == 0) lua_newtable(L);

        lines = get_line_ct(L);        /* s: table */
        d = init_LuaXOSD(L, lines);    /* s: table LuaXOSD */
        lua_pushvalue(L, -2);          /* s: table LuaXOSD table */
        lua_remove(L, -3);             /* s: LuaXOSD table */

        x = d->disp;
        set_defaults(L, x);

        lua_pop(L, 1);
        
        return 1;               /* leaving LuaXOSD userdata on stack */
}


/* Set the defaults, using config table if provided. */
static int set_defaults(lua_State *L, xosd* x) {
        const char* color;

        xosd_set_horizontal_offset(x, get_optint_field(L,
                "x", LX_DEF_X_OFFSET));
        xosd_set_vertical_offset(x, get_optint_field(L,
                "y", LX_DEF_Y_OFFSET));
        xosd_set_shadow_offset(x, get_optint_field(L,
                "shadow_offset", LX_DEF_SHADOW_OFFSET));
        xosd_set_outline_offset(x, get_optint_field(L,
                "outline_offset", LX_DEF_OUTLINE_OFFSET));
        xosd_set_timeout(x, get_optint_field(L,
                "timeout", LX_DEF_TIMEOUT));
        xosd_set_font(x, get_optstring_field(L,
                "font", LX_DEF_FONT));
        xosd_set_align(x,
            align_of_str(L, get_optstring_field(L,
                    "align", LX_DEF_ALIGN)));

        /* For colors, check table for "colour", then "color". */
        color = get_optstring_field(L, "shadow_colour", "");
        if (strcmp(color, "") == 0)
                color = get_optstring_field(L, "shadow_color",
                    LX_DEF_SHADOW_COLOR);
        xosd_set_shadow_colour(x, color);

        color = get_optstring_field(L, "outline_colour", "");
        if (strcmp(color, "") == 0)
                color = get_optstring_field(L, "outline_color",
                    LX_DEF_OUTLINE_COLOR);
        xosd_set_outline_colour(x, color);

        color = get_optstring_field(L, "colour", "");
        if (strcmp(color, "") == 0)
                color = get_optstring_field(L, "color", LX_DEF_COLOUR);
        xosd_set_colour(x, color);

        return 0;
}


/* Init: Get the line count. */
static int get_line_ct(lua_State *L) {
        int res;

        if (lua_istable(L, -1)) {
                lua_getfield(L, -1, "lines");
                res = luaL_optint(L, -1, 1);
                lua_pop(L, 1);
        } else {
                lua_pushstring(L, "Error getting line count.");
                lua_error(L);
        }
        return res;
}


/* Initialize a LuaXOSD userdata. */
static LuaXOSD* init_LuaXOSD(lua_State *L, int lines) {
        xosd* osd;              /* pointer, to wrap in userdata. */
        LuaXOSD* d;

        luaL_argcheck(L, lines >= 1, 1, "invalid line count");
        osd = xosd_create(lines);
        if (!osd) exit(EXIT_FAILURE);
        
        d = (LuaXOSD*)lua_newuserdata(L, sizeof(LuaXOSD));
        d->disp = osd;
        
        luaL_getmetatable(L, "LuaXOSD");
        lua_setmetatable(L, -2);
        return d;
}


/* Destroy xosd pointer, called by __gc. */
static int lx_destroy(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        xosd_destroy(osd->disp);
        return 0;
}


/* Get string representation of LuaXOSD. */
static int lx_tostring(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        lua_pushfstring(L, "xosd(%d, %s)",
            xosd_get_number_lines(osd->disp),
            xosd_is_onscreen(osd->disp) ? "onscreen" : "hidden");
        return 1;
}


/*********
 * Lines *
 *********/

/* Scroll. */
static int lx_scroll(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int lines = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_scroll(osd->disp, lines) == 0);
        return 1;
}


/* Get the number of lines. */
static int lx_get_line_ct(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);

        lua_pushinteger(L, xosd_get_number_lines(osd->disp));
        return 1;
}


/************
 * Settings *
 ************/

/* Set the timeout. */
static int lx_set_timeout(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int t = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_set_timeout(osd->disp, t) == 0);
        return 1;
}


/* Set the colo(u)r. */
static int lx_set_colour(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *col = (const char*) luaL_checkstring(L, 2);
        
        lua_pushboolean(L, xosd_set_colour(osd->disp, col) == 0);
        return 1;
}


/* Set the outline colo(u)r. (undocumented) */
static int lx_set_outline_colour(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *col = (const char*) luaL_checkstring(L, 2);

        lua_pushboolean(L, xosd_set_outline_colour(osd->disp, col) == 0);
        return 1;
}


/* Set the shadow colo(u)r. (undocumented) */
static int lx_set_shadow_colour(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *col = (const char*) luaL_checkstring(L, 2);

        lua_pushboolean(L, xosd_set_shadow_colour(osd->disp, col) == 0);
        return 1;
}


/* Set the font. (What does it do with nonexistant fonts?) */
static int lx_set_font(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *font = (const char*) luaL_checkstring(L, 2);

        lua_pushboolean(L, xosd_set_font(osd->disp, font) == 0);
        return 1;
}


/* Set the shadow offset. */
static int lx_set_shadow_offset(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int o = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_set_shadow_offset(osd->disp, o) == 0);
        return 1;
}


/* Set the outline offset. */
static int lx_set_outline_offset(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int o = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_set_outline_offset(osd->disp, o) == 0);
        return 1;
}


/* Convert a str key (L, C, R, case insensitive) to an align. */
static xosd_align align_of_str(lua_State *L, const char *key) {
        switch (*key) {
        case 'l':
        case 'L': return XOSD_left;
        case 'c':
        case 'C': return XOSD_center;
        case 'r':
        case 'R': return XOSD_right;;
        default:
                lua_pushstring(L, "Invalid align key, must be L/C/R.");
                lua_error(L);
        }

        /* Won't get here... */
        return XOSD_left;
}


/* Set the alignment. Uses 'L', 'C', 'R'. */
static int lx_set_align(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *key = luaL_checkstring(L, 2); /* L, C, R */
        xosd_align align = align_of_str(L, key);

        lua_pushboolean(L, xosd_set_align(osd->disp, align) == 0);
        return 1;
}


/* Set the vertical offset. */
static int lx_set_vertical_offset(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int offset = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_set_vertical_offset(osd->disp,
                offset) == 0);
        return 1;
}


/* Set the horizontal offset. */
static int lx_set_horizontal_offset(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int offset = luaL_checkint(L, 2);

        lua_pushboolean(L, xosd_set_horizontal_offset(osd->disp,
                offset) == 0);
        return 1;
}


/* Set position, takes "T", "B", or (x, y). */
static int lx_set_pos(lua_State *L) {
        int xo, yo;
        LuaXOSD* osd = check_xosd(L);
        int res;

        if (lua_type(L, -1) == LUA_TSTRING) {
                const char* pos = lua_tostring(L, -1);
                if (pos[0] == 'T') {
                        lua_pushboolean(L, xosd_set_pos(osd->disp,
                                XOSD_top) == 0);
                        return 1;
                } else if (pos[0] == 'B') {
                        lua_pushboolean(L, xosd_set_pos(osd->disp,
                                XOSD_bottom) == 0);
                        return 1;
                } else {
                        lua_pushstring(L, "Position must be T(op), B(ottom), or (x, y).");
                        lua_error(L);
                }
        }

        xo = luaL_checkint(L, 2);
        yo = luaL_checkint(L, 3);
        res = xosd_set_horizontal_offset (osd->disp, xo);
        res = res | xosd_set_vertical_offset (osd->disp, yo);
        lua_pushboolean(L, res == 0);
        return 1;
}


/**************
 * Displaying *
 **************/

/* Display a string. */
static int lx_display_string(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        const char *s = (const char*) luaL_checkstring(L, 2);
        int blocking = lua_toboolean(L, 3);
        int res = xosd_display(osd->disp, 0, XOSD_string, s);

        if (blocking) xosd_wait_until_no_display(osd->disp);
        lua_pushboolean(L, res == 0);
        return 1;
}


/* Common code for displaying a numeric value. */
static int lx_display_numeric(lua_State *L, xosd_command command) {
        LuaXOSD* osd = check_xosd(L);
        int perc = luaL_checkint(L, 2);
        int blocking;
        int res;

        if (perc < 0 || perc > 100) {
                lua_pushstring(L, "Error: Numeric display must be 0 < n < 100.");
                lua_error(L);
        }

        blocking = lua_toboolean(L, 3);

        res = xosd_display(osd->disp, 0, command, perc);
        if (blocking) xosd_wait_until_no_display(osd->disp);
        lua_pushboolean(L, res == 0);
        return 1;
}


/* Display a percentage. */
static int lx_display_percent(lua_State *L) {
        return lx_display_numeric(L, XOSD_percentage);
}


/* Display a slider. */
static int lx_display_slider(lua_State *L) {
        return lx_display_numeric(L, XOSD_slider);
}


/* Block until done displaying. */
static int lx_wait(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        lua_pushboolean(L, xosd_wait_until_no_display(osd->disp) == 0);
        return 1;
}


/* Show a hidden OSD. */
static int lx_show(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        lua_pushboolean(L, xosd_show(osd->disp) == 0);
        return 1;
}


/* Hide the OSD. */
static int lx_hide(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        lua_pushboolean(L, xosd_hide(osd->disp) == 0);
        return 1;
}


/* Is it onscreen? */
static int lx_is_onscreen(lua_State *L) {
        LuaXOSD* osd = check_xosd(L);
        int res = xosd_is_onscreen(osd->disp);
        if (res == -1) {
                lua_pushstring(L, "xosd_is_onscreen: unknown error.");
                lua_error(L);
        }

        lua_pushboolean(L, res == 1);
        return 1;
}


/****************
 * Module setup *
 ****************/

static const struct luaL_Reg xosd_metatable [] = {
        { "print", lx_display_string },
        { "print_percent", lx_display_percent },
        { "print_slider", lx_display_slider },
        { "destroy", lx_destroy },
        { "scroll", lx_scroll },
        { "get_line_ct", lx_get_line_ct },
        { "set_align", lx_set_align },
        { "set_color", lx_set_colour },
        { "set_colour", lx_set_colour },
        { "set_shadow_color", lx_set_shadow_colour },
        { "set_shadow_colour", lx_set_shadow_colour },
        { "set_outline_color", lx_set_outline_colour },
        { "set_outline_colour", lx_set_outline_colour },
        { "set_font", lx_set_font },
        { "set_timeout", lx_set_timeout },
        { "set_horizontal_offset", lx_set_horizontal_offset },
        { "set_vertical_offset", lx_set_vertical_offset },
        { "set_pos", lx_set_pos },
        { "set_shadow_offset", lx_set_shadow_offset },
        { "set_outline_offset", lx_set_outline_offset },
        { "show", lx_show },
        { "hide", lx_hide },
        { "wait", lx_wait },
        { "is_onscreen", lx_is_onscreen },
        { "__gc", lx_destroy },
        { "__tostring", lx_tostring },
        { NULL, NULL },
};        


static const struct luaL_Reg lxosdlib[] = {
        { "new", lx_new },
        { NULL, NULL },
};


/* Register the C library with Lua. */
int luaopen_xosd(lua_State *L) {
        luaL_newmetatable(L, "LuaXOSD");

        /* metatable.__index -> metatable */
        lua_pushvalue(L, -1);   /* dup */
        lua_setfield(L, -2, "__index");
        luaL_register(L, NULL, xosd_metatable);
        
        luaL_register(L, "xosd", lxosdlib);
        return 1;
}
