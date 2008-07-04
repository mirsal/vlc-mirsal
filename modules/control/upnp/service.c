/*****************************************************************************
 * service.c : UPnP A/V Media Server UPnP services handling
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
#include "service.h"
#include "webserver.h"

#include <string.h>
#include <stdio.h>

service_t* service_init( vlc_object_t* p_parent, webserver_t* p_webserver,
        char* psz_upnp_base_url, char* psz_service_name, char* psz_description,
        char* psz_type, char* psz_id )
{
    service_t* p_this = (service_t*) calloc( 1, sizeof( service_t ) );

    p_this->p_parent = p_parent;
    p_this->psz_description = strdup( psz_description );
    
    asprintf( &p_this->psz_description_url,
            "/services/%s/scpd.xml", psz_service_name );
    asprintf( &p_this->psz_control_url, "%s/services/%s/control",
            psz_upnp_base_url, psz_service_name );
    asprintf( &p_this->psz_event_url, "%s/services/%s/event",
            psz_upnp_base_url, psz_service_name );

    p_this->psz_type = strdup( psz_type );
    p_this->psz_id = strdup( psz_id );

    p_this->p_webserver_service = webserver_register_service( p_webserver,
            p_this->psz_description_url, p_this->psz_description );

    return p_this;
}

void service_destroy( service_t* p_this )
{
    free( p_this->psz_description_url );
    free( p_this->psz_control_url );
    free( p_this->psz_event_url );
    free( p_this->psz_description );
    free( p_this->psz_type );
    free( p_this->psz_id );
    webserver_unregister_service( p_this->p_webserver_service );
    free( p_this );
}

