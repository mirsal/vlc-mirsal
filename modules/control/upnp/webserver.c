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

#include <dlna.h>

#include <stdlib.h>

#include "webserver.h"
#include "device-description.h"

struct httpd_file_sys_t
{
    httpd_file_t* p_file;
    char* psz_url;
    char* psz_content;
};

struct _webserver_t
{
    vlc_object_t* p_parent;
    httpd_host_t* p_host;
    char* psz_hostname;
    int i_port;
    httpd_file_sys_t* p_device_description;
};

static int device_description_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data );

webserver_t* webserver_init( vlc_object_t* p_parent,
        char* psz_host, int i_port )
{
    webserver_t* p_this = (webserver_t*) malloc( sizeof( webserver_t ) );
    httpd_file_sys_t* p_device_description =
        (httpd_file_sys_t*) calloc( 1, sizeof( httpd_file_sys_t) );

    p_device_description->psz_content = dlna_dms_description_get( 
        FRIENDLY_NAME, MANUFACTURER, MANUFACTURER_URL, MODEL_DESCRIPTION,
        MODEL_NAME, MODEL_NUMBER, MODEL_URL, SERIAL_NUMBER, UUID,
        PRESENTATION_URL, CMS_SCPD_URL, CMS_CONTROL_URL, CMS_EVENT_URL,
        CDS_SCPD_URL, CDS_CONTROL_URL, CMS_EVENT_URL );
    srand( time( NULL ) );

    p_this->p_parent = p_parent;
    p_this->p_device_description = p_device_description;

    /*FIXME: ugly */
    if (i_port)
        p_this->p_host = httpd_HostNew( p_parent, psz_host, i_port );
    else do
    {
        i_port = rand() % 60000 + 1024;
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

    p_device_description->psz_url = strdup( MEDIASERVER_DESCRIPTION_URL );
    p_device_description->p_file = httpd_FileNew( p_this->p_host,
            p_device_description->psz_url, "text/xml", NULL, NULL, NULL,
            device_description_cb, p_device_description ); 

    return p_this;
}

void webserver_destroy( webserver_t* p_this )
{
    httpd_FileDelete( p_this->p_device_description->p_file );
    httpd_HostDelete( p_this->p_host );
    free( p_this->p_device_description->psz_url );
    free( p_this->p_device_description );
    free( p_this->psz_hostname );
    free( p_this );
}

static int device_description_cb( httpd_file_sys_t* p_sys, httpd_file_t *p_file,
        uint8_t *psz_request, uint8_t **pp_data, int *pi_data )
{
    VLC_UNUSED( p_file ); VLC_UNUSED( psz_request );

    char** ppsz_data = (char**) pp_data;

    *ppsz_data = p_sys->psz_content;
    *pi_data = strlen( *ppsz_data );

    return VLC_SUCCESS;
}

char* webserver_get_device_description_url( webserver_t* p_this )
{
    char* psz_url = NULL;
    asprintf( &psz_url, "http://%s:%d%s", p_this->psz_hostname, p_this->i_port,
            p_this->p_device_description->psz_url );
    return psz_url;
}
