#!/usr/bin/env python
#
#       discoverer.py
#       
#       Copyright 2008 Joao Mesquita <jmesquita@gmail.com>
#       
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 3 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.

import os
from SubRip import SubRip

def discoverer(file):
    """
        This procedure will negotiate and return the proper subtitle class to
        handle the specific format. If it returns None, format is not yet
        supported.
    """
    extension = os.path.splitext(file)[1]
    if extension == ".srt":
        return SubRip(file)
    return None
