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

#include "content-directory.h"
#include "service.h"

struct _content_directory_t
{
    service_t* p_service;
};

content_directory_t* content_directory_init( vlc_object_t* p_parent,
        webserver_t* p_webserver, char* psz_upnp_base_url )
{
    content_directory_t* p_this = malloc( sizeof( content_directory_t ) );
    p_this->p_service = service_init( p_parent, p_webserver, psz_upnp_base_url,
            "ContentDirectory", CDS_DESCRIPTION );

    return p_this;
}

void content_directory_destroy( content_directory_t* p_this )
{
    service_destroy( p_this->p_service );
    free( p_this );
}
