# Copyright (C) 2018  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org/licenses/>.

CMAKE_MINIMUM_REQUIRED (VERSION 3.2)

PROJECT (opendlv)

ADD_SUBDIRECTORY (opendlv-device-gps-ncom)
ADD_SUBDIRECTORY (opendlv-device-gps-nmea)
ADD_SUBDIRECTORY (opendlv-device-gps-pos)
ADD_SUBDIRECTORY (opendlv-device-lidar-vlp16)
ADD_SUBDIRECTORY (opendlv-device-lidar-hdl32e)

enable_testing()

