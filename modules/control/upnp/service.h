/*****************************************************************************
 * service.h : UPnP A/V Media Server UPnP services handling
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

#ifndef _SERVICE_H
#define _SERVICE_H

#include <vlc_common.h>
#include "webserver.h"

typedef void (*service_request_handler_t)(void*);

typedef struct _service_t
{
    vlc_object_t* p_parent;

    char* psz_description_url;
    char* psz_control_url;
    char* psz_event_url;
    char* psz_description;

    webserver_service_t* p_webserver_service;
    service_request_handler_t pf_request_handler;
} service_t;

service_t* service_init( vlc_object_t* p_parent,
                         webserver_t* p_webserver,
                         char* psz_upnp_base_url,
                         char* psz_service_name,
                         char* psz_description );

void service_destroy( service_t* p_this );

#endif //!service.h
