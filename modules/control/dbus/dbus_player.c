/*****************************************************************************
 * dbus-player.h : dbus control module (mpris v1.0) - /Player object
 *****************************************************************************
 * Copyright © 2006-2008 Rafaël Carré
 * Copyright © 2007-2010 Mirsal Ennaime
 * Copyright © 2009-2010 The VideoLAN team
 * $Id$
 *
 * Authors:    Mirsal Ennaime <mirsal at mirsal fr>
 *             Rafaël Carré <funman at videolanorg>
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
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_playlist.h>
#include <vlc_interface.h>
#include <vlc_aout.h>

#include <math.h>

#include "dbus_player.h"
#include "dbus_common.h"

static int MarshalStatus ( intf_thread_t *, DBusMessageIter * );

/* XML data to answer org.freedesktop.DBus.Introspectable.Introspect requests */
static const char* psz_player_introspection_xml =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node>"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.DBus.Properties\">\n"
"    <method name=\"Get\">\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"out\" type=\"v\"/>\n"
"    </method>\n"
"    <method name=\"Set\">\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"v\"/>\n"
"    </method>\n"
"    <signal name=\"PropertiesChanged\">\n"
"      <arg type=\"s\"/>\n"
"      <arg type=\"a{sv}\"/>\n"
"      <arg type=\"as\"/>\n"
"    </signal>\n"
"  </interface>\n"
"  <interface name=\"org.mpris.MediaPlayer.Player\">\n"
"    <property name=\"Metadata\" type=\"a{sv}\" access=\"read\" />\n"
"    <property name=\"PlaybackStatus\" type=\"s\" access=\"read\" />\n"
"    <property name=\"LoopStatus\" type=\"s\" access=\"readwrite\" />\n"
"    <property name=\"Volume\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"Shuffle\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"Position\" type=\"i\" access=\"read\" />\n"
"    <property name=\"Rate\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"MinimumRate\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"MaximumRate\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"CanControl\" type=\"b\" access=\"read\" />\n"
"    <property name=\"CanPlay\" type=\"b\" access=\"read\" />\n"
"    <property name=\"CanPause\" type=\"b\" access=\"read\" />\n"
"    <property name=\"CanSeek\" type=\"b\" access=\"read\" />\n"
"    <method name=\"Previous\" />\n"
"    <method name=\"Next\" />\n"
"    <method name=\"Stop\" />\n"
"    <method name=\"Play\" />\n"
"    <method name=\"Pause\" />\n"
"    <method name=\"PlayPause\" />\n"
"    <method name=\"OpenUri\">\n"
"      <arg type=\"s\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"SetPosition\">\n"
"      <arg type=\"s\" direction=\"in\" />\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"Seek\">\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"    </method>\n"
"    <signal name=\"TrackChanged\">\n"
"      <arg type=\"a{sv}\"/>\n"
"    </signal>\n"
"    <signal name=\"StatusChanged\">\n"
"      <arg type=\"(idbbb)\"/>\n"
"    </signal>\n"
"    <signal name=\"CapabilitiesChanged\">\n"
"      <arg type=\"i\"/>\n"
"    </signal>\n"
"    <signal name=\"MetadataChanged\">\n"
"      <arg type=\"a{sv}\" />\n"
"    </signal>\n"
"    <signal name=\"Seeked\">\n"
"      <arg type=\"x\" />\n"
"    </signal>\n"
"  </interface>\n"
"</node>\n"
;

DBUS_METHOD( Position )
{ /* returns position in microseconds */
    REPLY_INIT;
    OUT_ARGUMENTS;
    dbus_int32_t i_pos;

    input_thread_t *p_input = playlist_CurrentInput( PL );

    if( !p_input )
        i_pos = 0;
    else
    {
        i_pos = var_GetTime( p_input, "time" );
        vlc_object_release( p_input );
    }
    ADD_INT32( &i_pos );
    REPLY_SEND;
}

