# -*- Mode: python -*-
#
# audiofile.py - audio file handling
#
# Copyright (C) 2003, Xiph.org Foundation
#
# This file is part of positron.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of a BSD-style license (see the COPYING file in the
# distribution).
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the license for more details.

from types import *
import os
from os import path
import string
import struct

import MP3Info

def detect(filename):
    metadata = None

    for detect_func in detect_functions:
        metadata = detect_func(filename)
        if metadata != None:
            break
    else:
        metadata = None

    if metadata != None and metadata["title"] == None:
        # Must have title
        metadata["title"] = path.split(filename)[1]

    return metadata

def detect_mp3(filename):
    try:
        f = file(filename, "rb")

        mp3info = MP3Info.MP3Info(f)

        header = mp3info.mpeg
        info = { "type" : "mp3",
                 "size" : header.filesize,
                 "length" : header.length,
                 "title" : mp3info.title,
                 "artist" : mp3info.artist,
                 "album" : mp3info.album,
                 "genre" : mp3info.genre,
                 "tracknumber" : mp3info.track}

        # Convert empty string entries to nulls
        for key in info.keys():
            if info[key] == "":
                info[key] = None

        f.close()

    except MP3Info.Error:
        f.close()
        return None
    except IOError:
        return None
    except OSError:
        return None            

    return info

def detect_oggvorbis(filename):
    try:
        vf = ogg.vorbis.VorbisFile(filename)
        vc = vf.comment()
        vi = vf.info()        

        info = { "type" : "oggvorbis",
                 "size" : os.stat(filename).st_size,
                 "length" : int(vf.time_total(-1)),
                 "title" : None,
                 "artist" : None,
                 "album" : None,
                 "genre" : None,
                 "tracknumber" : None}

        actual_keys = []
        for tag in vc.keys():
            try:
                actual_keys.append(tag.lower())
            except UnicodeError:
                pass  # Don't let bad tags stop us
            
        for tag in ("title","artist","album","genre","tracknumber"):
            if tag in actual_keys:
                try:
                    value = vc[tag]
                    # Force these to be single valued
                    if type(value) == ListType or type(value) == TupleType:
                        value = value[0]

                    # Convert from Unicode to ASCII since the Neuros can't
                    # do Unicode anyway.
                    #
                    # I will probably burn in i18n hell for this.
                    info[tag] = value.encode('ascii','replace')
                except UnicodeError:
                    pass

    except ogg.vorbis.VorbisError:
        return None
    except IOError:
        return None
    except OSError:
        return None

    return info

def detect_wav(filename):
    if filename[-4:] in ['.wav','.WAV','.Wav']:
        info = { "type" : "wav",
                 "size" : os.stat(filename).st_size,
                 "length" : 1,
                 "title" : None,
                 "artist" : None,
                 "album" : None,
                 "genre" : None,
                 "tracknumber" : None}
        wav_file=open(filename)
        wav_file.seek(0x04,0)
        size = int(struct.unpack('1i',wav_file.read(4))[0])
        wav_file.seek(0x1c,0)
        bytes_sec = int(struct.unpack('1i',wav_file.read(4))[0])
        wav_file.close()
        duration = size/bytes_sec
         
        info["length"] = duration
        return info
    else:
        return None

# Only put the ogg vorbis detection code in the list if
# we have the python module needed.

detect_functions = [detect_mp3,detect_wav]

try:
     import ogg.vorbis
     detect_functions.insert(0, detect_oggvorbis)
except ImportError:
    pass

