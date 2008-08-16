/*****************************************************************************
 * connection-manager.c : UPnP A/V ConnectionManager service
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

#ifdef _HAVE_CONFIG_H
#    include "config.h"
#endif

#include <vlc_common.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <stdio.h>

#include "connection-manager.h"
#include "service.h"

struct _connection_manager_t
{
    service_t* p_service;
    vlc_dictionary_t* p_handlers;
};

connection_manager_t* connection_manager_init( vlc_object_t* p_parent,
        webserver_t* p_webserver, dlna_t* p_libdlna, char* psz_upnp_base_url )
{
    connection_manager_t* p_this = malloc( sizeof( connection_manager_t ) );

    p_this->p_handlers = malloc( sizeof( vlc_dictionary_t ) );
    vlc_dictionary_init( p_this->p_handlers, 1 );

    p_this->p_service = service_init( p_parent, p_webserver, p_libdlna,
            p_this->p_handlers, psz_upnp_base_url, "ConnectionManager",
            CMS_DESCRIPTION, CMS_SERVICE_TYPE, CMS_SERVICE_ID );

    return p_this;
}

void connection_manager_destroy( connection_manager_t* p_this )
{
    service_destroy( p_this->p_service );
    vlc_dictionary_clear( p_this->p_handlers );
    free( p_this->p_handlers );
    free( p_this );
}
