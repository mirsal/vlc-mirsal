/*****************************************************************************
 * dbus-root.c : dbus control module (mpris v1.0) - root object
 *****************************************************************************
 * Copyright © 2006-2008 Rafaël Carré
 * Copyright © 2007-2010 Mirsal Ennaime
 * Copyright © 2009-2010 The VideoLAN team
 * $Id$
 *
 * Authors:    Mirsal Ennaime <mirsal at mirsal fr>
 *             Rafaël Carré <funman at videolanorg>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_interface.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include <unistd.h>
#include <limits.h>

#include "dbus_root.h"
#include "dbus_common.h"

DBUS_METHOD( Identity )
{
    VLC_UNUSED(p_this);
    REPLY_INIT;
    OUT_ARGUMENTS;

    char *psz_identity = VLC_IDENTITY;

    DBusMessageIter v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    dbus_message_iter_append_basic( &v, DBUS_TYPE_STRING, &psz_identity );

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( CanQuit )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    const dbus_bool_t b_ret = TRUE;

    DBusMessageIter v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    dbus_message_iter_append_basic( &v, DBUS_TYPE_BOOLEAN, &b_ret );

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( CanRaise )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    const dbus_bool_t b_ret = FALSE;

    DBusMessageIter v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    dbus_message_iter_append_basic( &v, DBUS_TYPE_BOOLEAN, &b_ret );

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( HasTrackList )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    const dbus_bool_t b_ret = FALSE;

    DBusMessageIter v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    dbus_message_iter_append_basic( &v, DBUS_TYPE_BOOLEAN, &b_ret );

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( DesktopEntry )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    const char* psz_ret = PACKAGE;

    DBusMessageIter v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    dbus_message_iter_append_basic( &v, DBUS_TYPE_STRING, &psz_ret );

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( SupportedMimeTypes )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    DBusMessageIter ret, v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    size_t i_len = sizeof( ppsz_supported_mime_types ) / sizeof( char* );

    if( !dbus_message_iter_open_container( &v, DBUS_TYPE_ARRAY, "s", &ret ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    for( size_t i = 0; i < i_len; ++i )
        if( !dbus_message_iter_append_basic( &ret, DBUS_TYPE_STRING,
                                             &ppsz_supported_mime_types[i] ) )
            return DBUS_HANDLER_RESULT_NEED_MEMORY;

    if( !dbus_message_iter_close_container( &v, &ret ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( SupportedUriSchemes )
{
    VLC_UNUSED( p_this );
    REPLY_INIT;
    OUT_ARGUMENTS;

    DBusMessageIter ret, v;
    dbus_message_iter_open_container( &args, DBUS_TYPE_VARIANT, "s", &v );
    size_t i_len = sizeof( ppsz_supported_uri_schemes ) / sizeof( char* );

    if( !dbus_message_iter_open_container( &v, DBUS_TYPE_ARRAY, "s", &ret ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    for( size_t i = 0; i < i_len; ++i )
        if( !dbus_message_iter_append_basic( &ret, DBUS_TYPE_STRING,
                                             &ppsz_supported_uri_schemes[i] ) )
            return DBUS_HANDLER_RESULT_NEED_MEMORY;

    if( !dbus_message_iter_close_container( &v, &ret ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    if( !dbus_message_iter_close_container( &args, &v ) )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    REPLY_SEND;
}

DBUS_METHOD( Quit )
{ /* exits vlc */
    REPLY_INIT;
    libvlc_Quit(INTF->p_libvlc);
    REPLY_SEND;
}

#define WM_WINDOW_ROLE_S "WM_CLASS"
#define _NET_WM_PID_S "_NET_WM_PID"
#define VLC_MAIN_WINDOW_ROLE "vlc"

