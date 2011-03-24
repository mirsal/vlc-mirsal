/*****************************************************************************
 * content-directory.c : UPnP A/V ContentDirectory service
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

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#ifdef _HAVE_CONFIG_H
#    include "config.h"
#endif

#include <assert.h>

#include <vlc_common.h>
#include <vlc_playlist.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "content-directory.h"
#include "service.h"
#include "didl.h"

struct _content_directory_t
{
    service_t* p_service;
    vlc_dictionary_t* p_handlers;
    int i_update_id;
};

static void handle_browse( void* ev, void* user_data );
static void handle_get_search_capabilities( void* ev, void* user_data );
static void handle_get_sort_capabilities( void* ev, void* user_data );
static void handle_get_system_update_id( void* ev, void* user_data );
static didl_t* browse_metadata( vlc_object_t* p_this, int i_object_id );
static didl_t* browse_direct_children( vlc_object_t* p_this, int i_object_id,
       int i_start_index, int i_requested_count );
static char* get_request_string_value( struct Upnp_Action_Request* p_ar,
                                       const char* psz_key );

content_directory_t* content_directory_init( vlc_object_t* p_parent,
        webserver_t* p_webserver, dlna_t* p_libdlna, char* psz_upnp_base_url )
{
    content_directory_t* p_this = malloc( sizeof( content_directory_t ) );
    if( !p_this ) return NULL;

    p_this->p_handlers = malloc( sizeof( vlc_dictionary_t ) );
    if( !p_this->p_handlers )
    {
        free( p_this );
        return NULL;
    }

    vlc_dictionary_init( p_this->p_handlers, 1 );
    vlc_dictionary_insert( p_this->p_handlers, "Browse", &handle_browse );
    vlc_dictionary_insert( p_this->p_handlers, "GetSearchCapabilities",
            &handle_get_search_capabilities );
    vlc_dictionary_insert( p_this->p_handlers, "GetSortCapabilities",
            &handle_get_sort_capabilities );
    vlc_dictionary_insert( p_this->p_handlers, "GetSystemUpdateID",
            &handle_get_system_update_id );

    p_this->p_service = service_init( p_parent, p_webserver, p_libdlna,
            p_this->p_handlers, psz_upnp_base_url, "ContentDirectory",
            CDS_DESCRIPTION, CDS_SERVICE_TYPE, CDS_SERVICE_ID );

    if( !p_this->p_service )
        return NULL;

    p_this->i_update_id = 1;

    return p_this;
}

void content_directory_destroy( content_directory_t* p_this )
{
    service_destroy( p_this->p_service );
    vlc_dictionary_clear( p_this->p_handlers, NULL, NULL );
    free( p_this->p_handlers );
    free( p_this );
}

static char* get_request_string_value( struct Upnp_Action_Request* p_ar,
                                       const char* psz_key )
{
    char* psz_value;
    IXML_NodeList* p_nodes;
    IXML_Node* p_node;
    
    if( !(p_nodes = ixmlDocument_getElementsByTagName(
                    (IXML_Document*) p_ar->ActionRequest, psz_key )) )
        return NULL;

    p_node = ixmlNodeList_item( p_nodes, 0 );
    ixmlNodeList_free( p_nodes );

    if( !p_node )
        return NULL;

    p_node = ixmlNode_getFirstChild( p_node );
    psz_value = (char*) ixmlNode_getNodeValue( p_node );

    if( !psz_value )
        return NULL;

    return strdup( psz_value );
}

static didl_t* browse_metadata( vlc_object_t* p_this, int i_object_id )
{
    didl_t* p_didl = didl_init( p_this );
    playlist_t* p_playlist = pl_Get( p_this );
    playlist_item_t* p_item;
    
    if( i_object_id == 0 )
    {
        didl_add_container( p_didl, p_playlist->current.i_size );
        didl_finalize( p_didl );
        return p_didl;
    }

    if( !(p_item = playlist_ItemGetById( p_playlist, i_object_id )) )
    {
        didl_finalize( p_didl );
        return p_didl;
    }

    // FIXME: as we send this list to remote user, we need to provide
    // http://style URL here, not the one we got from playlist!
    didl_add_item( p_didl, p_item->i_id, "object.item.audioItem",
            p_item->p_input->psz_name,
            "http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01",
            p_item->p_input->psz_uri );

    didl_finalize( p_didl );

    msg_Dbg( p_this, "DIDL: %s", didl_print( p_didl ) );
    
    return p_didl;
}
static didl_t* browse_direct_children( vlc_object_t* p_this,
        int i_object_id, int i_start_index, int i_requested_count )
{
    didl_t* p_didl = didl_init( p_this );
    playlist_t* p_playlist = NULL;
    
    if( i_object_id != 0 )
    {
        didl_finalize( p_didl );
        return p_didl;
    }

    p_playlist = pl_Get( p_this );
    PL_LOCK;

    if( !i_start_index && !i_requested_count )
        i_requested_count = p_playlist->current.i_size;

    for( int i=0;
            (i < p_playlist->current.i_size && i < (i_start_index + i_requested_count));
            ++i )
    {
        didl_add_item( p_didl, p_playlist->current.p_elems[i]->p_input->i_id,
                "object.item.audioItem",
                p_playlist->current.p_elems[i]->p_input->psz_name,
                "http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01",
                p_playlist->current.p_elems[i]->p_input->psz_uri );
    }
    PL_UNLOCK;
    didl_finalize( p_didl );

    msg_Dbg( p_this, "Direct Children DIDL: %s", didl_print( p_didl ) );
    
    return p_didl;
}

static void handle_browse( void* ev, void* user_data )
{
    didl_t* p_result = NULL;
    char* psz_didl, *psz_count;
    content_directory_t* p_this = (content_directory_t*) user_data;
    service_t* p_cds = *(service_t**) p_this;
    struct Upnp_Action_Request* p_ar = (struct Upnp_Action_Request*) ev;
    char* psz_browse_flag = get_request_string_value( p_ar, "BrowseFlag" );
    if( !psz_browse_flag ) return;

    int i_object_id = atoi( get_request_string_value( p_ar, "ObjectID" ) );
    int i_start_index = atoi( get_request_string_value( p_ar, "StartingIndex" ) );
    int i_requested_count = atoi( get_request_string_value( p_ar, "RequestedCount" ) );

    if( !strcmp( psz_browse_flag, "BrowseMetadata" ) )
        p_result = browse_metadata( p_cds->p_parent, i_object_id );
    
    if( !strcmp( psz_browse_flag, "BrowseDirectChildren" ) )
        p_result = browse_direct_children( p_cds->p_parent, i_object_id,
               i_start_index, i_requested_count );
    
    psz_didl = didl_print( p_result );
    if( psz_didl )
    {
        if( asprintf( &psz_count, "%d", didl_count( p_result ) ) == -1 )
            return;
    }
    else
    {
        psz_didl = DIDL_EMPTY_DOC;
        psz_count = "0";
    }

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
    p_this->p_service->psz_type, "Result", psz_didl );

    free( psz_didl );

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "NumberReturned",
            psz_count );

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "TotalMatches",
            psz_count );

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "UpdateID", "0" );
    
    didl_destroy( p_result );
    
    msg_Dbg( p_this->p_service->p_parent, "UPnP Action response in handle_browse: %s",
            ixmlPrintDocument( p_ar->ActionResult ) );
}

static void handle_get_search_capabilities( void* ev, void* user_data )
{
    content_directory_t* p_this = (content_directory_t*) user_data;
    struct Upnp_Action_Request* p_ar = (struct Upnp_Action_Request*) ev;

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "SearchCapabilities", "" );

    msg_Dbg( p_this->p_service->p_parent, "UPnP Action response: %s",
            ixmlPrintDocument( p_ar->ActionResult ) );

}

static void handle_get_sort_capabilities( void* ev, void* user_data )
{
    content_directory_t* p_this = (content_directory_t*) user_data;
    struct Upnp_Action_Request* p_ar = (struct Upnp_Action_Request*) ev;

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "SortCapabilities", "" );

    msg_Dbg( p_this->p_service->p_parent, "UPnP Action response: %s",
            ixmlPrintDocument( p_ar->ActionResult ) );
}

static void handle_get_system_update_id( void* ev, void* user_data )
{
    content_directory_t* p_this = (content_directory_t*) user_data;
    struct Upnp_Action_Request* p_ar = (struct Upnp_Action_Request*) ev;
    char* psz_update_id;

    if( asprintf( &psz_update_id, "%d", p_this->i_update_id ) == -1 )
        return;

    UpnpAddToActionResponse( &p_ar->ActionResult, p_ar->ActionName,
            p_this->p_service->psz_type, "Id", psz_update_id );

    msg_Dbg( p_this->p_service->p_parent, "UPnP Action response: %s",
            ixmlPrintDocument( p_ar->ActionResult ) );
}
