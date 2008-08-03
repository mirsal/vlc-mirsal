/*****************************************************************************
 * upnp.c : UPnP A/V Media Server
 *****************************************************************************
 * Copyright © 2008 Mirsal Ennaime
 * $Id$ 
 *
 * Authors:     Mirsal Ennaime <mirsal dot ennaime at gmail dot com>
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
#include <vlc_plugin.h>
#include <vlc_interface.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <dlna.h>

#include <stdio.h>

#include "service.h"
#include "webserver.h"
#include "content-directory.h"
#include "device-description.h"

static int  Open    ( vlc_object_t * );
static void Close   ( vlc_object_t * );
static void Run     ( intf_thread_t * );

static int dispatch_event( Upnp_EventType event_type, void* ev, void* cookie );
static void handle_subscription_request( intf_thread_t* p_intf,
        struct Upnp_Subscription_Request* p_req );
static void dispatch_action_request( intf_thread_t* p_intf,
        struct Upnp_Action_Request* ar );

struct intf_sys_t
{
    webserver_t* p_webserver;
    dlna_t*      p_libdlna;
    UpnpDevice_Handle* p_device_handle;
    content_directory_t* p_content_directory;
    webserver_service_t* p_device_description;
    char* psz_upnp_base_url;
};

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

vlc_module_begin();
    set_shortname( N_("upnp"));
    set_category( CAT_INTERFACE );
    set_subcategory( SUBCAT_INTERFACE_CONTROL );
    set_description( N_("UPnP A/V Media Server") );
    set_capability( "interface", 0 );
    set_callbacks( Open, Close );
vlc_module_end();

/*****************************************************************************
 * Open: initialize interface
 *****************************************************************************/

