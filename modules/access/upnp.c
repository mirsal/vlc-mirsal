/*****************************************************************************
 * upnp.c: UPnP A/V content directory container access module
 *****************************************************************************
 * Copyright (C) 2001, 2002 the VideoLAN team
 * $Id$
 *
 * Authors: Mirsal Ennaime <mirsal at videolan org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_access.h>

static int   Open( vlc_object_t * );
static void  Close( vlc_object_t * );

static input_item_t *ReadDir( access_t *p_access );
static int           Control( access_t *p_access, int i_query, va_list args );

vlc_module_begin ()
    set_shortname( N_("upnp-container") )
    set_description( N_("UPnP A/V CDS container access") )
    set_capability( "access", 0 )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    set_callbacks( Open, Close )
    add_shortcut( "upnpavc", "vlc" )
    add_shortcut( "upnp+container", "vlc" )
vlc_module_end ()

struct access_sys_t
{
};

/**
 * Open: initialize the module
 */
static int Open( vlc_object_t *p_this )
{
    access_t *p_access = (access_t*)p_this;
    access_InitFields( p_access );

    p_access->pf_readdir = ReadDir;
    p_access->pf_control = Control;
    p_access->p_sys = calloc( 1, sizeof( access_sys_t ) );

    msg_Dbg( p_access, "UPnP container access initialized" );

    return VLC_SUCCESS;
}

/**
 * Close: uninitialize the module
 */
static void Close( vlc_object_t *p_this )
{
    access_t *p_access = (access_t*)p_this;

    free( p_access->p_sys );
}

/**
 * ReadDir: Expand a browsable directory
 * TODO: Fetch contents from the media server if needed
 *       and return a VLC input item for the next element
 */
static input_item_t *ReadDir( access_t *p_access )
{
    access_sys_t *p_sys = p_access->p_sys;
    input_item_t *p_item = NULL;

    return p_item;
}

/**
 * Control: Reacts to control queries
 */
static int Control( access_t *p_access, int i_query, va_list args )
{
    switch( i_query )
    {
        /* */
        case ACCESS_CAN_SEEK:
        case ACCESS_CAN_FASTSEEK:
            *va_arg( args, bool* ) = false;
            break;

        case ACCESS_CAN_PAUSE:
        case ACCESS_CAN_CONTROL_PACE:
            *va_arg( args, bool* ) = true;
            break;

        /* */
        case ACCESS_GET_PTS_DELAY:
            *va_arg( args, int64_t * ) = DEFAULT_PTS_DELAY * 1000;
            break;

        /* */
        case ACCESS_SET_PAUSE_STATE:
        case ACCESS_GET_TITLE_INFO:
        case ACCESS_SET_TITLE:
        case ACCESS_SET_SEEKPOINT:
        case ACCESS_SET_PRIVATE_ID_STATE:
        case ACCESS_GET_CONTENT_TYPE:
        case ACCESS_GET_META:
            return VLC_EGENERIC;

        default:
            msg_Warn( p_access, "control query not implemented" );
            return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}
