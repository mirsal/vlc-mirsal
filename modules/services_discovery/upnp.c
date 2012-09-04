/*****************************************************************************
 * upnp.c :  UPnP A/V service discovery module (libupnp)
 *****************************************************************************
 * Copyright (C) 2004-2012 the VideoLAN team
 * $Id$
 *
 * Authors: Rémi Denis-Courmont <rem # videolan.org> (original c++ plugin)
 *          Mirsal Ennaime <mirsal # videolan org>
 *
 * UPnP services discovery module using the portable upnp library (libupnp)
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

#define __STDC_CONSTANT_MACROS 1

#undef PACKAGE_NAME
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "upnp.h"

#include <vlc_plugin.h>
#include <vlc_services_discovery.h>

#include <assert.h>
#include <limits.h>

/*
 * Constants
*/
const char* MEDIA_SERVER_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
const char* CONTENT_DIRECTORY_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";

/*
 * VLC handle
 */
struct services_discovery_sys_t
{
    UpnpClient_Handle client_handle;
    upnp_sd_device_list_t *p_device_list;
    vlc_mutex_t callback_lock;
};

/*
 * VLC callback prototypes
 */
static int   Open( vlc_object_t* );
static void  Close( vlc_object_t* );
VLC_SD_PROBE_HELPER( "upnp", "Universal Plug'n'Play", SD_CAT_LAN )

/*
 * UPnP callback prototypes
 */
static void
onUpnpSearchResult( services_discovery_t   *p_sd,
                    struct Upnp_Discovery  *p_discovery );
static void
onUpnpServerRemoved( services_discovery_t   *p_sd,
                     struct Upnp_Discovery  *p_discovery );
static void
onUpnpControlEvent( services_discovery_t  *p_sd,
                    struct Upnp_Event     *p_event );
static void
onUpnpSubscriptionExpired( services_discovery_t         *p_sd,
                           struct Upnp_Event_Subscribe  *p_event );
static int
onUpnpEvent( Upnp_EventType event_type, void* p_event, void* p_user_data );

/*
 * Device list utility function prototypes
 */
static void
upnp_sd_device_free( upnp_sd_device_t *p_device );

static void
upnp_sd_device_list_free( upnp_sd_device_list_t *p_device_list );

static void
upnp_sd_device_list_cat( upnp_sd_device_list_t *p_left,
                         upnp_sd_device_list_t *p_right );

static unsigned int
upnp_sd_device_list_len( upnp_sd_device_list_t* p_list );

static upnp_sd_device_list_t*
upnp_sd_device_list_find_udn( upnp_sd_device_list_t* p_list,
                              const char *psz_udn );

static upnp_sd_device_list_t*
upnp_sd_device_list_rm( upnp_sd_device_list_t **pp_list,
                        upnp_sd_device_list_t *p_elem );

/*
* Module descriptor
*/
vlc_module_begin();
    set_shortname( "UPnP" );
    set_description( N_( "Universal Plug'n'Play" ) );
    set_category( CAT_PLAYLIST );
    set_subcategory( SUBCAT_PLAYLIST_SD );
    set_capability( "services_discovery", 0 );
    set_callbacks( Open, Close );

    VLC_SD_PROBE_SUBMODULE
vlc_module_end();