DBUS_METHOD( SetPosition )
{ /* set position in microseconds */

    REPLY_INIT;
    dbus_int32_t i_pos;
    vlc_value_t position;
    char *psz_trackid, *psz_dbus_trackid;
    input_item_t *p_item;

    DBusError error;
    dbus_error_init( &error );

    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_OBJECT_PATH, &psz_dbus_trackid,
            DBUS_TYPE_INT64, &i_pos,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    input_thread_t *p_input = playlist_CurrentInput( PL );

    if( p_input )
    {
        if( ( p_item = input_GetItem( p_input ) ) )
        {
            if( -1 == asprintf( &psz_trackid,
                                MPRIS_TRACKID_FORMAT,
                                p_item->i_id ) )
            {
                vlc_object_release( p_input );
                return DBUS_HANDLER_RESULT_NEED_MEMORY;
            }

            if( !strcmp( psz_trackid, psz_dbus_trackid ) )
            {
                position.i_time = (mtime_t) i_pos;
                var_Set( p_input, "time", position );
            }
        }

        vlc_object_release( p_input );
    }


    REPLY_SEND;
}

DBUS_METHOD( Seek )
{
    REPLY_INIT;
    dbus_int32_t i_step;
    vlc_value_t  newpos;
    mtime_t      i_pos;

    DBusError error;
    dbus_error_init( &error );

    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_INT32, &i_step,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    input_thread_t *p_input = playlist_CurrentInput( PL );
    if( p_input && var_GetBool( p_input, "can-seek" ) )
    {
        i_pos = var_GetTime( p_input, "time" );
        newpos.i_time = (mtime_t) i_step + i_pos;

        if( newpos.i_time < 0 )
            newpos.i_time = 0;

        var_Set( p_input, "time", newpos );
    }

    if( p_input )
        vlc_object_release( p_input );

    REPLY_SEND;
}

static void
MarshalVolume( intf_thread_t *p_intf, DBusMessageIter *container )
{
    audio_volume_t i_vol = aout_VolumeGet( PL );

    /* A volume of 1.0 represents a sensible maximum, ie: 0dB */
    double d_vol = (double) i_vol / ( AOUT_VOLUME_MAX >> 2 );

    dbus_message_iter_append_basic( container, DBUS_TYPE_DOUBLE, &d_vol );
}

DBUS_METHOD( VolumeGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalVolume( p_this, &args );

    REPLY_SEND;
}

DBUS_METHOD( VolumeSet )
{
    REPLY_INIT;
    double d_dbus_vol;

    if( VLC_SUCCESS != DemarshalSetPropertyValue( p_from, &d_dbus_vol ) )
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if( d_dbus_vol > 1. )
        d_dbus_vol = 1.;
    else if( d_dbus_vol < 0. )
        d_dbus_vol = 0.;

    double d_vol = d_dbus_vol * AOUT_VOLUME_MAX;
    audio_volume_t i_vol = round( d_vol );
    aout_VolumeSet( PL, i_vol );

    REPLY_SEND;
}

DBUS_METHOD( Next )
{ /* next playlist item */
    REPLY_INIT;
    playlist_Next( PL );
    REPLY_SEND;
}

DBUS_METHOD( Prev )
{ /* previous playlist item */
    REPLY_INIT;
    playlist_Prev( PL );
    REPLY_SEND;
}

DBUS_METHOD( Stop )
{ /* stop playing */
    REPLY_INIT;
    playlist_Stop( PL );
    REPLY_SEND;
}

DBUS_METHOD( Play )
{
    REPLY_INIT;
    input_thread_t *p_input =  playlist_CurrentInput( PL );

    if( !p_input || var_GetInteger( p_input, "state" ) != PLAYING_S )
        playlist_Play( PL );

    if( p_input )
        vlc_object_release( p_input );

    REPLY_SEND;
}

DBUS_METHOD( Pause )
{
    REPLY_INIT;
    input_thread_t *p_input = playlist_CurrentInput( PL );

    if( p_input && var_GetInteger(p_input, "state") == PLAYING_S )
        playlist_Pause( PL );

    if( p_input )
        vlc_object_release( p_input );

    REPLY_SEND;
}

