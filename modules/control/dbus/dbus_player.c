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
"  <interface name=\"org.mpris.MediaPlayer.Player\">\n"
"    <property name=\"Status\" type=\"(idbbb)\" access=\"read\" />\n"
"    <property name=\"Metadata\" type=\"a{sv}\" access=\"read\" />\n"
"    <property name=\"Capabilities\" type=\"i\" access=\"read\" />\n"
"    <property name=\"Volume\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"Shuffle\" type=\"d\" access=\"readwrite\" />\n"
"    <property name=\"Position\" type=\"i\" access=\"read\" />\n"
"    <method name=\"Previous\" />\n"
"    <method name=\"Next\" />\n"
"    <method name=\"Stop\" />\n"
"    <method name=\"Play\" />\n"
"    <method name=\"Pause\" />\n"
"    <method name=\"PlayPause\" />\n"
"    <method name=\"SetRepeat\">\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"SetLoop\">\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"AdjustVolume\">\n"
"      <arg type=\"d\" direction=\"in\" />\n"
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
"  </interface>\n"
"</node>\n"
;

DBUS_METHOD( PositionGet )
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
            DBUS_TYPE_STRING, &psz_dbus_trackid,
            DBUS_TYPE_INT32, &i_pos,
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
            if( -1 == asprintf( &psz_trackid, "%d", p_item->i_id ) )
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