static int find_vlc_main_window( intf_thread_t    *p_this,
                                 xcb_connection_t *p_x_conn,
                                 xcb_window_t      parent,
                                 xcb_window_t     *vlc_main_window )
{
    xcb_window_t            *p_children  = NULL;
    xcb_generic_error_t     *p_x_error   = NULL;
    xcb_query_tree_reply_t  *p_tree      = NULL;
    size_t                   i_tree_len  = 0;

    xcb_intern_atom_reply_t *p_intern_atom = NULL;
    xcb_atom_t               wm_window_role;

    xcb_get_property_cookie_t *p_cookies = NULL;

    p_intern_atom = xcb_intern_atom_reply ( p_x_conn,
        xcb_intern_atom( p_x_conn, 0,
                         strlen( WM_WINDOW_ROLE_S ),
                         WM_WINDOW_ROLE_S ), NULL );

    if( !p_intern_atom )
        return VLC_ENOMEM;

    wm_window_role = p_intern_atom->atom;
    free( p_intern_atom );

    p_tree = xcb_query_tree_reply( p_x_conn,
                                   xcb_query_tree( p_x_conn, parent ),
                                   &p_x_error );

    if( !p_tree )
        return VLC_ENOMEM;

    i_tree_len = xcb_query_tree_children_length( p_tree );
    p_children = xcb_query_tree_children( p_tree );
    p_cookies  = calloc( i_tree_len + 1, sizeof( xcb_get_property_cookie_t ) );

    msg_Dbg( (vlc_object_t*) p_this, "This window has %d children", i_tree_len );

    for( int i = 1; (size_t) i <= i_tree_len; i++ )
    {
        xcb_window_t vlc_window;
        if( VLC_SUCCESS == find_vlc_main_window( p_this,
                                                 p_x_conn,
                                                 p_children[i],
                                                 &vlc_window ) )
        {
            *vlc_main_window = vlc_window;
            return VLC_SUCCESS;
        }

        p_cookies[i] = xcb_get_text_property( p_x_conn,
                                              p_children[i],
                                              wm_window_role );
    }

    p_cookies[0] = xcb_get_text_property( p_x_conn,
                                          parent,
                                          wm_window_role );

    for( int i = 0; (size_t) i <= i_tree_len; i++ )
    {
        int i_len = 0;
        char* psz_wm_window_role = NULL;
        xcb_get_property_reply_t *p_reply = NULL;

        p_reply = xcb_get_property_reply( p_x_conn, p_cookies[i], &p_x_error );

        if( !p_reply ||
             0 == ( i_len = xcb_get_property_value_length( p_reply ) ) )
        {
            if( !p_reply )
                msg_Err( (vlc_object_t*) p_this,
                         "No reply: iteration: %d Error code %d; %s:%d",
                         i, p_x_error->error_code, __FILE__, __LINE__ );
            else
                msg_Err( (vlc_object_t*) p_this,
                         "Empty X reply: iteration: %d; %s:%d",
                         i, __FILE__, __LINE__ );

            free( p_reply );
            free( p_x_error );
            p_reply = NULL;
            continue;
        }

        psz_wm_window_role = calloc( i_len + 1, sizeof( char ) );
        strncpy( psz_wm_window_role, xcb_get_property_value( p_reply ), i_len );
        free( p_reply );

        msg_Dbg( (vlc_object_t*) p_this, "WM_WINDOW_ROLE = %s",
                                         psz_wm_window_role );
        if( psz_wm_window_role &&
            !strncmp( psz_wm_window_role, VLC_MAIN_WINDOW_ROLE, i_len ) )
        {
            free( psz_wm_window_role );
            *vlc_main_window = p_children[i];
            return VLC_SUCCESS;
        }

        free( psz_wm_window_role );
    }

    return VLC_EGENERIC;
}

