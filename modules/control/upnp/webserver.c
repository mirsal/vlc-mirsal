/*****************************************************************************
 * webserver.c : UPnP A/V Media Server web server
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
#include <vlc_httpd.h>

#include <stdlib.h>

#include "webserver.h"
#include "device-description.h"

struct httpd_file_sys_t
{
    httpd_file_t* p_file;
    char* psz_content;
};

static int device_description_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data );

webserver_t* webserver_init( vlc_object_t* p_parent,
        char* psz_host, int i_port )
{
    webserver_t* p_webserver = (webserver_t*) malloc( sizeof( webserver_t ) );
    httpd_file_sys_t* p_device_description =
        (httpd_file_sys_t*) malloc( sizeof( httpd_file_sys_t) );

    p_device_description->psz_content = strdup( MEDIASERVER_DESCRIPTION ); 
    
    srand( time( NULL ) );

    p_webserver->p_parent = p_parent;
    p_webserver->p_device_description = p_device_description;

    /*FIXME: ugly */
    if (i_port)
        p_webserver->p_host = httpd_HostNew( p_parent, psz_host, i_port );
    else do
    {
        i_port = rand() % 60000 + 1024;
        p_webserver->p_host = httpd_HostNew( p_parent, psz_host, i_port );
    }
    while (!p_webserver->p_host);

    p_device_description->p_file = httpd_FileNew( p_webserver->p_host,
            "/MediaServer.xml", "text/xml", NULL, NULL, NULL,
            device_description_cb, p_device_description ); 

    return p_webserver;
}

void webserver_destroy( webserver_t* p_webserver )
{
    httpd_FileDelete( p_webserver->p_device_description->p_file );
    httpd_HostDelete( p_webserver->p_host );
    free( p_webserver->p_device_description );
    free( p_webserver );
}

static int device_description_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data )
{
    char** ppsz_data = (char**) pp_data;
    *ppsz_data = p_sys->psz_content;
    *pi_data = strlen( *ppsz_data );
    return VLC_SUCCESS;
}
