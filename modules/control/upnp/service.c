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
#include "services.h"

struct _service_t
{
    vlc_object_t* p_parent;

    char* psz_url;
    char* psz_description;
};

service_t* service_init( vlc_object_t* p_parent,
        char* psz_url, char* psz_description )
{
    service_t* p_this = (service_t*) calloc( 1, sizeof( service_t ) );
    
    p_this->p_parent = p_parent;
    p_this->psz_url = strdup( psz_url );
    p_this->psz_description = strdup( psz_description );
}

void service_destroy( service_t* p_this )
{
    free( p_this->psz_url );
    free( p_this->psz_description );
}

char* service_get_url( service_t* p_this )
{
    return p_this->psz_url;
}

char* service_get_description( service_t* p_this )
{
    return p_this->psz_description;
}
