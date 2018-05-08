#-------------------------------------------------------------------------------
# Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-------------------------------------------------------------------------------

# These commands will be running when performing the cloud build in VSTS agent

# Test build for ARM
cp arm.makefile.buildopts ../../makefile.buildopts
cd ../../
make clean
make runtime