DBUS_METHOD( VolumeGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    audio_volume_t i_vol = aout_VolumeGet( PL );
    double d_vol = (double) i_vol / AOUT_VOLUME_MAX;

    ADD_DOUBLE( &d_vol );
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

DBUS_METHOD( AdjustVolume )
{
    REPLY_INIT;

    DBusError error;
    dbus_error_init( &error );

    double d_step, d_new_vol;
    audio_volume_t i_vol, i_new_vol;

    dbus_message_get_args( p_from, &error,
                           DBUS_TYPE_DOUBLE, &d_step,
                           DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    aout_VolumeGet( PL, &i_vol );
    d_new_vol = ( d_step + ( (double) i_vol / AOUT_VOLUME_MAX ) );

    if( d_new_vol > 1. )
        d_new_vol = 1.;
    else if( d_new_vol < 0. )
        d_new_vol = 0.;

    i_new_vol = round( d_new_vol * AOUT_VOLUME_MAX );
    aout_VolumeSet( PL, i_new_vol );
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

DBUS_METHOD( StatusGet )
{ /* returns the current status as a struct of 4 ints */
/*
    First   0 = Playing, 1 = Paused, 2 = Stopped.
    Second  0 = Playing linearly , 1 = Playing randomly.
    Third   0 = Go to the next element once the current has finished playing , 1 = Repeat the current element
    Fourth  0 = Stop playing once the last element has been played, 1 = Never give up playing *
 */
    REPLY_INIT;
    OUT_ARGUMENTS;

    MarshalStatus( p_this, &args );

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

DBUS_METHOD( SetRepeat )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    DBusError error;
    dbus_bool_t b_repeat;

    dbus_error_init( &error );
    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_BOOLEAN, &b_repeat,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    var_SetBool( PL, "repeat", ( b_repeat == TRUE ) );

    REPLY_SEND;
}

DBUS_METHOD( ShuffleGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    dbus_bool_t b_shuffle = var_GetBool( PL, "random" );
    ADD_BOOL( &b_shuffle );

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

DBUS_METHOD( SetLoop )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    DBusError error;
    dbus_bool_t b_loop;

    dbus_error_init( &error );
    dbus_message_get_args( p_from, &error,
            DBUS_TYPE_BOOLEAN, &b_loop,
            DBUS_TYPE_INVALID );

    if( dbus_error_is_set( &error ) )
    {
        msg_Err( (vlc_object_t*) p_this, "D-Bus message reading : %s",
                error.message );
        dbus_error_free( &error );
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    var_SetBool( PL, "loop", ( b_loop == TRUE ) );

    REPLY_SEND;
}

DBUS_METHOD( MetadataGet )
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

DBUS_METHOD( CapabilitiesGet )
{
    REPLY_INIT;
    OUT_ARGUMENTS;

    ADD_INT32( &INTF->p_sys->i_player_caps );

    REPLY_SEND;
}

/*****************************************************************************
 * StatusChanged: Player status change signal
 *****************************************************************************/

DBUS_SIGNAL( StatusChangedSignal )
{ /* send the updated status info on the bus */
    SIGNAL_INIT( DBUS_MPRIS_PLAYER_INTERFACE,
                 DBUS_MPRIS_PLAYER_PATH,
                 "StatusChanged" );

    OUT_ARGUMENTS;

    /* we're called from a callback of input_thread_t, so it can not be
     * destroyed before we return */
    MarshalStatus( (intf_thread_t*) p_data, &args );

    SIGNAL_SEND;
}

/*****************************************************************************
 * TrackChanged: Playlist item change callback
 *****************************************************************************/

DBUS_SIGNAL( TrackChangedSignal )
{ /* emit the metadata of the new item */
    SIGNAL_INIT( DBUS_MPRIS_PLAYER_INTERFACE,
                 DBUS_MPRIS_PLAYER_PATH,
                 "TrackChanged" );

    OUT_ARGUMENTS;

    input_item_t *p_item = (input_item_t*) p_data;
    GetInputMeta ( p_item, &args );

    SIGNAL_SEND;
}

/******************************************************************************
 * CapabilitiesChanged: player capabilities change signal
 *****************************************************************************/
DBUS_SIGNAL( CapabilitiesChangedSignal )
{
    SIGNAL_INIT( DBUS_MPRIS_PLAYER_INTERFACE,
                 DBUS_MPRIS_PLAYER_PATH,
                 "CapsChanged" );

    OUT_ARGUMENTS;

    ADD_INT32( &((intf_thread_t*)p_data)->p_sys->i_player_caps );
    SIGNAL_SEND;
}

/******************************************************************************
 * CapabilitiesChanged: player capabilities change signal
 *****************************************************************************/
DBUS_SIGNAL( MetadataChangedSignal )
{
    SIGNAL_INIT( DBUS_MPRIS_PLAYER_INTERFACE,
                 DBUS_MPRIS_PLAYER_PATH,
                 "MetadataChanged" );

    OUT_ARGUMENTS;

    input_item_t *p_item = (input_item_t*) p_data;
    GetInputMeta ( p_item, &args );

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
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Status", StatusGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Metadata", MetadataGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Capabilities", CapabilitiesGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Position", PositionGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Shuffle", ShuffleGet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Volume", VolumeGet )
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
//    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "LoopStatus", LoopStatusSet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Shuffle",    ShuffleSet )
    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Volume",     VolumeSet )
//    PROPERTY_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "Rate",       RateSet )
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
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "SetRepeat",    SetRepeat );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "SetLoop",      SetLoop );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "AdjustVolume", AdjustVolume );
    METHOD_FUNC( DBUS_MPRIS_PLAYER_INTERFACE, "SetPosition",  SetPosition );

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

#undef METHOD_FUNC

/*****************************************************************************
 * StatusChangedEmit: Emits the StatusChanged signal
 *****************************************************************************/
int PlayerStatusChangedEmit( intf_thread_t * p_intf )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    StatusChangedSignal( p_intf->p_sys->p_conn, p_intf );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * TrackChangedEmit: Emits the TrackChanged signal
 *****************************************************************************/
int TrackChangedEmit( intf_thread_t * p_intf, input_item_t* p_item )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    TrackChangedSignal( p_intf->p_sys->p_conn, p_item );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * PlayerMetadataChangedEmit: Emits the MetadataChanged signal
 *****************************************************************************/
int PlayerMetadataChangedEmit( intf_thread_t * p_intf, input_item_t* p_item )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    MetadataChangedSignal( p_intf->p_sys->p_conn, p_item );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * PlayerCapsChangedEmit: Emits the CapabilitiesChanged signal
 *****************************************************************************/
int PlayerCapsChangedEmit( intf_thread_t * p_intf )
{
    if( p_intf->p_sys->b_dead )
        return VLC_SUCCESS;

    CapabilitiesChangedSignal( p_intf->p_sys->p_conn, p_intf );
    return VLC_SUCCESS;
}

/**
 * MarshalStatus() fills a DBus message container with the media player status
 *
 * This function must be called with p_sys->lock locked
 *
 * @return VLC_SUCCESS on success
 * @param intf_thread_t *p_intf this interface thread state
 * @param DBusMessageIter *args An iterator over the container to fill
 */
static int MarshalStatus( intf_thread_t* p_intf, DBusMessageIter* args )
{
    DBusMessageIter status;
    dbus_int32_t i_state;
    double d_rate;
    dbus_bool_t b_shuffle, b_repeat, b_endless;
    playlist_t* p_playlist = p_intf->p_sys->p_playlist;

    vlc_mutex_lock( &p_intf->p_sys->lock );
    i_state = p_intf->p_sys->i_playing_state;
    vlc_mutex_unlock( &p_intf->p_sys->lock );

    d_rate = ( i_state == PLAYBACK_STATE_PLAYING ) ? 1. : 0.;

    b_shuffle = var_CreateGetBool( p_playlist, "random" );
    b_repeat  = var_CreateGetBool( p_playlist, "repeat" );
    b_endless = b_repeat ? TRUE : var_CreateGetBool( p_playlist, "loop" );

    dbus_message_iter_open_container( args, DBUS_TYPE_STRUCT, NULL, &status );
    dbus_message_iter_append_basic( &status, DBUS_TYPE_INT32, &i_state );
    dbus_message_iter_append_basic( &status, DBUS_TYPE_DOUBLE, &d_rate );
    dbus_message_iter_append_basic( &status, DBUS_TYPE_BOOLEAN, &b_shuffle );
    dbus_message_iter_append_basic( &status, DBUS_TYPE_BOOLEAN, &b_repeat );
    dbus_message_iter_append_basic( &status, DBUS_TYPE_BOOLEAN, &b_endless );
    dbus_message_iter_close_container( args, &status );

    return VLC_SUCCESS;
}

/**
 * UpdatePlayerCaps() updates the player capabilities and sends a
 * CapabilitiesChanged signal if needed
 *
 * This function must be called with the playlist unlocked
 *
 * @param intf_thread_t *p_intf This interface thread state
 */
void UpdatePlayerCaps( intf_thread_t* p_intf )
{
    intf_sys_t* p_sys      = p_intf->p_sys;
    playlist_t* p_playlist = p_sys->p_playlist;

    dbus_int32_t i_caps    = PLAYER_CAN_REPEAT |
                             PLAYER_CAN_LOOP |
                             PLAYER_CAN_SHUFFLE;

    PL_LOCK;
    if( p_playlist->current.i_size > 0 )
        i_caps |= PLAYER_CAN_PLAY | PLAYER_CAN_GO_PREVIOUS | PLAYER_CAN_GO_NEXT;
    PL_UNLOCK;

    input_thread_t* p_input = playlist_CurrentInput( p_playlist );
    if( p_input )
    {
        i_caps |= PLAYER_CAN_PROVIDE_POSITION;

        /* XXX: if UpdatePlayerCaps() is called too early, these are
         * unconditionnaly true */
        if( var_GetBool( p_input, "can-pause" ) )
            i_caps |= PLAYER_CAN_PAUSE;
        if( var_GetBool( p_input, "can-seek" ) )
            i_caps |= PLAYER_CAN_SEEK;
        vlc_object_release( p_input );
    }

    if( p_sys->b_meta_read )
        i_caps |= PLAYER_CAN_PROVIDE_METADATA;

    if( i_caps != p_intf->p_sys->i_player_caps )
    {
        p_sys->i_player_caps = i_caps;
        PlayerCapsChangedEmit( p_intf );
    }
}
