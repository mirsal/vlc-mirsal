/*****************************************************************************
 * device-description.h : UPnP A/V Media Server device description
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

#ifndef _MEDIASERVER_H
#define _MEDIASERVER_H

#include <vlc_common.h>

#define MEDIASERVER_DESCRIPTION_URL "/MediaServer.xml"

#define FRIENDLY_NAME     "VLC media player"
#define MANUFACTURER      "VideoLAN"
#define MANUFACTURER_URL  "http://www.videolan.org"
#define MODEL_DESCRIPTION "VLC media player"
#define MODEL_NAME        "vlc"
#define MODEL_NUMBER      PACKAGE_VERSION
#define MODEL_URL         "http://www.videolan.org/vlc"
#define SERIAL_NUMBER     PACKAGE_VERSION
#define UUID              "0"
#define PRESENTATION_URL  "http://www.videolan.org/vlc"

#endif //!mediaserver.h
