/*****************************************************************************
 * ayatana.c : Ayatana sound indicator registration
 *****************************************************************************
 * Copyright © 2010 Mirsal Ennaime
 * Copyright © 2010 The VideoLAN team
 * $Id$
 *
 * Authors:    Mirsal Ennaime <mirsal dot ennaime at gmail dot com>
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
#endif //config.h

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

#include <libindicate/server.h>

static int  Activate   ( vlc_object_t * );
static void Deactivate ( vlc_object_t * );

struct intf_sys_t
{
    IndicateServer *p_indicate_server;
};

vlc_module_begin ()
    set_description( N_("Ayatana sound-indicator support") )
    set_capability( "interface", 0 )
    set_callbacks( Activate, Deactivate )
vlc_module_end ()

static int Activate( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t*) p_this;
    intf_sys_t *p_sys = NULL;

    p_sys = p_intf->p_sys = (intf_sys_t *) calloc( 1, sizeof( intf_sys_t ) );

    if( !p_sys )
        return VLC_ENOMEM;

    g_type_init();
    p_sys->p_indicate_server = indicate_server_ref_default();
    indicate_server_set_type( p_sys->p_indicate_server, "music.vlc" );
    indicate_server_show( p_sys->p_indicate_server );

    return VLC_SUCCESS;
}

static void Deactivate( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t*)p_this;
    intf_sys_t *p_sys = p_intf->p_sys;

    g_object_unref ( p_sys->p_indicate_server );
    free ( p_sys );
}