DBUS_METHOD( PlayPause )
{
    REPLY_INIT;
    input_thread_t *p_input = playlist_CurrentInput( PL );

    if( p_input && var_GetInteger(p_input, "state") == PLAYING_S )
        playlist_Pause( PL );
    else
        playlist_Play( PL );

    if( p_input )
        vlc_object_release( p_input );

    REPLY_SEND;
}

DBUS_METHOD( OpenUri )
{
    REPLY_INIT;

    char *psz_mrl;
    DBusError error;
    dbus_error_init( &error );

    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_STRING, &psz_mrl,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    playlist_Add( PL, psz_mrl, NULL,
                  PLAYLIST_APPEND | PLAYLIST_GO,
                  PLAYLIST_END, true, false );

    REPLY_SEND;
}

static void
MarshalCanPlay( intf_thread_t *p_intf, DBusMessageIter *container )
{
    dbus_bool_t b_can_play = p_intf->p_sys->b_can_play;
    dbus_message_iter_append_basic( container, DBUS_TYPE_BOOLEAN, &b_can_play );
}

DBUS_METHOD( CanPlay )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalCanPlay( p_this, &args );

    REPLY_SEND;
}

static void
MarshalCanPause( intf_thread_t *p_intf, DBusMessageIter *container )
{
    dbus_bool_t b_can_pause = FALSE;
    input_thread_t *p_input;
    p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist );

    if( p_input )
    {
        b_can_pause = var_GetBool( p_input, "can-pause" );
        vlc_object_release( p_input );
    }

    dbus_message_iter_append_basic( container, DBUS_TYPE_BOOLEAN,
                                    &b_can_pause );
}

DBUS_METHOD( CanPause )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalCanPause( p_this, &args );

    REPLY_SEND;
}

DBUS_METHOD( CanControl )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    dbus_bool_t b_can_control = TRUE;

    ADD_BOOL( &b_can_control );
    REPLY_SEND;
}

static void
MarshalCanSeek( intf_thread_t *p_intf, DBusMessageIter *container )
{
    dbus_bool_t b_can_seek = FALSE;
    input_thread_t *p_input;
    p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist );

    if( p_input )
    {
        b_can_seek = var_GetBool( p_input, "can-seek" );
        vlc_object_release( p_input );
    }

    dbus_message_iter_append_basic( container, DBUS_TYPE_BOOLEAN, &b_can_seek );
}

DBUS_METHOD( CanSeek )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalCanSeek( p_this, &args );

    REPLY_SEND;
}

static void
MarshalShuffle( intf_thread_t *p_intf, DBusMessageIter *container )
{
    dbus_bool_t b_shuffle = var_GetBool( p_intf->p_sys->p_playlist, "random" );
    dbus_message_iter_append_basic( container, DBUS_TYPE_BOOLEAN, &b_shuffle );
}

DBUS_METHOD( ShuffleGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalShuffle( p_this, &args );

    REPLY_SEND;
}

DBUS_METHOD( ShuffleSet )
{
    REPLY_INIT;
    dbus_bool_t b_shuffle;

    if( VLC_SUCCESS != DemarshalSetPropertyValue( p_from, &b_shuffle ) )
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    var_SetBool( PL, "random", ( b_shuffle == TRUE ) );

    REPLY_SEND;
}

static void
MarshalPlaybackStatus( intf_thread_t *p_intf, DBusMessageIter *container )
{
    input_thread_t *p_input;
    char *psz_playback_status;
    if( ( p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist ) ) )
    {
        switch( var_GetInteger( p_input, "state" ) )
        {
            case OPENING_S:
            case PLAYING_S:
                psz_playback_status = PLAYBACK_STATUS_PLAYING;
                break;
            case PAUSE_S:
                psz_playback_status = PLAYBACK_STATUS_PAUSED;
                break;
            default:
                psz_playback_status = PLAYBACK_STATUS_STOPPED;
        }

        vlc_object_release( (vlc_object_t*) p_input );
    }
    else
        psz_playback_status = PLAYBACK_STATUS_STOPPED;

    dbus_message_iter_append_basic( container, DBUS_TYPE_STRING,
                                    &psz_playback_status );
}

