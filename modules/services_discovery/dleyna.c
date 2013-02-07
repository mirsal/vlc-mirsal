/*****************************************************************************
 * dleyna.c: DLNA services discovery module using dLeyna
 *****************************************************************************
 * Copyright (C) 2013 the VideoLAN team
 * $Id$
 *
 * Authors: Mirsal Ennaime <mirsal@mirsal.fr>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_services_discovery.h>

#include <dbus/dbus.h>

#define DLEYNA_SERVICE_NAME "com.intel.media-service-upnp"
#define DLEYNA_SERVICE_PATH "/com/intel/MediaServiceUPnP"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

/* Callbacks */
    static int  Open ( vlc_object_t * );
    static void Close( vlc_object_t * );

VLC_SD_PROBE_HELPER("dleyna", "DLNA devices (dLeyna)", SD_CAT_LAN)

vlc_module_begin ()
    set_shortname( "dLeyna" )
    set_description( N_("DLNA / UPnP with dLeyna") )
    set_category( CAT_PLAYLIST )
    set_subcategory( SUBCAT_PLAYLIST_SD )
    set_capability( "services_discovery", 0 )
    set_callbacks( Open, Close )

    VLC_SD_PROBE_SUBMODULE
vlc_module_end ()

/*****************************************************************************
 * Local structures
 *****************************************************************************/

struct services_discovery_sys_t
{
    DBusConnection *p_conn;
};

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

/*****************************************************************************
 * Open: initialize and create stuff
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t* )p_this;
    services_discovery_sys_t *p_sys;

    p_sd->p_sys = p_sys = calloc( 1, sizeof( services_discovery_sys_t ) );

    if( unlikely(!p_sys) )
        return VLC_ENOMEM;

    DBusError err;
    dbus_error_init(&err);

    p_sys->p_conn = dbus_bus_get_private( DBUS_BUS_SESSION, &err );

    if( !p_sys->p_conn )
    {
        msg_Err( p_sd, "cannot connect to session bus: %s", err.message);
        dbus_error_free( &err );
        free( p_sys );
        return VLC_EGENERIC;
    }

    if( !dbus_bus_name_has_owner( p_sys->p_conn, DLEYNA_SERVICE_NAME, &err ) )
    {
        msg_Err( p_sd, "Cannot find dLeyna" );

        if( dbus_error_is_set( &err ) )
        {
            msg_Err( p_sd, "D-Bus error: %s", err.message );
            dbus_error_free( &err );
        }

        goto error;
    }

    return VLC_SUCCESS;

error:
    dbus_connection_close( p_sd->p_sys->p_conn );
    dbus_connection_unref( p_sd->p_sys->p_conn );
    free( p_sys );

    return VLC_EGENERIC;
}

/*****************************************************************************
 * Close: cleanup
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = (services_discovery_t*) p_this;

    dbus_connection_close( p_sd->p_sys->p_conn );
    dbus_connection_unref( p_sd->p_sys->p_conn );

    free( p_sd->p_sys );
}
