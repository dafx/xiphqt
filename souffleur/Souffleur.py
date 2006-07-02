#!/usr/bin/env python

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#import oggStreams
from gstfile import GstFile
from GPlayer import VideoWidget
from GPlayer import GstPlayer
from Subtitles import Subtitles
from datetime import time
import sys

try:
    import pygtk
    #tell pyGTK, if possible, that we want GTKv2
    pygtk.require("2.0")
except:
    #Some distributions come with GTK2, but not pyGTK
    pass
try:
    import gtk
    import gobject
    import gtk.glade
except:
    print "You need to install pyGTK or GTKv2 ",
    print "or set your PYTHONPATH correctly."
    print "try: export PYTHONPATH=",
    print "/usr/local/lib/python2.2/site-packages/"
    sys.exit(1)
#now we have both gtk and gtk.glade imported
#Also, we know we are running GTK v2
import gst

class Souffleur:
#    gladefile=""
    def __init__(self):
        """
        In this init we are going to display the main
        Souffleur window
        """
        gladefile="souffleur.glade"
        windowname="MAIN_WINDOW"
        
        self.update_id = -1
        self.p_position = gst.CLOCK_TIME_NONE
        self.p_duration = gst.CLOCK_TIME_NONE
        self.UPDATE_INTERVAL=100
        
        self.Subtitle = None
        self.curSub = -1
        
        #self.videoWidget=VideoWidget();
        #gtk.glade.set_custom_handler(self.videoWidget, VideoWidget())

        #gtk.glade.set_custom_handler(self.custom_handler)
        self.wTree=gtk.glade.XML (gladefile,windowname)
        self.gladefile = gladefile
        # we only have two callbacks to register, but
        # you could register any number, or use a
        # special class that automatically
        # registers all callbacks. If you wanted to pass
        # an argument, you would use a tuple like this:
        # dic = { "on button1_clicked" : (self.button1_clicked, arg1,arg2) , ...
        #dic = { "on_button1_clicked" : self.button1_clicked, \
        #	"gtk_main_quit" : (gtk.mainquit) }
        dic = { "gtk_main_quit" : (gtk.main_quit),\
            "on_main_file_quit_activate": (gtk.main_quit), \
            "on_main_file_open_activate": self.mainFileOpen}
        self.wTree.signal_autoconnect (dic)
        
        self.windowFileOpen=None
        self.windowStreams=gtk.glade.XML (self.gladefile,"STREAM_WINDOW")
        ### Setup LIST_STREAMS
        LIST = self.windowStreams.get_widget("LIST_STREAMS")
        if LIST:
            self.streamsTreeStore = gtk.TreeStore(gobject.TYPE_STRING)
            LIST.set_model(self.streamsTreeStore)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Streams', cell, text = 0)
            LIST.append_column(tvcolumn)
        WND=self.windowStreams.get_widget("STREAM_WINDOW")
        WND.hide()
        ### Main window setup
        self.videoWidget = self.wTree.get_widget("VIDEO_OUT_PUT")
        self.adjustment = self.wTree.get_widget("MEDIA_ADJUSTMENT")
        self.SubEdit = self.wTree.get_widget("VIEW_SUB")
        return
#==============================================================================
    def mainFileOpen(self, widget):
        if(self.windowFileOpen==None):
            self.windowFileOpen=gtk.glade.XML (self.gladefile,"OPEN_OGG")
            dic={"on_OPEN_BUTTON_CANCEL_clicked": self.openFileCancel,\
                "on_OPEN_BUTTON_OPEN_clicked": self.openFileOpen }
            self.windowFileOpen.signal_autoconnect(dic)
#   	    WND=self.windowFileOpen.get_widget("OPEN_OGG")
#	        Filter=gtk.FileFilter()
#	        Filter.set_name("OGM file")
#   	    Filter.add_pattern("*.og[gm]")
#	        WND.add_filter(Filter)
        else:
            WND=self.windowFileOpen.get_widget("OPEN_OGG")
            if(WND==None):
                self.windowFileOpen=None
                self.mainFileOpen(widget)
            else:
                WND.show()
        return