DBUS_METHOD( PlaybackStatus )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalPlaybackStatus( p_this, &args );

    REPLY_SEND;
}

static void
MarshalRate( intf_thread_t *p_intf, DBusMessageIter *container )
{
    double d_rate;
    input_thread_t *p_input;
    if( ( p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist ) ) )
    {
        d_rate = var_GetFloat( p_input, "rate" );
        vlc_object_release( (vlc_object_t*) p_input );
    }
    else
        d_rate = 0.;

    dbus_message_iter_append_basic( container, DBUS_TYPE_DOUBLE, &d_rate );
}

DBUS_METHOD( RateGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalRate( p_this, &args );

    REPLY_SEND;
}

DBUS_METHOD( RateSet )
{
    REPLY_INIT;

    double d_rate;

    if( VLC_SUCCESS != DemarshalSetPropertyValue( p_from, &d_rate ) )
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    input_thread_t *p_input;
    if( ( p_input = playlist_CurrentInput( PL ) ) )
    {
        var_SetFloat( p_input, "rate", (float) d_rate );
        vlc_object_release( (vlc_object_t*) p_input );
    }
    else
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    REPLY_SEND;
}

DBUS_METHOD( MinimumRate )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    double d_min_rate = (double) INPUT_RATE_MIN / INPUT_RATE_DEFAULT;

    ADD_DOUBLE( &d_min_rate );
    REPLY_SEND;
}

DBUS_METHOD( MaximumRate )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    double d_max_rate = (double) INPUT_RATE_MAX / INPUT_RATE_DEFAULT;

    ADD_DOUBLE( &d_max_rate );
    REPLY_SEND;
}

static void
MarshalLoopStatus( intf_thread_t *p_intf, DBusMessageIter *container )
{
    char *psz_loop_status;
    if( var_GetBool( p_intf->p_sys->p_playlist, "repeat" ) )
        psz_loop_status = LOOP_STATUS_TRACK;
    else if( var_GetBool( p_intf->p_sys->p_playlist, "loop" ) )
        psz_loop_status = LOOP_STATUS_PLAYLIST;
    else
        psz_loop_status = LOOP_STATUS_NONE;

    dbus_message_iter_append_basic( container, DBUS_TYPE_STRING,
                                    &psz_loop_status );
}

DBUS_METHOD( LoopStatusGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalLoopStatus( p_this, &args );

    REPLY_SEND;
}

DBUS_METHOD( LoopStatusSet )
{
    REPLY_INIT;
    char *psz_loop_status;

    if( VLC_SUCCESS != DemarshalSetPropertyValue( p_from, &psz_loop_status ) )
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if( !strcmp( psz_loop_status, LOOP_STATUS_NONE ) )
    {
        var_SetBool( PL, "loop",   FALSE );
        var_SetBool( PL, "repeat", FALSE );
    }
    else if( !strcmp( psz_loop_status, LOOP_STATUS_TRACK ) )
    {
        var_SetBool( PL, "loop",   FALSE );
        var_SetBool( PL, "repeat", TRUE  );
    }
    else if( !strcmp( psz_loop_status, LOOP_STATUS_PLAYLIST ) )
    {
        var_SetBool( PL, "loop",   TRUE );
        var_SetBool( PL, "repeat", FALSE  );
    }
    else
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    REPLY_SEND;
}

DBUS_METHOD( Metadata )
{
    REPLY_INIT;
    OUT_ARGUMENTS;
    playlist_t *p_playlist = PL;

    PL_LOCK;
    playlist_item_t* p_item =  playlist_CurrentPlayingItem( p_playlist );
    if( p_item )
        GetInputMeta( p_item->p_input, &args );
    PL_UNLOCK;
    REPLY_SEND;
}

