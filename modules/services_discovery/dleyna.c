/*****************************************************************************
 * dleyna.c: DLNA services discovery module using dLeyna
 *****************************************************************************
 * Copyright (C) 2013 VLC authors and VideoLAN
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
#include <vlc_threads.h>
#include <vlc_services_discovery.h>

#include <assert.h>
#include <dbus/dbus.h>

#define DLEYNA_SERVICE_NAME "com.intel.media-service-upnp"
#define DLEYNA_SERVICE_PATH "/com/intel/MediaServiceUPnP"
#define DLEYNA_MANAGER_INTERFACE "com.intel.MediaServiceUPnP.Manager"

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
    vlc_thread_t probe_thread;
};

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

static void  *Probe( void* );
static int    SubscribeToMediaServer( services_discovery_t *p_sd,
                                      const char *psz_server );

/*****************************************************************************
 * Open: initialize and create stuff
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t* )p_this;
    services_discovery_sys_t *p_sys;

    if( !dbus_threads_init_default() )
        return VLC_EGENERIC;

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

    if( !vlc_clone( &p_sys->probe_thread, Probe, p_sd, VLC_THREAD_PRIORITY_LOW ) )
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

    vlc_cancel( p_sd->p_sys->probe_thread );
    vlc_join( p_sd->p_sys->probe_thread, NULL );

    dbus_connection_close( p_sd->p_sys->p_conn );
    dbus_connection_unref( p_sd->p_sys->p_conn );

    free( p_sd->p_sys );
}

/**
 * Finds media servers
 *
 * This is supposed to be used as a probe thread function.
 *
 * @param void* p_data This services_discovery_t object
 * @return NULL
 */
static void *Probe( void *p_data )
{
    services_discovery_t *p_sd = (services_discovery_t*)p_data;

    DBusMessage *p_call       = NULL, *p_reply = NULL;
    const char **ppsz_servers = NULL;
    int i_servers = 0;

    DBusError err;
    dbus_error_init(&err);

    msg_Dbg( p_sd, "Probing dLeyna for available DLNA media servers");

    p_call = dbus_message_new_method_call( DLEYNA_SERVICE_NAME,
            DLEYNA_SERVICE_PATH, DLEYNA_MANAGER_INTERFACE, "GetServers" );

    if( !p_call )
        return NULL;

    p_reply = dbus_connection_send_with_reply_and_block( p_sd->p_sys->p_conn,
            p_call, DBUS_TIMEOUT_USE_DEFAULT, &err );

    dbus_message_unref( p_call );

    if( !p_reply )
    {
        if( dbus_error_is_set( &err ) )
        {
            msg_Dbg( p_sd, "DBus error: %s", err.message );
            dbus_error_free( &err );
        }

        msg_Dbg( p_sd, "Failed to retrieve a list of media servers" );
        return NULL;
    }

    if( !dbus_message_get_args( p_reply, &err,
                DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH,
                &ppsz_servers, &i_servers,
                DBUS_TYPE_INVALID) )
    {
        msg_Dbg( p_sd, "DBus error: %s", err.message );
        dbus_message_unref( p_reply );
        dbus_error_free( &err );
        return NULL;
    }

    msg_Dbg( p_sd, "Found %d DLNA media servers", i_servers );

    for( int i = 0; i < i_servers; ++i )
    {
        const char *psz_server = ppsz_servers[i];

        if( VLC_ENOMEM == SubscribeToMediaServer( p_sd, psz_server ) )
            return NULL;
    }

    dbus_message_unref( p_reply );
    return NULL;
}

/**
 * Subscribes to a media server given its DBus object path
 *
 * @param services_discovery_t* p_sd This SD instance
 * @param const char* psz_server A media server's DBus object path
 *
 * @return int VLC error code
 */
static int SubscribeToMediaServer( services_discovery_t *p_sd,
                                   const char *psz_server )
{
    DBusError err;
    dbus_error_init( &err );

    assert( psz_server );
    msg_Dbg( p_sd, "Subscribing to media server at %s", psz_server );

    return VLC_SUCCESS;
}