static int Open( vlc_object_t *p_this )
{
    int i_res;
    services_discovery_t *p_sd = ( services_discovery_t* )p_this;
    services_discovery_sys_t *p_sys  = ( services_discovery_sys_t * )
            calloc( 1, sizeof( services_discovery_sys_t ) );

    if( !( p_sd->p_sys = p_sys ) )
        return VLC_ENOMEM;

    /* IPv4 address or ipv6 network interface */
    char* psz_iface = NULL;

    psz_iface = var_InheritString( p_sd, "upnp-iface" );
    msg_Info( p_sd, "Initializing libupnp on %s",
            psz_iface ? psz_iface : "default interface");

#ifdef UPNP_ENABLE_IPV6
    i_res = UpnpInit2( psz_iface, 0 );
    free( psz_iface );
#else
    /* If UpnpInit2 isnt available, initialize on the interface which address
     * was specified  as --upnp-iface or on the first IPv4-capable interface */
    i_res = UpnpInit( psz_iface, 0 );
#endif

    if( i_res != UPNP_E_SUCCESS )
    {
        msg_Err( p_sd, "Initialization failed: %s",
                 UpnpGetErrorMessage( i_res ) );

        free( p_sys );
        return VLC_EGENERIC;
    }

    ixmlRelaxParser( 1 );

    vlc_mutex_init( &p_sys->callback_lock );

    /* Register a control point synchronously */
    i_res = UpnpRegisterClient( onUpnpEvent, p_sd, &p_sys->client_handle );
    if( i_res != UPNP_E_SUCCESS )
    {
        msg_Err( p_sd, "Client registration failed: %s",
                 UpnpGetErrorMessage( i_res ) );

        Close( (vlc_object_t*) p_sd );
        return VLC_EGENERIC;
    }

    /* libupnp does not treat a maximum content length of 0 as unlimited
     * until 64dedf (~ pupnp v1.6.7) and provides no sane way to discriminate
     * between versions */
    if( (i_res = UpnpSetMaxContentLength( INT_MAX )) != UPNP_E_SUCCESS )
    {
        msg_Err( p_sd, "Failed to set maximum content length: %s",
                UpnpGetErrorMessage( i_res ));

        Close( (vlc_object_t*) p_sd );
        return VLC_EGENERIC;
    }

    /* Search for media servers */
    i_res = UpnpSearchAsync( p_sys->client_handle, 5,
            MEDIA_SERVER_DEVICE_TYPE, p_sd );
    if( i_res != UPNP_E_SUCCESS )
    {
        msg_Err( p_sd, "Error sending search request: %s",
                 UpnpGetErrorMessage( i_res ) );

        Close( (vlc_object_t*) p_sd );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/**
 * Reads the device UDN from an XML device description
 *
 * @param services_discovery_t* p_sd This SD instance
 * @param IXML_Element* p_desc
 *
 * @return char* The device's UDN string
 *
 * The returned value must be freed after use
 */
static const char *parseUDN( services_discovery_t *p_sd, IXML_Element *p_desc )
{
    assert(p_sd);
    assert(p_desc);

    IXML_NodeList *p_udns;
    IXML_Node     *p_udn = NULL, *p_udn_text = NULL;

    const char* psz_udn = NULL;

    if( ( p_udns = ixmlElement_getElementsByTagName( p_desc, "UDN" ) ) )
    {
        ( p_udn = ixmlNodeList_item( p_udns, 0 ) ) &&
        ( p_udn_text = ixmlNode_getFirstChild( p_udn ) ) &&
        ( psz_udn = ixmlNode_getNodeValue( p_udn_text ) );
    }

    msg_Dbg( p_sd, "UDN: %s", psz_udn );
    ixmlNodeList_free( p_udns );

    return psz_udn;
}

/**
 * Reads a device attribute from an XML device description fragment
 *
 * @param services_discovery_t* p_sd This SD instance
 * @param IXML_Element* p_desc An XML device description fragment
 *
 * @return char* The parsed attribute value
 *
 * The returned string must be freed after use
 */
static const char *parseDeviceAttribute( services_discovery_t *p_sd,
                                         IXML_Element *p_desc,
                                         const char *psz_attr )
{
    assert(p_sd);
    assert(p_desc);
    assert(psz_attr);

    IXML_NodeList *p_nodes;
    IXML_Node     *p_node = NULL, *p_node_text = NULL;

    const char    *psz_value = NULL;

    if( ( p_nodes = ixmlElement_getElementsByTagName( p_desc, psz_attr ) ) )
    {
        ( p_node = ixmlNodeList_item( p_nodes, 0 ) ) &&
        ( p_node_text = ixmlNode_getFirstChild( p_node ) ) &&
        ( psz_value = ixmlNode_getNodeValue( p_node_text ) );
    }

    msg_Dbg( p_sd, "Parsed %s: %s", psz_attr, psz_value );
    ixmlNodeList_free( p_nodes );

    return psz_value ? psz_value : "";
}

/**
 * Reads the base URL from an XML device list
 *
 * @param services_discovery_t* p_sd This SD instance
 * @param IXML_Document* p_desc an XML device list document
 *
 * @return const char* The base URL
 */
static const char *parseBaseUrl( services_discovery_t *p_sd,
                                 IXML_Document *p_desc )
{
    IXML_NodeList *p_urls;
    IXML_Node     *p_url  = NULL,  *p_url_text = NULL;

    const char    *psz_base_url = NULL;

    if( ( p_urls = ixmlDocument_getElementsByTagName( p_desc, "URLBase" ) ) )
    {
        ( p_url = ixmlNodeList_item( p_urls, 0 ) ) &&
        ( p_url_text = ixmlNode_getFirstChild( p_url ) ) &&
        ( psz_base_url = ixmlNode_getNodeValue( p_url_text ) );
    }

    msg_Dbg( p_sd, "Base URL: %s", psz_base_url );
    ixmlNodeList_free( p_urls );

    return psz_base_url;
}

/**
 * Extracts relevant data from a device description XML fragment
 *
 * @param services_discovery_t* p_sd this SD instance
 * @param const char* psz_base_url The discovery's base URL
 * @param IXML_Element* p_desc an XML device description fragment
 *
 * @return upnp_sd_device_t* A new device instance
 *
 * The returned value must be freed after use
 */
static upnp_sd_device_t *parseDeviceDescription( services_discovery_t *p_sd,
                                                 const char *psz_base_url,
                                                 IXML_Element *p_desc )
{
    upnp_sd_device_t *p_dev   = NULL;
    const char       *psz_udn = NULL;

    const char *psz_manufacturer = NULL, *psz_friendly_name  = NULL,
               *psz_control_url  = NULL, *psz_model_desc     = NULL,
               *psz_pres_url     = NULL;

    assert(p_sd);
    assert(p_desc);

    psz_udn = parseUDN( p_sd, p_desc );

    if( !psz_udn )
        return NULL;

    p_dev = calloc( 1, sizeof( upnp_sd_device_t ) );
    if( unlikely(!p_dev) )
        return NULL;

    psz_friendly_name = parseDeviceAttribute( p_sd, p_desc, "friendlyName" );
    psz_manufacturer  = parseDeviceAttribute( p_sd, p_desc, "manufacturer" );
    psz_model_desc    = parseDeviceAttribute( p_sd, p_desc, "modelDescription" );
    psz_pres_url      = parseDeviceAttribute( p_sd, p_desc, "presentationURL" );
    psz_control_url   = parseDeviceAttribute( p_sd, p_desc, "controlURL");

    p_dev->psz_udn               = strdup( psz_udn );
    p_dev->psz_base_url          = strdup( psz_base_url );
    p_dev->psz_control_url       = strdup( psz_control_url );
    p_dev->psz_manufacturer      = strdup( psz_manufacturer );
    p_dev->psz_friendly_name     = strdup( psz_friendly_name );
    p_dev->psz_presentation_url  = strdup( psz_pres_url );
    p_dev->psz_model_description = strdup( psz_model_desc );

    p_dev->psz_url = (char*) malloc( strlen( psz_base_url ) + strlen( psz_control_url ) + 1 );
    if( UpnpResolveURL( psz_base_url, psz_control_url, p_dev->psz_url ) != UPNP_E_SUCCESS )
        msg_Err( p_sd, "Unable to resolve device URL");

    msg_Dbg( p_sd, "Base URL is %s", p_dev->psz_base_url);
    msg_Dbg( p_sd, "Control URL is %s", p_dev->psz_control_url);
    msg_Dbg( p_sd, "Device URL is %s", p_dev->psz_url);

    return p_dev;
}

/**
 * Extracts relevant data from an XML device list and builds
 * new upnp_sd_device_list entries accordingly
 *
 * @param services_discovery_t* p_sd    This SD instance
 * @param IXML_Document*        p_doc   An XML device list document
 * @param const char*           psz_url The discovery URL
 *
 * @return upnp_sd_device_list_t* The discovered devices list
 */
static upnp_sd_device_list_t *parseDiscoveryResult( services_discovery_t *p_sd,
                                                    IXML_Document *p_doc,
                                                    const char *psz_url )
{
    upnp_sd_device_list_t *p_devs = NULL;
    IXML_NodeList *p_device_nodes = NULL;
    const char *psz_base_url;

    p_device_nodes = ixmlDocument_getElementsByTagName( p_doc, "device" );
    if( !p_device_nodes )
        return NULL;

    /* Fallback to the Device description URL basename
     * if no base URL is advertised */
    psz_base_url = parseBaseUrl( p_sd, p_doc );
    if( !psz_base_url )
    {
        psz_base_url = strdup( psz_url );
        *(strrchr( psz_base_url, '/' ) + 1) = '\0';
    }

    if( unlikely(!psz_base_url) )
    {
        ixmlNodeList_free( p_device_nodes );
        return NULL;
    }

    for ( size_t i = 0; i < ixmlNodeList_length( p_device_nodes ); i++ )
    {
        IXML_Element *p_node    = NULL;
        upnp_sd_device_t *p_dev = NULL;

        p_node = (IXML_Element*)ixmlNodeList_item( p_device_nodes, i );
        p_dev  = parseDeviceDescription( p_sd, psz_base_url, p_node );

        if( !p_dev )
            continue;

        upnp_sd_device_list_t *p_new_dev = (upnp_sd_device_list_t*)
            calloc( 1, sizeof( upnp_sd_device_list_t ) );

        if( unlikely(!p_new_dev) )
        {
            ixmlNodeList_free( p_device_nodes );
            upnp_sd_device_list_free( p_devs );
            upnp_sd_device_free( p_dev );
            return NULL;
        }

        p_new_dev->p_device = p_dev;

        if( !p_devs )
            p_devs = p_new_dev;

        else
            upnp_sd_device_list_cat( p_devs, p_new_dev );
    }

    return p_devs;
}

/**
 * Add newly discovered devices to the playlist
 *
 * This function allocates memory which should be
 * freed after use with upnp_sd_device_list_free()
 *
 * @param services_discovery_t*  p_sd    This SD instance
 * @param upnp_sd_device_list_t* p_devs  A list of newly discovered devices
 *
 * @return int VLC error code
 */
static int addNewDevices( services_discovery_t  *p_sd,
                           upnp_sd_device_list_t *p_devs )
{
    upnp_sd_device_list_t* p_cur = p_devs;

    do
    {
        char *psz_mrl, *psz_friendly_name;
        input_item_t* p_input_item = NULL;
        upnp_sd_device_list_t *p_list_entry = NULL;

        if( !p_cur )
                break;

        if( !p_cur->p_device )
                continue;

        /* Skip already known devices */
        if( upnp_sd_device_list_find_udn( p_sd->p_sys->p_device_list,
                                          p_cur->p_device->psz_udn ) )
        {
                msg_Dbg( p_sd, "Device %s (UDN %s) already present, skipping",
                                p_cur->p_device->psz_friendly_name,
                                p_cur->p_device->psz_udn );
                continue;
        }

        if( unlikely(asprintf( &psz_mrl, "upnp+container://%s?ObjectID=0",
                                p_cur->p_device->psz_url ) < 0) )
                return VLC_ENOMEM;

        /* Create and add a playlist node of type directory */
        msg_Dbg( p_sd, "Adding VLC mrl %s", psz_mrl);
        psz_friendly_name = p_cur->p_device->psz_friendly_name;
        p_input_item = input_item_NewWithTypeExt( psz_mrl, psz_friendly_name,
                        0, NULL, 0, -1, ITEM_TYPE_DIRECTORY, 1);

        free( psz_mrl );
        if( unlikely(!p_input_item) )
                return VLC_ENOMEM;

        services_discovery_AddItem( p_sd, p_input_item, NULL );

        /* Move the newly added device data structure to
         * the list of known devices */
        p_list_entry = (upnp_sd_device_list_t*)
                calloc( 1, sizeof( upnp_sd_device_list_t ) );

        if( unlikely(!p_list_entry) )
        {
                services_discovery_RemoveItem( p_sd, p_input_item );
                input_item_Release( p_input_item );
                return VLC_ENOMEM;
        }

        p_list_entry->p_device = p_cur->p_device;
        p_list_entry->p_device->p_input_item = input_item_Hold( p_input_item );
        p_cur->p_device = NULL;

        if( !p_sd->p_sys->p_device_list )
                p_sd->p_sys->p_device_list = p_list_entry;
        else
                upnp_sd_device_list_cat( p_sd->p_sys->p_device_list, p_list_entry );

    } while( (p_cur = p_cur->p_next) );

    return VLC_SUCCESS;
}

/**
* Handles the UPNP_DISCOVERY_ADVERTISEMENT_ALIVE
* UPNP_DISCOVERY_SEARCH_RESULT libupnp events.
*/
static void
onUpnpSearchResult( services_discovery_t   *p_sd,
                    struct Upnp_Discovery  *p_discovery )
{
    int i_res;
    IXML_Document *p_doc = NULL;
    upnp_sd_device_list_t *p_devs = NULL;
    const char *psz_url = p_discovery->Location;

    msg_Dbg( p_sd, "Downloading device description from \"%s\"", psz_url );
    i_res = UpnpDownloadXmlDoc( psz_url, &p_doc );

    if ( i_res != UPNP_E_SUCCESS && p_doc)
    {
        const char *psz_errmsg = ( UPNP_E_SUCCESS == i_res ) ?
            "Invalid XML document" : UpnpGetErrorMessage( i_res );

        msg_Warn( p_sd, "Could not download device description! "
                        "Fetching data from %s failed: %s",
                        psz_url,  psz_errmsg);
        return;
    }
    msg_Dbg( p_sd, "Received device description: %s", ixmlPrintDocument( p_doc ) );

    p_devs = parseDiscoveryResult( p_sd, p_doc, psz_url );
    msg_Dbg( p_sd, "Parsed discovery result. Found %d new devices",
             upnp_sd_device_list_len( p_devs ));

    msg_Dbg( p_sd, "Adding new UPnP devices to the playlist");
    if( VLC_SUCCESS != addNewDevices( p_sd, p_devs ))
            msg_Err( p_sd, "Could not add discovered devices to the playlist" );

    upnp_sd_device_list_free( p_devs );
    ixmlDocument_free( p_doc );

    return;
}

/**
* Handles the UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE libupnp event.
*/
static void
onUpnpServerRemoved( services_discovery_t   *p_sd,
                 struct Upnp_Discovery  *p_discovery )
{
    upnp_sd_device_list_t *p_list_entry = NULL;

    if( !p_sd->p_sys->p_device_list )
            return;

    p_list_entry = upnp_sd_device_list_find_udn( p_sd->p_sys->p_device_list,
                                                 p_discovery->DeviceId );

    services_discovery_RemoveItem( p_sd, p_list_entry->p_device->p_input_item );
    upnp_sd_device_list_rm( &p_sd->p_sys->p_device_list, p_list_entry );
    msg_Dbg( p_sd, "server removed" );

    return;
}

/**
* Handles the UPNP_EVENT_RECEIVED libupnp event.
*/
static void
onUpnpControlEvent( services_discovery_t  *p_sd,
                    struct Upnp_Event     *p_event )
{
    (void) p_event;

    msg_Dbg( p_sd, "Control Event receieved" );
    return;
}

/**
 * Handles the UPNP_EVENT_AUTORENEWAL_FAILED and
 * UPNP_EVENT_SUBSCRIPTION_EXPIRED libupnp events.
 */
static void
onUpnpSubscriptionExpired( services_discovery_t         *p_sd,
                           struct Upnp_Event_Subscribe  *p_event )
{
    (void) p_event;

    msg_Dbg( p_sd, "Subscription expired" );
    return;
}

/**
 * Handles all UPnP events by routing the call to their respective handlers
 */
static int
onUpnpEvent( Upnp_EventType event_type, void* p_event, void* p_user_data )
{
    services_discovery_t*      p_sd  = ( services_discovery_t* ) p_user_data;

    switch( event_type )
    {
        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
            onUpnpSearchResult( p_sd, ( struct Upnp_Discovery* ) p_event );
            break;

        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
            onUpnpServerRemoved( p_sd, ( struct Upnp_Discovery* ) p_event );
            break;

        case UPNP_EVENT_RECEIVED:
            onUpnpControlEvent( p_sd, ( struct Upnp_Event* ) p_event );
            break;

        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
            onUpnpSubscriptionExpired( p_sd,
                    ( struct Upnp_Event_Subscribe* ) p_event );
            break;

        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
            msg_Dbg( p_sd, "subscription complete" );
            break;

        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
            msg_Dbg( p_sd, "search timeout" );
            break;

        default:
            msg_Err( p_sd, "Unhandled event, please report ( type=%d )", event_type );
            break;
    }

    return UPNP_E_SUCCESS;
}

/**
 * Uninitializes the upnp module
 */
static void Close( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t* ) p_this;
    UpnpUnRegisterClient( p_sd->p_sys->client_handle );
    UpnpFinish();

    upnp_sd_device_list_free( p_sd->p_sys->p_device_list );
    vlc_mutex_destroy( &p_sd->p_sys->callback_lock );

    free( p_sd->p_sys );
    msg_Dbg( p_sd, "Bye Bye" );
}

/**
 * Frees memory used by a upnp_sd_device_t structure
 */
static void upnp_sd_device_free( upnp_sd_device_t *p_device )
{
    if( !p_device )
        return;

    free( p_device->psz_url );
    free( p_device->psz_udn );
    free( p_device->psz_base_url );
    free( p_device->psz_friendly_name );
    free( p_device->psz_manufacturer );
    free( p_device->psz_presentation_url );
    free( p_device->psz_model_description );

    if( p_device->p_input_item )
            input_item_Release( p_device->p_input_item );
}

/**
 * Frees memory used by a device list
 */
static void upnp_sd_device_list_free( upnp_sd_device_list_t *p_list )
{
    if( !p_list )
        return;

    upnp_sd_device_list_free( p_list->p_next );
    upnp_sd_device_free( p_list->p_device );
}

/**
 * Appends p_right at the end of p_left
 */
static void upnp_sd_device_list_cat( upnp_sd_device_list_t *p_left,
                                     upnp_sd_device_list_t *p_right )
{
    assert( p_left );
    assert( p_right );

    if( !p_left->p_next )
    {
        p_left->p_next = p_right;
        return;
    }

    upnp_sd_device_list_cat( p_left->p_next, p_right );
}

/**
 * Counts the number of entries in a device list
 */
static unsigned int upnp_sd_device_list_count( upnp_sd_device_list_t *p_list,
                                               unsigned int i_count )
{
    if( !p_list )
        return i_count;

    return upnp_sd_device_list_count( p_list->p_next, i_count + 1 );
}

/**
 * Returns the length of a device list
 */
static unsigned int upnp_sd_device_list_len( upnp_sd_device_list_t *p_list )
{
    return upnp_sd_device_list_count( p_list, 0 );
}

/**
 * Finds a list entry given its UDN attribute
 *
 * @param upnp_sd_device_list_t* p_list The haystack
 * @param const char* psz_udn The needle's UDN
 *
 * @return upnp_sd_device_list_t* The first matching list entry
 */
static upnp_sd_device_list_t*
upnp_sd_device_list_find_udn( upnp_sd_device_list_t *p_list,
                              const char *psz_udn )
{
    assert(psz_udn);

    if(!p_list)
        return NULL;

    if( !strncmp( psz_udn, p_list->p_device->psz_udn, strlen( psz_udn ) ) )
        return p_list;

    return upnp_sd_device_list_find_udn( p_list->p_next, psz_udn );
}

/**
 * Removes an element from a device list
 *
 * @param upnp_sd_device_list_t* pp_list The list from which to remove an element
 * @param upnp_sd_device_list_t* p_elem The list element to remove
 *
 * @return upnp_sd_device_list_t* The removed element or null if not found
 *
 * Note: This function does not destroy the list element
 */
static upnp_sd_device_list_t*
upnp_sd_device_list_rm( upnp_sd_device_list_t **pp_list,
                        upnp_sd_device_list_t *p_elem )
{
    assert(pp_list);
    assert(p_elem);

    upnp_sd_device_list_t *p_list = *pp_list;
    assert(p_list);

    /* Are we removing the first
     * element of the list ? */
    if(p_elem == p_list )
    {
        *pp_list = p_list->p_next;
        return p_elem;
    }

    if(p_elem == p_list->p_next)
    {
        p_list->p_next = p_elem->p_next;
        return p_elem;
    }

    if(!p_list->p_next)
        return NULL;

    return upnp_sd_device_list_rm( &p_list->p_next, p_elem );
}