/******************************************************************************
 * Seeked: non-linear playback signal
 *****************************************************************************/
DBUS_SIGNAL( SeekedSignal )
{
    SIGNAL_INIT( DBUS_MPRIS_PLAYER_INTERFACE,
                 DBUS_MPRIS_OBJECT_PATH,
                 "Seeked" );

    OUT_ARGUMENTS;

    dbus_int64_t i_pos = 0;
    intf_thread_t *p_intf = (intf_thread_t*) p_data;
    input_thread_t *p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist );

    if( p_input )
    {
        i_pos = var_GetTime( p_input, "time" );
        vlc_object_release( p_input );
    }

    ADD_INT64( &i_pos );
    SIGNAL_SEND;
}

#define PROPERTY_MAPPING_BEGIN if( 0 ) {}
#define PROPERTY_FUNC( interface, property, function ) \
    else if( !strcmp( psz_interface_name, interface ) && \
             !strcmp( psz_property_name,  property ) ) \
        return function( p_conn, p_from, p_this );
#define PROPERTY_MAPPING_END return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

DBUS_METHOD( GetProperty )
{
    DBusError error;

    char *psz_interface_name = NULL;
    char *psz_property_name  = NULL;

    dbus_error_init( &error );
    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_STRING, &psz_interface_name,
            DBUS_TYPE_STRING, &psz_property_name,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                                         error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    PROPERTY_MAPPING_BEGIN
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Metadata", Metadata )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Position", Position )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "PlaybackStatus", PlaybackStatus )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "LoopStatus", LoopStatusGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Shuffle", ShuffleGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Volume", VolumeGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Rate", RateGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "MinimumRate", MinimumRate )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "MaximumRate", MaximumRate )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "CanControl", CanControl )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "CanPlay", CanPlay )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "CanPause", CanPause )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "CanSeek", CanSeek )
    PROPERTY_MAPPING_END
}

DBUS_METHOD( SetProperty )
{
    DBusError error;

    char *psz_interface_name = NULL;
    char *psz_property_name  = NULL;

    dbus_error_init( &error );
    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_STRING, &psz_interface_name,
            DBUS_TYPE_STRING, &psz_property_name,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                                         error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    PROPERTY_MAPPING_BEGIN
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "LoopStatus", LoopStatusSet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Shuffle",    ShuffleSet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Volume",     VolumeSet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Rate",       RateSet )
    PROPERTY_MAPPING_END
}

#undef PROPERTY_MAPPING_BEGIN
#undef PROPERTY_GET_FUNC
#undef PROPERTY_MAPPING_END

DBUS_METHOD( handle_introspect_player )
{
    VLC_UNUSED(p_this);
    REPLY_INIT;
    OUT_ARGUMENTS;
    ADD_STRING( &psz_player_introspection_xml );
    REPLY_SEND;
}

#define METHOD_FUNC( interface, method, function ) \
    else if( dbus_message_is_method_call( p_from, interface, method ) )\
        return function( p_conn, p_from, p_this )

DBusHandlerResult
handle_player ( DBusConnection *p_conn, DBusMessage *p_from, void *p_this )
{
    if( dbus_message_is_method_call( p_from,
                DBUS_INTERFACE_INTROSPECTABLE, "Introspect" ) )
        return handle_introspect_player( p_conn, p_from, p_this );

    METHOD_FUNC( DBUS_INTERFACE_PROPERTIES,   "Get",        GetProperty );
    METHOD_FUNC( DBUS_INTERFACE_PROPERTIES,   "Set",        SetProperty );
/*  METHOD_FUNC( DBUS_INTERFACE_PROPERTIES,   "GetAll",     GetAllProperties );*/

    /* here D-Bus method names are associated to an handler */

    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Previous",     Prev );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Next",         Next );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Stop",         Stop );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Seek",         Seek );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Play",         Play );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Pause",        Pause );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "PlayPause",    PlayPause );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "OpenUri",      OpenUri );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "SetPosition",  SetPosition );

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

