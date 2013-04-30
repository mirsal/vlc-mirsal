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
#define DLEYNA_DEVICE_INTERFACE "com.intel.UPnP.MediaDevice"

/**
 * Represents a DLNA DMS device.
 * The fields and docstrings mirror the dLeyna server object's properties
 */
typedef struct {

    char *psz_location;          /* The device description XML document's URI */

    char *psz_device_type;       /* The UPnP type of the device, such as
                                    urn:schemas-upnp-org:device:MediaServer:1 */

    char *psz_udn;               /* The Unique Device Name of the server. */

    char *psz_friendly_name;     /* The friendly name of the media server. */

    char *psz_icon_url;          /* A URL pointing to an icon that graphically
                                    identifies the server. (optional) */

    char *psz_manufacturer;      /* A string identifying the manufacturer
                                    of the server. */

    char *psz_manufacturer_url;  /* A URL pointing to the manufacturer's
                                   web site. (optional) */

    char *psz_model_description; /* A description of the server. (optional) */

    char *psz_model_name;        /* The model name of the server. */

    char *psz_model_number;      /* The server's version number. (optional) */

    char *psz_serial_number;     /* The server's serial number. (optional) */

    char *psz_presentation_url;  /* The presentation URL of the server, that is,
                                    a link to its HTML management interface.
                                    (optional) */

    vlc_dictionary_t *p_dlna_caps; /* Represents the device capabilities
                                      as announced in the device description
                                      file via the dlna:X_DLNACAP element.
                                      A value of -1 for the
                                      srs-rt-retention-period capability
                                      denotes an infinite retention period. */

    uint32_t i_system_update_id; /* An integer value that is incremented every
                                    time changes are made to the DMS. */

    vlc_array_t *p_search_caps;  /* List of property names that can be used
                                    in search queries. Empty if not supported
                                    by the device. */

    vlc_array_t *p_sort_caps;    /* List of property names that can be used
                                    to sort Search() or Browse() action results.
                                    Empty if not supported by the device. */

    vlc_array_t *p_sort_ext_caps;/* List of sort modifiers that can be used
                                    to sort Search() or Browse() results.
                                    Empty if not supported by the device. */

    vlc_array_t *p_feature_list; /* Each element in the FeatureList array
                                    represents a feature supported by the DMS.
                                    Each feature contains three pieces of
                                    information, a name, a version number
                                    and an array of object paths that
                                    clients can use to take advantage of the
                                    feature. There are three standardized
                                    feature names, BOOKMARK, EPG and TUNER.
                                    (optional) */
} dleyna_media_server_t;

/**
 * Passed to a fetcher thread as a command for retrieving
 * the list of media served by a DMS through dLeyna
 *
 * TODO: Add a callback and a result field
 */
struct fetch_media_command
{
    services_discovery_t *p_sd;   /* This instance */
    dleyna_media_server_t *p_dms; /* A media server from which
                                     content should be fetched */
};


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
    vlc_array_t *p_media_servers;
    vlc_array_t *p_fetcher_threads;
};

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

static void  *Probe( void* );
static void  *Fetch( void* );

static int SubscribeToMediaServer( services_discovery_t *p_sd,
                                   const char *psz_server );

static int GetAllDBusProperties( services_discovery_t *p_sd,
                                 const char *psz_object_path,
                                 const char *psz_interface,
                                 DBusMessage **pp_result );

static int GetDBusProperty( services_discovery_t *p_sd,
                            const char *psz_object_path,
                            const char *psz_interface,
                            const char *psz_property,
                            DBusMessage **pp_result );

static char* DemarshalStringPropertyValue( services_discovery_t *p_sd,
                                           DBusMessage *p_msg );

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

    p_sys->p_media_servers = vlc_array_new();
    p_sys->p_fetcher_threads = vlc_array_new();

    if( unlikely(!p_sys->p_media_servers) )
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
    vlc_array_destroy( p_sd->p_sys->p_media_servers );
    vlc_array_destroy( p_sd->p_sys->p_fetcher_threads );
    free( p_sys );

    return VLC_EGENERIC;
}

