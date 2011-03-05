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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_httpd.h>

#include <dlna.h>

#include <stdio.h>
#include <stdlib.h>

#include "webserver.h"
#include "device-description.h"
#include "content-directory.h"

struct httpd_file_sys_t
{
    httpd_file_t* p_file;
    char* psz_url;
    char* psz_content;
};

struct _webserver_service_t
{
    httpd_file_sys_t* p_sys;
};

struct _webserver_t
{
    vlc_object_t* p_parent;
    httpd_host_t* p_host;
    char* psz_hostname;
    int i_port;
};

static int static_content_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data );

webserver_t* webserver_init( vlc_object_t* p_parent,
        char* psz_host, int i_port )
{
    webserver_t* p_this = (webserver_t*) malloc( sizeof( webserver_t ) );
    
    srand( time( NULL ) );

    p_this->p_parent = p_parent;

    /*FIXME: ugly */
    if (i_port)
        p_this->p_host = httpd_HostNew( p_parent, psz_host, i_port );
    else do
    {
        i_port = rand() % 64511 + 1024;
        p_this->p_host = httpd_HostNew( p_parent, psz_host, i_port );
    }
    while (!p_this->p_host);

    if (!p_this->p_host)
    {
        msg_Err( p_parent, "the httpd could not listen on specified port" );
        return NULL;
    }

    p_this->psz_hostname = strdup( psz_host );
    p_this->i_port = i_port;

    msg_Info( p_parent, "httpd running on %s",
            webserver_get_base_url( p_this ) );

    return p_this;
}

void webserver_destroy( webserver_t* p_this )
{
    httpd_HostDelete( p_this->p_host );
    free( p_this->psz_hostname );
    free( p_this );
}

webserver_service_t* webserver_register_service( webserver_t* p_this,
                                             const char*  psz_url,
                                             const char*  psz_description )
{
    webserver_service_t* p_service =
        (webserver_service_t*) malloc( sizeof( webserver_service_t ) );
    p_service->p_sys =
        (httpd_file_sys_t*) malloc( sizeof( httpd_file_sys_t* ) );
    p_service->p_sys->psz_url = strdup( psz_url );
    p_service->p_sys->psz_content = strdup( psz_description );
    p_service->p_sys->p_file = httpd_FileNew( p_this->p_host,
            p_service->p_sys->psz_url, "text/xml", NULL, NULL, NULL,
            static_content_cb, p_service->p_sys ); 
    return p_service;
}

void webserver_unregister_service( webserver_service_t* p_service )
{
    httpd_FileDelete( p_service->p_sys->p_file );
    free( p_service->p_sys->psz_url );
    free( p_service->p_sys->psz_content );
    free( p_service->p_sys );
    free( p_service );
}


static int static_content_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data )
{
    VLC_UNUSED( p_file ); VLC_UNUSED( psz_request );

    char** ppsz_data = (char**) pp_data;

    *ppsz_data = strdup( p_sys->psz_content );
    *pi_data = strlen( *ppsz_data );

    return VLC_SUCCESS;
}

char* webserver_get_base_url( webserver_t* p_this )
{
    char* psz_url;
    if( asprintf( &psz_url, "http://%s:%d", p_this->psz_hostname,
            p_this->i_port ) == -1 )
        psz_url = NULL;
    return psz_url;
}

