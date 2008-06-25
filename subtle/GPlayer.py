#    This file is part of Subtle
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import pygtk
pygtk.require('2.0')

import gobject
gobject.threads_init()

import pygst
pygst.require('0.10')
import gst
import gst.interfaces
import gtk

## \file GPlayer.py
# Documentation for GPlayer module of Subtle project.
# \todo Add better seeking.


## GstPlayer class.
# Class for playing media in GStreamer.
class GstPlayer:
    ## Construstor
    # \param videowidget - VideoWidget class.
    def __init__(self, videowidget):
        self.playing = False
        bin = gst.Bin('my-bin')
        self.textoverlay = gst.element_factory_make('textoverlay')
        bin.add(self.textoverlay)
        pad = self.textoverlay.get_pad("video_sink")
        ghostpad = gst.GhostPad("sink", pad)
        bin.add_pad(ghostpad)
        color = gst.element_factory_make('ffmpegcolorspace')
        bin.add(color)
        scale = gst.element_factory_make('videoscale')
        bin.add(scale)
        self.videosink = gst.element_factory_make('autovideosink')
        bin.add(self.videosink)
        gst.element_link_many(self.textoverlay, color, scale, self.videosink)
        self.player = gst.element_factory_make("playbin", "player")
        self.player.set_property("video-sink", bin)
        self.videowidget = videowidget

        bus = self.player.get_bus()
        bus.enable_sync_message_emission()
        bus.add_signal_watch()
        bus.connect('sync-message::element', self.on_sync_message)
        bus.connect('message', self.on_message)
        self.cur_frame = 0
        self.rate = 1.0
        
    def on_sync_message(self, bus, message):
        if message.structure is None:
            return
        if message.structure.get_name() == 'prepare-xwindow-id':
            self.videowidget.set_sink(message.src)
            message.src.set_property('force-aspect-ratio', True)

    def on_message(self, bus, message):
        t = message.type
        if t == gst.MESSAGE_ERROR:
            err, debug = message.parse_error()
            print "Error: %s" % err, debug
            self.playing = False
        elif t == gst.MESSAGE_EOS:
            self.playing = False
            
    def fast_forward(self):
        """
            Here we will fast forward the stream for as many times
            as this is called
        """
        if self.rate < 8.0:
            self.rate = self.rate*2.0
            event = gst.event_new_seek(self.rate, gst.FORMAT_TIME,
                gst.SEEK_FLAG_FLUSH,
                gst.SEEK_TYPE_SET, self.query_position()[0],
                gst.SEEK_TYPE_NONE, 0)

            res = self.player.send_event(event)
            if res:
                gst.info("fast forwarding at rate: %f" % self.rate)
                self.player.set_new_stream_time(0L)
            else:
                gst.error("change rate to %f failed" % self.rate)
        return
    
    def slow_motion(self):
        """
            Here we will slow motion the stream for as many times
            as this is called
        """
        self.rate = self.rate/2.0
        event = gst.event_new_seek(self.rate, gst.FORMAT_TIME,
            gst.SEEK_FLAG_FLUSH,
            gst.SEEK_TYPE_SET, self.query_position()[0],
            gst.SEEK_TYPE_NONE, 0)

        res = self.player.send_event(event)
        if res:
            gst.info("slowing playback to rate: %f" % self.rate)
            self.player.set_new_stream_time(0L)
        else:
            gst.error("change rate to %f failed" % self.rate)

        return
        
    def get_rate(self):
        """
            Get the playing rate at the moment
        """
        return self.rate

    ## Set location.
    # Set location of the source.
    # \param location - URI of the source.
    def set_location(self, location):
        self.player.set_state(gst.STATE_NULL)
        self.player.set_property('uri', location)

    ## Set Subtitle Text
    # Set subtitle text to be overlayed.
    # \param text - Text (may have pango tags) 
    # \param font - Pango FontDescrition for the text
    def set_subtitle_text(self, text, font=None):
        if font:
            self.textoverlay.set_property('subtitle-font-desc', font)
        self.textoverlay.set_property('text', text)
        return

    ## Get location.
    # Get location of the source.
    def get_location(self):
        return self.player.get_property('uri')

    def query_position(self):
        "Returns a (position, duration) tuple"
        try:
            position, format = self.player.query_position(gst.FORMAT_TIME)
        except:
            position = gst.CLOCK_TIME_NONE

        try:
            duration, format = self.player.query_duration(gst.FORMAT_TIME)
        except:
            duration = gst.CLOCK_TIME_NONE

        return (position, duration)
        
    def query_frame(self, position):
        "Query the frame position"
        if position != gst.CLOCK_TIME_NONE:
            pad = self.videosink.get_pad('sink')
            caps = pad.get_negotiated_caps()
            if caps is not None:
                framerate = caps[0]['framerate']
                position = float(position)/float(1000000000)
                self.cur_frame = (float(position)*float(
                                    framerate.num))/float(framerate.denom)
        return self.cur_frame

    ## Seek.
    # Seek media.
    # \param location - location to the seek.
    def seek(self, location):
        gst.debug("seeking to %r" % location)
        event = gst.event_new_seek(self.rate, gst.FORMAT_TIME,
            gst.SEEK_FLAG_FLUSH,
            gst.SEEK_TYPE_SET, location,
            gst.SEEK_TYPE_NONE, 0)

        res = self.player.send_event(event)
        if res:
            gst.info("setting new stream time to 0")
            self.player.set_new_stream_time(0L)
        else:
            gst.error("seek to %r failed" % location)

    ## Pause.
    # Media pause.
    def pause(self):
        gst.info("pausing player")
        self.player.set_state(gst.STATE_PAUSED)
        self.playing = False

    ## Play.
    # Media play.
    def play(self):
        """
            Change the stream state to playing or simply
            change its playing rate to normal rate
        """
        if self.rate != 1.0:
            self.rate = 1.0
            event = gst.event_new_seek(self.rate, gst.FORMAT_TIME,
                gst.SEEK_FLAG_FLUSH,
                gst.SEEK_TYPE_SET, self.query_position()[0],
                gst.SEEK_TYPE_NONE, 0)

            res = self.player.send_event(event)
            if res:
                gst.info("slowing playback to rate: %f" % self.rate)
                self.player.set_new_stream_time(0L)
            else:
                gst.error("change rate to %f failed" % self.rate)           
        else:
            gst.info("playing player")
            self.player.set_state(gst.STATE_PLAYING)
            self.playing = True
        return

    ## Stop
    # Media stop.
    def stop(self):
        self.player.set_state(gst.STATE_READY)
        self.playing = False
        gst.info("stopped player")

    ## Get state.
    # Get current state of the media.
    # \param timeout - time out of the operation.
    # \raturn state of the media.
    def get_state(self, timeout=1):
        return self.player.get_state(timeout=timeout)

    ## Is playing
    # \return TRUE if media is playing.
    def is_playing(self):
        return self.playing

    ## \var playing
    # Bool variable, TRUE - if media is playing.

    ## \var player
    # GStreamer playerbin element.

    ## \var videowidget
    # GTK+ widget for video render.

#==============================================================================
## VideoWidget class.
# VideoWidget class for render video stream on GTK+ widget.
class VideoWidget:
    ## Constructor.
    # \param TArea - GTK+ drowing area widget.
    def __init__(self, TArea):
        self.Area=TArea
        self.imagesink = None
        self.Area.unset_flags(gtk.DOUBLE_BUFFERED)

    ## \var Area
    # GTK+ drowing area widget.

    ## \var imagesink
    # Sink element for 

    def do_expose_event(self, event):
        if self.imagesink:
            self.imagesink.expose()
            return False
        else:
            return True

    def set_sink(self, sink):
        assert self.Area.window.xid
        self.imagesink = sink
        self.imagesink.set_xwindow_id(self.Area.window.xid)
