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
#include <upnp/upnp.h>

#include "webserver.h"
#include "device-description.h"

static int ws_get_info( const char* psz_filename, struct File_Info* p_info );
static int ws_read( UpnpWebFileHandle file, char* p_buf, size_t i_buflen );
static int ws_write( UpnpWebFileHandle file, char* p_buf, size_t i_buflen );
static int ws_seek( UpnpWebFileHandle file, long offset, int i_origin );
static int ws_close( UpnpWebFileHandle file);
static UpnpWebFileHandle ws_open( const char* psz_filename,
        enum UpnpOpenFileMode i_mode );

int webserver_init()
{
    struct UpnpVirtualDirCallbacks virtual_dir_callbacks =
    {
        ws_get_info,
        ws_open,
        ws_read,
        ws_write,
        ws_seek,
        ws_close
    };

    UpnpSetVirtualDirCallbacks( &virtual_dir_callbacks );
    UpnpAddVirtualDir( "/services" );
    UpnpEnableWebserver( true );

    return VLC_SUCCESS;
}

static int ws_get_info( const char* psz_filename, struct File_Info* p_info )
{
    return 0;
}

static int ws_read( UpnpWebFileHandle file, char* p_buf, size_t i_buflen )
{
    return 0;
}

static int ws_write( UpnpWebFileHandle file, char* p_buf, size_t i_buflen )
{
    return 0;
}

static int ws_seek( UpnpWebFileHandle file, long offset, int i_origin )
{
    return 0;
}

static int ws_close( UpnpWebFileHandle file)
{
    return 0;
}

static UpnpWebFileHandle ws_open( const char* psz_filename,
        enum UpnpOpenFileMode i_mode )
{
    return NULL;
}
