/*****************************************************************************
 * upnp.h :  UPnP discovery module (libupnp) header
 *****************************************************************************
 * Copyright (C) 2004-2012 the VideoLAN team
 * $Id$
 *
 * Authors: Mirsal Ennaime <mirsal at videolan org>
 *
 * UPnP Plugin using the portable UPnP library (libupnp)
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

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <vlc_common.h>

typedef struct
{
    IXML_Document  *p_device_description;

    char *psz_udn;
    char *psz_url;
    char *psz_base_url;

    char *psz_friendly_name;
    char *psz_manufacturer;
    char *psz_presentation_url;
    char *psz_control_url;
    char *psz_model_description;

    input_item_t *p_input_item;

} upnp_sd_device_t;

typedef struct upnp_sd_device_list_st upnp_sd_device_list_t;

struct upnp_sd_device_list_st
{
    upnp_sd_device_list_t *p_next;
    upnp_sd_device_t      *p_device;

};
