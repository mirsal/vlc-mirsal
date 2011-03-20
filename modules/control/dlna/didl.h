/*****************************************************************************
 * didl.h : DIDL manipulation functions
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

#ifndef _DIDL_H
#define _DIDL_H

#include <vlc_common.h>

typedef struct _didl_t didl_t;
typedef struct _didl_container_t didl_container_t;

didl_t* didl_init( vlc_object_t* p_parent );
void didl_destroy( didl_t* p_didl );
void didl_finalize( didl_t* p_didl );
char* didl_print( didl_t* p_didl );

int didl_count( didl_t* p_didl );

void didl_add_container( didl_t* p_didl, size_t i_items );
void didl_add_item( didl_t* p_didl, int i_id, char* psz_upnp_class,
        char* psz_title, char* psz_protocol_info, char* psz_url );

#endif //!didl.h
