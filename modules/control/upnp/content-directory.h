/*****************************************************************************
 * content-directory.h : UPnP A/V ContentDirectory service
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

#ifndef _CONTENT_DIRECTORY_H
#define _CONTENT_DIRECTORY_H

#define CDS_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <actionList>" \
"    <action>" \
"      <name>Browse</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>ObjectID</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>BrowseFlag</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_BrowseFlag</relatedStateVariable>" \
"       </argument>" \
"        <argument>" \
"          <name>Filter</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>StartingIndex</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>RequestedCount</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>SortCriteria</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>" \
"       </argument>" \
"        <argument>" \
"          <name>Result</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>NumberReturned</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>TotalMatches</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>UpdateID</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>GetSystemUpdateID</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>Id</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SystemUpdateID</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>GetSearchCapabilities</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>SearchCaps</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SearchCapabilities</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>GetSortCapabilities</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>SortCaps</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SortCapabilities</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"  </actionList>" \
"  <serviceStateTable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_BrowseFlag</name>" \
"      <dataType>string</dataType>" \
"      <allowedValueList>" \
"        <allowedValue>BrowseMetadata</allowedValue>" \
"        <allowedValue>BrowseDirectChildren</allowedValue>" \
"      </allowedValueList>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"yes\">" \
"      <name>SystemUpdateID</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Count</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_SortCriteria</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>SortCapabilities</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Index</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_ObjectID</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_UpdateID</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TagValueList</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Result</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"   <stateVariable sendEvents=\"no\">" \
"      <name>SearchCapabilities</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Filter</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"  </serviceStateTable>" \
"</scpd>"

#endif //!content_directory.h