#undef METHOD_FUNC

/*****************************************************************************
 * SeekedEmit: Emits the Seeked signal
 *****************************************************************************/
int SeekedEmit( intf_thread_t * p_intf )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    SeekedSignal( p_intf->p_sys->p_conn, p_intf );
    return VLC_SUCCESS;
}

/**
 * PropertiesChangedSignal() synthetizes and sends the
 * org.freedesktop.DBus.Properties.PropertiesChanged signal
 */
static DBusHandlerResult
PropertiesChangedSignal( intf_thread_t    *p_intf,
                         vlc_dictionary_t *p_changed_properties )
{
    DBusConnection  *p_conn = p_intf->p_sys->p_conn;
    DBusMessageIter changed_properties, invalidated_properties, entry, variant;
    const char *psz_interface_name = DBUS_MPRIS_PLAYER_INTERFACE;
    char **ppsz_properties = NULL;
    int i_properties = 0;

    SIGNAL_INIT( DBUS_INTERFACE_PROPERTIES,
                 DBUS_MPRIS_OBJECT_PATH,
                 "PropertiesChanged" );

    OUT_ARGUMENTS;
    ADD_STRING( &psz_interface_name );
    dbus_message_iter_open_container( &args, DBUS_TYPE_ARRAY, "{sv}",
                                      &changed_properties );

    i_properties = vlc_dictionary_keys_count( p_changed_properties );
    ppsz_properties = vlc_dictionary_all_keys( p_changed_properties );

    for( int i = 0; i < i_properties; i++ )
    {
        dbus_message_iter_open_container( &changed_properties,
                                          DBUS_TYPE_DICT_ENTRY, NULL,
                                          &entry );

        dbus_message_iter_append_basic( &entry, DBUS_TYPE_STRING,
                                        &ppsz_properties[i] );

        if( !strcmp( ppsz_properties[i], "Metadata" ) )
        {
            input_thread_t *p_input;
            p_input = playlist_CurrentInput( p_intf->p_sys->p_playlist );

            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "a{sv}",
                                              &variant );

            if( p_input )
            {
                input_item_t *p_item = input_GetItem( p_input );
                GetInputMeta( p_item, &variant );
                vlc_object_release( p_input );
            }

            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "PlaybackStatus" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "s",
                                              &variant );
            MarshalPlaybackStatus( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "LoopStatus" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "s",
                                              &variant );
            MarshalLoopStatus( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "Rate" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "d",
                                              &variant );
            MarshalRate( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "Shuffle" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "b",
                                              &variant );
            MarshalShuffle( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "Volume" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "d",
                                              &variant );
            MarshalVolume( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "CanSeek" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "b",
                                              &variant );
            MarshalCanSeek( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "CanPlay" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "b",
                                              &variant );
            MarshalCanPlay( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        else if( !strcmp( ppsz_properties[i], "CanPause" ) )
        {
            dbus_message_iter_open_container( &entry,
                                              DBUS_TYPE_VARIANT, "b",
                                              &variant );
            MarshalCanPause( p_intf, &variant );
            dbus_message_iter_close_container( &entry, &variant );
        }
        dbus_message_iter_close_container( &changed_properties, &entry );
    }

    dbus_message_iter_close_container( &args, &changed_properties );
    dbus_message_iter_open_container( &args, DBUS_TYPE_ARRAY, "s",
                                      &invalidated_properties );
    dbus_message_iter_close_container( &args, &invalidated_properties );

    SIGNAL_SEND;
}

/*****************************************************************************
 * PropertiesChangedEmit: Emits the Seeked signal
 *****************************************************************************/
int PlayerPropertiesChangedEmit( intf_thread_t    * p_intf,
                                 vlc_dictionary_t * p_changed_properties )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    PropertiesChangedSignal( p_intf, p_changed_properties );
    return VLC_SUCCESS;
}
