/*****************************************************************************
 * webserver.h : UPnP A/V Media Server web server
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

#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include <vlc_common.h>
#include <vlc_httpd.h>

typedef struct
{
    vlc_object_t* p_parent;
    httpd_host_t* p_host;
    httpd_file_sys_t* p_device_description;
} webserver_t;

webserver_t* webserver_init( vlc_object_t*, char* psz_host, int i_port );
void         webserver_destroy( webserver_t* p_webserver );

#endif //!webserver.h
