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

#define MEDIASERVER_DESCRIPTION_URL "/MediaServer.xml"

#define MEDIASERVER_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <device>" \
"    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>" \
"    <friendlyName>VLC media player</friendlyName>" \
"    <manufacturer>VideoLAN</manufacturer>" \
"    <manufacturerURL>http://www.videolan.org/</manufacturerURL>" \
"    <modelDescription>VLC media player</modelDescription>" \
"    <modelName>vlc</modelName>" \
"    <modelNumber>090</modelNumber>" \
"    <modelURL>http://www.videolan.org/</modelURL>" \
"    <serialNumber>vlc-0.9.0-git</serialNumber>" \
"    <UDN>0</UDN>" \
"    <serviceList>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>" \
"        <SCPDURL>/services/ConnectionManager/ConnectionManager.xml</SCPDURL>" \
"        <controlURL>/services/ConnectionManager/control</controlURL>" \
"        <eventSubURL>/services/ConnectionManager/event</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>" \
"        <SCPDURL>/services/ContentDirectory/ContentDirectory.xml</SCPDURL>" \
"        <controlURL>/services/ContentDirectory/control</controlURL>" \
"        <eventSubURL>/services/ContentDirectory/event</eventSubURL>" \
"      </service>" \
"    </serviceList>" \
"    <presentationURL>http://www.videolan.org/</presentationURL>" \
"  </device>" \
"</root>"

#endif //!mediaserver.h
