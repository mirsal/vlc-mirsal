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

#include <vlc_common.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "content-directory.h"
#include "service.h"

struct _content_directory_t
{
    service_t* p_service;
    vlc_dictionary_t* p_handlers;
};

static void handle_browse( void* ev, void* user_data );
static void handle_get_search_capabilities( void* ev, void* user_data );
static void handle_get_sort_capabilities( void* ev, void* user_data );
static void handle_get_system_update_id( void* ev, void* user_data );

content_directory_t* content_directory_init( vlc_object_t* p_parent,
        webserver_t* p_webserver, char* psz_upnp_base_url )
{
    content_directory_t* p_this = malloc( sizeof( content_directory_t ) );

    p_this->p_handlers = malloc( sizeof( vlc_dictionary_t ) );
    vlc_dictionary_init( p_this->p_handlers, 1 );
    vlc_dictionary_insert( p_this->p_handlers, "Browse", &handle_browse );
    vlc_dictionary_insert( p_this->p_handlers, "GetSearchCapabilities",
            &handle_get_search_capabilities );
    vlc_dictionary_insert( p_this->p_handlers, "GetSortCapabilities",
            &handle_get_sort_capabilities );
    vlc_dictionary_insert( p_this->p_handlers, "GetSystemUpdateID",
            &handle_get_system_update_id );

    p_this->p_service = service_init( p_parent, p_webserver, p_this->p_handlers,
            psz_upnp_base_url, "ContentDirectory", CDS_DESCRIPTION,
            CDS_SERVICE_TYPE, CDS_SERVICE_ID );

    return p_this;
}

void content_directory_destroy( content_directory_t* p_this )
{
    service_destroy( p_this->p_service );
    vlc_dictionary_clear( p_this->p_handlers );
    free( p_this->p_handlers );
    free( p_this );
}

static void handle_browse( void* ev, void* user_data )
{
    content_directory_t* p_this = (content_directory_t*) user_data;
    msg_Dbg( p_this->p_service->p_parent, "Browse action not yet implemented" );
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
    msg_Dbg( p_this->p_service->p_parent,
            "GetSystemUpdateID action not yet implemented" );
}