DBUS_METHOD( Raise )
{
/*  TODO: Make this piece of code more concise.
    TODO: Use ifdefs to include this code only when VLC is built with X support.
    TODO: Use _NET_ACTIVE_WINDOW so the window is raised even when it is
          withdrawn, minimized and/or not on the current viewport.
*/

    REPLY_INIT;

    int i_x_screen = -1, i_tree_len = 0;

    xcb_connection_t        *p_x_conn   = xcb_connect( NULL, &i_x_screen );
    const xcb_setup_t       *p_x_setup  = xcb_get_setup( p_x_conn );
    xcb_screen_iterator_t    roots      = xcb_setup_roots_iterator( p_x_setup );

    xcb_screen_t              *p_x_screen    = NULL;
    xcb_intern_atom_reply_t   *p_intern_atom = NULL;
    xcb_atom_t                 _net_wm_pid;

    xcb_get_property_cookie_t *p_cookies     = NULL;
    xcb_window_t              *p_windows     = NULL;

    xcb_window_t               vlc_main_window;
    int i_search_ret = -1;

    msg_Dbg( (vlc_object_t*) p_this, "Looking for a VLC window on X screen %d",
                                     i_x_screen );

    for( int i = i_x_screen ; roots.rem ; --i, xcb_screen_next( &roots ) )
    {
        if( 0 == i )
        {
            p_x_screen = roots.data;
            break;
        }
    }

    p_intern_atom = xcb_intern_atom_reply ( p_x_conn,
        xcb_intern_atom( p_x_conn, 0,
                         strlen( _NET_WM_PID_S ),
                         _NET_WM_PID_S ), NULL );
    if( !p_intern_atom )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    _net_wm_pid = p_intern_atom->atom;
    free( p_intern_atom );

    xcb_query_tree_reply_t *p_tree;
    p_tree = xcb_query_tree_reply( p_x_conn,
                                   xcb_query_tree( p_x_conn, p_x_screen->root ),
                                   NULL );

    if( !p_tree )
        return DBUS_HANDLER_RESULT_NEED_MEMORY;

    i_tree_len = xcb_query_tree_children_length( p_tree );
    p_windows  = xcb_query_tree_children( p_tree );
    p_cookies  = calloc( i_tree_len, sizeof( xcb_get_property_cookie_t ) );

    for( int i = 0; i < i_tree_len; i++ )
    {
        p_cookies[i] = xcb_get_any_property( p_x_conn,
                                             0,
                                             p_windows[i],
                                             _net_wm_pid,
                                             INT_MAX );
    }

    for( int i = 0; i < i_tree_len; i++ )
    {
        int *pi_wm_pid;
        size_t i_len = 0;

        xcb_get_property_reply_t *p_reply;
        xcb_generic_error_t      *p_x_error;

        p_reply = xcb_get_property_reply( p_x_conn, p_cookies[i], &p_x_error );

        if( !p_reply ||
             0 == ( i_len = xcb_get_property_value_length( p_reply ) ) )
        {
            if( !p_reply )
            {
                msg_Err( (vlc_object_t*) p_this,
                         "No reply: iteration: %d Error code %d",
                         i, p_x_error->error_code );

                free( p_x_error );
            }

            free( p_reply );
            p_reply = NULL;
            continue;
        }

        pi_wm_pid = (int*) xcb_get_property_value( p_reply );
        free( p_reply );

        if( getpid() != *pi_wm_pid )
            continue;

        msg_Dbg( (vlc_object_t*) p_this,
                 "Found a window which belongs to this vlc instance. "
                 "Recursively trying to find the main VLC window" );

        if( VLC_SUCCESS ==
          ( i_search_ret = find_vlc_main_window( p_this,
                                                 p_x_conn,
                                                 p_windows[i],
                                                 &vlc_main_window ) ) )
            break;
    }

    if( VLC_SUCCESS == i_search_ret )
    {
        msg_Dbg( (vlc_object_t*) p_this, "VICTORY !!!!!" );

        /* XXX */
        const uint32_t p_stack_mode[] = { XCB_STACK_MODE_ABOVE };
        xcb_map_window( p_x_conn, vlc_main_window );
        xcb_configure_window( p_x_conn, vlc_main_window,
                              XCB_CONFIG_WINDOW_STACK_MODE,
                              p_stack_mode );
    }
    else
        msg_Err( (vlc_object_t*) p_this,
                 "Can not find a main VLC window, giving up :(" );

    free( p_tree );
    free( p_cookies );
    REPLY_SEND;
}

#undef _NET_WM_PID_S
#undef WM_WINDOW_ROLE_S
#undef VLC_MAIN_WINDOW_ROLE

#define PROPERTY_MAPPING_BEGIN if( 0 ) {}
#define PROPERTY_FUNC( interface, property, function ) \
    else if( !strcmp( psz_interface_name, interface ) && \
             !strcmp( psz_property_name,  property ) ) \
        return function( p_conn, p_from, p_this );
#define PROPERTY_MAPPING_END return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

DBUS_METHOD( GetProperty )
{
    DBusError error;

    char *psz_interface_name = NULL;
    char *psz_property_name  = NULL;

    dbus_error_init( &error );
    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_STRING, &psz_interface_name,
            DBUS_TYPE_STRING, &psz_property_name,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                                         error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    msg_Dbg( (vlc_object_t*) p_this, "Getting property %s",
                                     psz_property_name );

    PROPERTY_MAPPING_BEGIN
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "Identity",            Identity )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "DesktopEntry",        DesktopEntry )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "SupportedMimeTypes",  SupportedMimeTypes )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "SupportedUriSchemes", SupportedUriSchemes )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "HasTrackList",        HasTrackList )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "CanQuit",             CanQuit )
    PROPERTY_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "CanRaise",            CanRaise )
    PROPERTY_MAPPING_END
}

#undef PROPERTY_MAPPING_BEGIN
#undef PROPERTY_GET_FUNC
#undef PROPERTY_MAPPING_END

#define METHOD_MAPPING_BEGIN if( 0 ) {}
#define METHOD_FUNC( interface, method, function ) \
    else if( dbus_message_is_method_call( p_from, interface, method ) )\
        return function( p_conn, p_from, p_this )
#define METHOD_MAPPING_END return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

DBusHandlerResult
handle_root ( DBusConnection *p_conn, DBusMessage *p_from, void *p_this )
{
    METHOD_MAPPING_BEGIN
    METHOD_FUNC( DBUS_INTERFACE_PROPERTIES, "Get",          GetProperty );
    METHOD_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "Raise",        Raise );
    METHOD_FUNC( DBUS_MPRIS_ROOT_INTERFACE, "Quit",         Quit );
    METHOD_MAPPING_END
}

#undef METHOD_MAPPING_BEGIN
#undef METHOD_FUNC
#undef METHOD_MAPPING_END