static int Open( vlc_object_t* p_this )
{
    intf_thread_t   *p_intf = (intf_thread_t*)p_this;
    intf_sys_t      *p_sys  = calloc( 1, sizeof( intf_sys_t ) );
    int              e;

    if( !p_sys )
        return VLC_ENOMEM;

    p_intf->pf_run = Run;
    p_intf->p_sys = p_sys;

    if( (e = UpnpInit( NULL, 0 )) != UPNP_E_SUCCESS )
    {
        msg_Err( p_this, "%s", UpnpGetErrorMessage( e ));
        free( p_sys );
        return VLC_EGENERIC;
    }

    msg_Info( p_this, "UPnP subsystem initialized on %s:%d",
           UpnpGetServerIpAddress(), UpnpGetServerPort() );
    
    p_sys->p_libdlna = dlna_init();

    if( !(p_sys->p_webserver = webserver_init( p_this, 
                    UpnpGetServerIpAddress(), 0 )) )
    {
        msg_Err( p_this, "Webserver initialization failed" );
        free( p_sys );
        return VLC_EGENERIC;
    } 

    if( asprintf( &p_sys->psz_upnp_base_url, "http://%s:%d",
            UpnpGetServerIpAddress(), UpnpGetServerPort() ) == -1 )
    {
        webserver_destroy( p_sys->p_webserver );
        free( p_sys );
        return VLC_ENOMEM;
    }

    dlna_device_set_friendly_name( p_sys->p_libdlna, FRIENDLY_NAME );
    dlna_device_set_manufacturer( p_sys->p_libdlna, MANUFACTURER );
    dlna_device_set_manufacturer_url( p_sys->p_libdlna, MANUFACTURER_URL );
    dlna_device_set_model_description( p_sys->p_libdlna, MODEL_DESCRIPTION );
    dlna_device_set_model_name( p_sys->p_libdlna, MODEL_NAME );
    dlna_device_set_model_number( p_sys->p_libdlna, MODEL_NUMBER );
    dlna_device_set_model_url( p_sys->p_libdlna, MODEL_URL );
    dlna_device_set_serial_number( p_sys->p_libdlna, SERIAL_NUMBER );
    dlna_device_set_uuid( p_sys->p_libdlna, UUID ); //XXX: should be generated
    dlna_device_set_presentation_url( p_sys->p_libdlna, PRESENTATION_URL );

    p_sys->p_content_directory = content_directory_init( p_this,
            p_sys->p_webserver, p_sys->p_libdlna, p_sys->psz_upnp_base_url );

    p_sys->p_device_description =
        webserver_register_service( p_sys->p_webserver,
                MEDIASERVER_DESCRIPTION_URL,
                dlna_dms_description_get( p_sys->p_libdlna ) ); 

    p_sys->p_device_handle = malloc( sizeof( UpnpDevice_Handle ) );

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: cleanup interface
 *****************************************************************************/

static void Close( vlc_object_t *p_this )
{
    intf_thread_t   *p_intf     = (intf_thread_t*) p_this;
    intf_sys_t      *p_sys      = p_intf->p_sys;     

    content_directory_destroy( p_sys->p_content_directory );
    webserver_unregister_service( p_sys->p_device_description );
    webserver_destroy( p_sys->p_webserver ); 
    UpnpFinish();
    dlna_uninit( p_sys->p_libdlna );
    free( p_sys->psz_upnp_base_url );
    free( p_sys );
}

static int dispatch_event( Upnp_EventType event_type, void* ev, void* cookie )
{
    intf_thread_t* p_intf = (intf_thread_t*) cookie;
    msg_Dbg( p_intf, "Catched an event, dispatching it");

    if (event_type == UPNP_CONTROL_ACTION_REQUEST)
        dispatch_action_request( p_intf, (struct Upnp_Action_Request*) ev );
    else if (event_type == UPNP_EVENT_SUBSCRIPTION_REQUEST)
        handle_subscription_request( p_intf,
                (struct Upnp_Subscription_Request*) ev );

    return 0; //The return value of this function is ignored by the SDK
}

//TODO: Finish that
static void handle_subscription_request( intf_thread_t* p_intf,
        struct Upnp_Subscription_Request* p_req )
{
    intf_sys_t* p_sys = p_intf->p_sys;
    service_t* p_cds = *(service_t**) p_sys->p_content_directory;
    msg_Dbg( p_intf, "Handling subscription request to service %s",
               p_req->ServiceId );
    if (!strcmp( p_cds->psz_id, p_req->ServiceId ))
        UpnpAcceptSubscription( p_sys->p_device_handle,
                "urn:schemas-upnp-org:device:MediaServer:1", p_cds->psz_id,
                NULL, NULL, 0, "12345" );
}

static void dispatch_action_request( intf_thread_t* p_intf,
        struct Upnp_Action_Request* ar )
{
    intf_sys_t* p_sys = p_intf->p_sys;
    service_t* p_cds = *(service_t**) p_intf->p_sys->p_content_directory;
    service_request_handler_t pf_request_handler = NULL;

    msg_Dbg( (vlc_object_t*) p_intf,
            "Dispatching %s action request to service %s",
            ar->ActionName, ar->ServiceID );
    
    if( !strcmp( p_cds->psz_id, ar->ServiceID ) )
    {
        pf_request_handler =
            (service_request_handler_t) vlc_dictionary_value_for_key(
                p_cds->p_request_handlers, ar->ActionName );
        if( pf_request_handler )
                pf_request_handler( (void*) ar,
                        (void*) p_sys->p_content_directory );
    }
}

/*****************************************************************************
 * Run: main loop
 *****************************************************************************/

static void Run( intf_thread_t *p_intf )
{
    int e;
    intf_sys_t* p_sys = p_intf->p_sys;
    char* psz_url;
    
    if( asprintf( &psz_url,
                "%s%s", webserver_get_base_url( p_sys->p_webserver ),
                 MEDIASERVER_DESCRIPTION_URL ) == -1 )
        return;

    if ((e = UpnpRegisterRootDevice(
            psz_url,
            dispatch_event, (void*) p_intf,
            p_sys->p_device_handle )) != UPNP_E_SUCCESS)
        msg_Err( p_intf, "%s", UpnpGetErrorMessage( e ));

    free( psz_url );

    while( !intf_ShouldDie( p_intf ) )
    {
        msleep( INTF_IDLE_SLEEP );
    }
    
    UpnpUnRegisterRootDevice( *p_sys->p_device_handle );
}