#==============================================================================
    def openFileCancel(self, widget):
        if(self.windowFileOpen==None): return
        WND=self.windowFileOpen.get_widget("OPEN_OGG")
        WND.hide()
        return
#==============================================================================
    def openFileOpen(self, widget):
        WND=self.windowFileOpen.get_widget("OPEN_OGG")
        FN=WND.get_filename()
        Streams = None
        if((FN!="")and(FN!=None)):
#	        self.Streams=oggStreams.oggStreams(FN)
#   	    print FN
            Streams = GstFile(FN)
            if Streams:
                Streams.run()
        WND.hide()
        WND=self.windowStreams.get_widget("STREAM_WINDOW")
        WND.show()
        self.addStreams(Streams)
#   	self.refreshStreamsWindow()
        return
#==============================================================================
    def loadSubtitle(self, FileName):
        pass
#==============================================================================
    def addStreams(self, Streams):
        if not Streams:
            return
        iter = self.streamsTreeStore.append(None)
        self.streamsTreeStore.set(iter, 0, Streams.MIME + " " + Streams.SOURCE)
        for i in Streams.STREAMS.keys():
            child = self.streamsTreeStore.append(iter)
            self.streamsTreeStore.set(child, 0, i +" " + Streams.STREAMS[i])
            print i +" " + Streams.STREAMS[i]
        if "subtitle" in Streams.MIME:
            self.Subtitle = Subtitles()
            self.Subtitle.subLoad(Streams.SOURCE)
        else:
            self.videoWidgetGst=VideoWidget(self.videoWidget)
            self.player=GstPlayer(self.videoWidgetGst)
            self.player.set_location("file://"+Streams.SOURCE)
            if self.videoWidget.flags() & gtk.REALIZED:
                self.play_toggled()
            else:
                self.videoWidget.connect_after('realize',
                                           lambda *x: self.play_toggled())
        return
#==============================================================================
    def play_toggled(self):
        if self.player.is_playing():
            self.player.pause()
            #self.button.set_label(gtk.STOCK_MEDIA_PLAY)
        else:
            self.player.play()
            if self.update_id == -1:
                self.update_id = gobject.timeout_add(self.UPDATE_INTERVAL,
                                                     self.update_scale_cb)
            #self.button.set_label(gtk.STOCK_MEDIA_PAUSE)
#==============================================================================
    def update_scale_cb(self):
        had_duration = self.p_duration != gst.CLOCK_TIME_NONE
        self.p_position, self.p_duration = self.player.query_position()
        if self.Subtitle:
            tmSec= self.p_position/1000000
            MSec = tmSec%1000
            tmSec = tmSec/1000
            Sec = tmSec%60
            tmSec = tmSec/60
            Min = tmSec%60
            Hour=tmSec/60
            ST = time( Hour, Min, Sec, MSec )
            TText = self.Subtitle.getSub(ST)
            if TText:
                if (TText.N!=self.curSub):
                    BUF=gtk.TextBuffer()
                    BUF.set_text(TText.text)
                    self.SubEdit.set_buffer(BUF)
                    self.curSub=TText.N
            else:
                if (self.curSub!=-1):
                    BUF=gtk.TextBuffer()
                    BUF.set_text("")
                    self.SubEdit.set_buffer(BUF)
                    self.curSub=-1
        if self.p_position != gst.CLOCK_TIME_NONE:
            value = self.p_position * 100.0 / self.p_duration
            self.adjustment.set_value(value)
            #if not had_duration:
            #    self.cutin.set_time(0)
        return True
#==============================================================================
#	MAIN:
#==============================================================================
souffleur=Souffleur()
gtk.main()
