/*****************************************************************************
 * didl.c : DIDL manipulation functions
 *****************************************************************************
 * Copyright © 2008 Mirsal Ennaime
 * $Id$
 *
 * Authors:     Mirsal Ennaime <mirsal dot ennaime at gmail dot com>
 *
 * Sightly inspired from libdlna code written by Benjamen Zores.
 *      http://libdlna.geexbox.org
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
#include <stdio.h>
#include <assert.h>
#include "didl.h"

#define DIDL_ROOT_OPEN \
"<DIDL-Lite" \
" xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" \
" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"" \
" xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"

#define DIDL_ROOT_CLOSE \
"</DIDL-Lite>"

#define DIDL_CONTAINER_FORMAT \
"<container id=\"%d\" parentID=\"%d\" childCount=\"%d\"" \
" restricted=\"true\" searchable=\"true\">" \
"    <dc:title>%s</dc:title>" \
"    <upnp:class>object.container.storageFolder</upnp:class>" \
"</container>"

struct _didl_t
{
    char* psz_xml;

    vlc_object_t* p_parent;

    size_t i_size;
    int i_items;
    bool b_consistant;
    bool b_finalized;
};

struct _didl_container_t
{
    int id;
    bool b_closed;
};

static char* didl_append_string( didl_t* p_didl, const char* psz )
{
    size_t i_psz_size;

    if( !p_didl || !psz || p_didl->b_finalized )
        return NULL;

    i_psz_size = strlen( psz ); 
    p_didl->psz_xml = realloc( p_didl->psz_xml, p_didl->i_size + i_psz_size+1 );
    strcat( p_didl->psz_xml, psz );
    p_didl->i_size += i_psz_size;

    return p_didl->psz_xml;
}

static char* didl_appendf( didl_t* p_didl, const char* psz_format, ... )
{
    va_list va_args;
    char* psz, *psz_ret;
    int i_len;
    
    if( !p_didl || !psz_format || p_didl->b_finalized )
        return NULL;

    va_start( va_args, psz_format );
    if( vasprintf( &psz, psz_format, va_args ) == -1 )
        return NULL;
    va_end( va_args );
    
    psz_ret = didl_append_string( p_didl, psz );
    free( psz );

    return psz_ret;
}

static char* didl_append_tag( didl_t* p_didl, const char* psz_tag,
        const char* psz_value )
{
    if( !p_didl || p_didl->b_finalized || !psz_tag || !psz_value )
        return NULL;

    return didl_appendf( p_didl, "<%s>%s</%s>", psz_tag, psz_value, psz_tag );
}

static char* didl_append_param( didl_t* p_didl, const char* psz_param,
        const char* psz_value )
{
    if( !p_didl || p_didl->b_finalized || !psz_param || !psz_value )
        return NULL;

    return didl_appendf( p_didl, " %s=\"%s\"", psz_param, psz_value );
}

static char* didl_append_value( didl_t* p_didl, const char* psz_param,
        const int psz_value )
{
    if( !p_didl || p_didl->b_finalized || !psz_param )
        return NULL;

    return didl_appendf( p_didl, " %s=\"%d\"", psz_param, psz_value );
}

didl_t* didl_init( vlc_object_t* p_parent )
{
    didl_t* p_this = calloc( 1, sizeof( didl_t ) );
    
    if( !p_this )
        return NULL;
    
    p_this->psz_xml = strdup( DIDL_ROOT_OPEN );
    p_this->p_parent = p_parent;

    if( !p_this->psz_xml )
    {
        free( p_this );
        return NULL;
    }

    p_this->i_size = strlen( p_this->psz_xml );
    p_this->b_consistant = true;

    return p_this;
}

void didl_destroy( didl_t* p_this )
{
    if( !p_this )
        return;
    free( p_this->psz_xml );
    free( p_this );
}

char* didl_print( didl_t* p_didl )
{
    if (!p_didl || !p_didl->b_finalized || !p_didl->b_consistant )
        return NULL;

    return strdup( p_didl->psz_xml );
}

void didl_finalize( didl_t* p_didl )
{
    if (!p_didl || p_didl->b_finalized || !p_didl->b_consistant )
        return;

    didl_append_string( p_didl, DIDL_ROOT_CLOSE );
    p_didl->b_finalized = true;
}

int didl_count( didl_t* p_didl )
{
    if( !p_didl || !p_didl->b_finalized )
        return -1;

    return p_didl->i_items;
}

void didl_add_container( didl_t* p_didl, int i_items )
{
    if( !p_didl || p_didl->b_finalized )
        return;

    didl_appendf( p_didl, DIDL_CONTAINER_FORMAT,
            0, 0, i_items, "VLC media player" );
    ++p_didl->i_items;
}

void didl_add_item( didl_t* p_didl, int i_id, char* psz_upnp_class, 
        char* psz_title, char* psz_protocol_info, char* psz_url )
{
    if( !p_didl || p_didl->b_finalized || !psz_upnp_class || !psz_title )
        return;

    p_didl->b_consistant = false;

    if( !didl_append_string( p_didl, "<item" ) )
        return;

    if( !didl_append_value( p_didl, "id", i_id ) )
        return;

    if( !didl_append_value( p_didl, "parentID", 0 ) )
        return;

    if( !didl_append_string( p_didl, ">" ) )
        return;

    if( !didl_append_tag( p_didl, "dc:title", psz_title ) )
        return;

    if( !didl_append_tag( p_didl, "upnp:class", psz_upnp_class ) )
        return;

    if( !didl_append_string( p_didl, "<res " ) )
        return;

    if( !didl_append_param( p_didl, "protocolInfo", psz_protocol_info ) )
        return;

    if( !didl_append_string( p_didl, ">" ) )
        return;

    if( !didl_append_string( p_didl, psz_url ) )
        return;

    if( !didl_append_string( p_didl, "</res>" ) )
        return;

    if( !didl_append_string( p_didl, "</item>" ) )
        return;

    ++p_didl->i_items;
    p_didl->b_consistant = true;
}