/*****************************************************************************
 * Close: cleanup
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = (services_discovery_t*) p_this;
    vlc_array_t* p_media_servers = p_sd->p_sys->p_media_servers;
    vlc_array_t* p_fetcher_threads = p_sd->p_sys->p_fetcher_threads;

    vlc_cancel( p_sd->p_sys->probe_thread );
    vlc_join( p_sd->p_sys->probe_thread, NULL );

    for( int i = 0; i < vlc_array_count( p_fetcher_threads ); i++ )
    {
        vlc_thread_t *p_fetcher_thread = NULL;
        p_fetcher_thread = vlc_array_item_at_index( p_fetcher_threads, i );

        vlc_cancel( *p_fetcher_thread );
        vlc_join( *p_fetcher_thread, NULL );
    }

    dbus_connection_close( p_sd->p_sys->p_conn );
    dbus_connection_unref( p_sd->p_sys->p_conn );

    for( int i = 0; i < vlc_array_count( p_media_servers ); i++ )
    {
        dleyna_media_server_t *p_dms =
            vlc_array_item_at_index( p_media_servers, i );

        free( p_dms->psz_location );
        free( p_dms->psz_device_type );
        free( p_dms->psz_udn );
        free( p_dms->psz_friendly_name );
        free( p_dms->psz_icon_url );
        free( p_dms->psz_manufacturer );
        free( p_dms->psz_manufacturer_url );
        free( p_dms->psz_model_description );
        free( p_dms->psz_model_name );
        free( p_dms->psz_model_number );
        free( p_dms->psz_serial_number );
        free( p_dms->psz_presentation_url );

        if( p_dms->p_dlna_caps )
            vlc_dictionary_clear( p_dms->p_dlna_caps, NULL, NULL );

        vlc_array_destroy( p_dms->p_search_caps );
        vlc_array_destroy( p_dms->p_sort_caps );
        vlc_array_destroy( p_dms->p_sort_ext_caps );
        vlc_array_destroy( p_dms->p_feature_list );
    }

    vlc_array_destroy( p_media_servers );
    vlc_array_destroy( p_fetcher_threads );
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

    /* Wait for all the fetcher threads to finish
     * before returning control */
    vlc_array_t *p_threads = p_sd->p_sys->p_fetcher_threads;
    int i_threads = vlc_array_count( p_threads );

    for( int i = 0; i < i_threads; i++ )
    {
        vlc_thread_t *p_fetcher_thread = vlc_array_item_at_index( p_threads, i );
        vlc_join( *p_fetcher_thread, NULL );
        vlc_array_remove( p_threads, i );
        free( p_fetcher_thread );
    }

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

    DBusMessageIter properties, dict, dict_entry, variant;
    DBusMessage *p_properties_msg = NULL;
    dleyna_media_server_t *p_dms = NULL;
    int i_type = DBUS_TYPE_INVALID;
    char *psz_property_name = NULL;
    char *psz_property_value = NULL;

    assert( psz_server );
    msg_Dbg( p_sd, "Subscribing to media server at %s", psz_server );

    p_dms = calloc(1, sizeof( dleyna_media_server_t ) );

    if( !p_dms )
        return VLC_ENOMEM;

    vlc_array_append( p_sd->p_sys->p_media_servers, p_dms );

    int ret = GetAllDBusProperties( p_sd,
            psz_server, DLEYNA_DEVICE_INTERFACE,
            &p_properties_msg );

    if( VLC_SUCCESS != ret )
        return ret;

    if( !dbus_message_iter_init( p_properties_msg, &properties ) )
    {
        msg_Err( p_sd, "Empty reply from media server" );
        return VLC_EGENERIC;
    }

    if( DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type( &properties ) ||
            strcmp( "a{sv}", dbus_message_iter_get_signature( &properties ) ) )
    {
        msg_Err( p_sd, "Invalid reply from media server" );
        return VLC_EGENERIC;
    }

    dbus_message_iter_recurse( &properties, &dict );
    while( ( i_type = dbus_message_iter_get_arg_type( &dict ) )
            != DBUS_TYPE_INVALID )
    {
        dbus_message_iter_recurse( &dict, &dict_entry );
        dbus_message_iter_get_basic( &dict_entry, &psz_property_name );

        msg_Dbg( p_sd, "Reading property %s", psz_property_name );

        dbus_message_iter_next( &dict_entry );
        dbus_message_iter_recurse( &dict_entry, &variant );

        if( !strcmp( "Location", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_location = strdup( psz_property_value );
        }

        else if( !strcmp( "DeviceType", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_device_type = strdup( psz_property_value );
        }

        else if( !strcmp( "UDN", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_udn = strdup( psz_property_value );
        }

        else if( !strcmp( "FriendlyName", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_friendly_name = strdup( psz_property_value );
        }

        else if( !strcmp( "IconURL", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_icon_url = strdup( psz_property_value );
        }

        else if( !strcmp( "Manufacturer", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_manufacturer = strdup( psz_property_value );
        }

        else if( !strcmp( "ManufacturerUrl", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_manufacturer_url = strdup( psz_property_value );
        }

        else if( !strcmp( "ManufacturerUrl", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_manufacturer_url = strdup( psz_property_value );
        }

        else if( !strcmp( "ModelDescription", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_model_description = strdup( psz_property_value );
        }

        else if( !strcmp( "ModelName", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_model_name = strdup( psz_property_value );
        }

        else if( !strcmp( "ModelNumber", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_model_number = strdup( psz_property_value );
        }

        else if( !strcmp( "SerialNumber", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_serial_number = strdup( psz_property_value );
        }

        else if( !strcmp( "PresentationURL", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant, &psz_property_value );
            msg_Dbg( p_sd, "%s: %s", psz_property_name, psz_property_value );

            p_dms->psz_presentation_url = strdup( psz_property_value );
        }

        else if( !strcmp( "SystemUpdateID", psz_property_name ) )
        {
            dbus_message_iter_get_basic( &variant,
                    &p_dms->i_system_update_id );

            msg_Dbg( p_sd, "%s: %d", psz_property_name,
                    p_dms->i_system_update_id );
        }

        else
            msg_Warn( p_sd, "Unknown property %s", psz_property_name );

        dbus_message_iter_next( &dict );
    }

    dbus_message_unref( p_properties_msg );

    input_item_t *p_input_item = input_item_New( "vlc://nop",
            p_dms->psz_friendly_name );

    input_item_SetURL( p_input_item, p_dms->psz_presentation_url );
    input_item_SetArtURL( p_input_item, p_dms->psz_icon_url );
    input_item_SetCopyright( p_input_item, p_dms->psz_manufacturer );
    input_item_SetDescription( p_input_item, p_dms->psz_model_description );

    services_discovery_AddItem( p_sd, p_input_item, NULL );

    /* Start a new thread for fetching the media server contents */
    vlc_thread_t *p_fetcher_thread = malloc( sizeof(vlc_thread_t) );
    if( !p_fetcher_thread )
        return VLC_EGENERIC;

    struct fetch_media_command *cmd = NULL;
    cmd = calloc( 1, sizeof(struct fetch_media_command) );
    cmd->p_sd   = p_sd;
    cmd->p_dms  = p_dms;

    if( vlc_clone( p_fetcher_thread, Fetch, cmd, VLC_THREAD_PRIORITY_LOW ) )
    {
        free( cmd );
        free( p_fetcher_thread );
        return VLC_EGENERIC;
    }

    vlc_array_append( p_sd->p_sys->p_fetcher_threads, p_fetcher_thread );

    return VLC_SUCCESS;
}

/**
 * Calls the org.freedesktop.DBus.Properties.GetAll method synchrously
 * and returns the method call reply message.
 *
 * @param services_discovery_t  *p_sd             This SD instance
 * @param const char            *psz_object_path  A DBus object path
 * @param const char            *psz_interface    A DBus interface name
 * @param DBusMessage          **pp_result        Reply placeholder
 *
 * @return int VLC error code
 */
static int GetAllDBusProperties( services_discovery_t *p_sd,
                                 const char *psz_object_path,
                                 const char *psz_interface,
                                 DBusMessage **pp_result )
{
    DBusMessage *p_call = NULL, *p_reply = NULL;

    DBusError err;
    dbus_error_init( &err );

    msg_Dbg( p_sd, "Getting DBus properties for %s on %s",
            psz_interface, psz_object_path );

    p_call = dbus_message_new_method_call( DLEYNA_SERVICE_NAME,
            psz_object_path, DBUS_INTERFACE_PROPERTIES, "GetAll" );

    if( !p_call )
        return VLC_ENOMEM;

    if( !dbus_message_append_args( p_call,
                DBUS_TYPE_STRING, &psz_interface,
                DBUS_TYPE_INVALID ) )
    {
        dbus_message_unref( p_call );
        return VLC_EGENERIC;
    }

    p_reply = dbus_connection_send_with_reply_and_block( p_sd->p_sys->p_conn,
            p_call, DBUS_TIMEOUT_USE_DEFAULT, &err );

    dbus_message_unref( p_call );

    if( !p_reply )
    {
        if( dbus_error_is_set( &err ) )
        {
            msg_Err( p_sd, "DBus error: %s", err.message );
            dbus_error_free( &err );
        }

        msg_Err( p_sd, "Failed to get DBus properties for %s on %s",
              psz_interface, psz_object_path );

        return VLC_EGENERIC;
    }

    *pp_result = p_reply;
    return VLC_SUCCESS;
}

/**
 * Calls the org.freedesktop.DBus.Properties.Get method synchrously
 * and returns the method call reply message.
 *
 * @param services_discovery_t  *p_sd             This SD instance
 * @param const char            *psz_object_path  A DBus object path
 * @param const char            *psz_interface    A DBus interface name
 * @param const char            *psz_property     A DBus property
 * @param DBusMessage          **pp_result        Reply placeholder
 *
 * @return int VLC error code
 */
static int GetDBusProperty( services_discovery_t *p_sd,
                            const char *psz_object_path,
                            const char *psz_interface,
                            const char *psz_property,
                            DBusMessage **pp_result )
{
    DBusMessage *p_call = NULL, *p_reply = NULL;

    DBusError err;
    dbus_error_init( &err );

    msg_Dbg( p_sd, "Getting DBus property %s.%s on %s",
            psz_interface, psz_property, psz_object_path );

    p_call = dbus_message_new_method_call( DLEYNA_SERVICE_NAME,
            psz_object_path, DBUS_INTERFACE_PROPERTIES, "Get" );

    if( !p_call )
        return VLC_ENOMEM;

    if( !dbus_message_append_args( p_call,
                DBUS_TYPE_STRING, &psz_interface,
                DBUS_TYPE_STRING, &psz_property,
                DBUS_TYPE_INVALID ) )
    {
        dbus_message_unref( p_call );
        return VLC_EGENERIC;
    }

    p_reply = dbus_connection_send_with_reply_and_block( p_sd->p_sys->p_conn,
            p_call, DBUS_TIMEOUT_USE_DEFAULT, &err );

    dbus_message_unref( p_call );

    if( !p_reply )
    {
        if( dbus_error_is_set( &err ) )
        {
            msg_Err( p_sd, "DBus error: %s", err.message );
            dbus_error_free( &err );
        }

        msg_Err( p_sd, "Failed to get DBus property %s.%s on %s",
              psz_interface, psz_property, psz_object_path );

        return VLC_EGENERIC;
    }

    *pp_result = p_reply;
    return VLC_SUCCESS;
}

/**
 * Reads a string property from a org.freedesktop.Properties.Get reply message
 * The returned string should be freed after use
 *
 * @param services_discovery_t  *p_sd           This SD instance
 * @param DBusMessage           *p_msg          A dbus reply message
 *
 * @return const char* The DBus property value as a string
 */
static char *DemarshalStringPropertyValue( services_discovery_t *p_sd,
                                                 DBusMessage *p_msg )
{
    DBusMessageIter property_value, variant;
    const char *psz_ret;

    if( !dbus_message_iter_init( p_msg, &variant ) )
    {
        msg_Err( p_sd, "Empty reply from media server" );
        return NULL;
    }

    if( DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type( &variant ) )
    {
        msg_Err( p_sd, "Invalid reply from media server" );
        return NULL;
    }

    dbus_message_iter_recurse( &variant, &property_value );

    if( DBUS_TYPE_STRING != dbus_message_iter_get_arg_type( &property_value ) )
    {
        msg_Err( p_sd, "Invalid DBus property type" );
        return NULL;
    }

    dbus_message_iter_get_basic( &property_value, &psz_ret );
    return strdup( psz_ret );
}

/**
 * Fetches the contents of a media server from dLeyna
 *
 * This is supposed to be used as a fetcher thread function
 *
 * @param void* p_data A fetch_media_command data structure
 * @return NULL
 */
static void *Fetch( void* p_data )
{
    assert( p_data );

    struct fetch_media_command *cmd = (struct fetch_media_command*)p_data;
    services_discovery_t  *p_sd  = cmd->p_sd;
    dleyna_media_server_t *p_dms = cmd->p_dms;
    free( cmd );

    msg_Dbg( p_sd, "Fetching Data from %s", p_dms->psz_friendly_name );
    msleep( 2000000 );
    msg_Dbg( p_sd, "Done Fetching Data... (not really, but bear with me here)" );

    return NULL;
}
