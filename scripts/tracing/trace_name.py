#-------------------------------------------------------------------------------
# Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
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


# Obtains a canonical filename for a trace file that can be used as
# input for generating predictors and for the offline sim
# Produces an output in the format: app_name--core_arch--freq_mhz
# Takes various different options as parameters
# See trace_one-parse.sh for use

import sys
import os
import argparse
import pandas as pd
import json

parser = argparse.ArgumentParser(description='Gives canonical name for trace file: app_name--core_arch--freq_mhz.csv')             
parser.add_argument('--input_file', help='Tries to extract the program name, core, and freq from input file. Input name is in the format: app_name--core_id--freq_khz.csv', default='')
parser.add_argument('--input_core_id', default=-1 )
parser.add_argument('--input_freq_khz', default=-1 )
parser.add_argument('--app_name', default='' )
parser.add_argument('--output_core_arch', default='' )
parser.add_argument('--output_freq_mhz', default=-1 )
parser.add_argument('--sysinfo', default='' )
args = parser.parse_args()

app_name = ''
input_core_id = -1
input_freq_khz = -1
output_core_arch = ''
output_freq_mhz = -1

output_path = ''
output_ext = '.csv'

#first tries to parse --intput_file

if args.input_file is not '':
    output_path = os.path.dirname(args.input_file)
    splitedName = os.path.splitext(os.path.basename(args.input_file))[0].split('--')
    if len(splitedName) == 3:
        app_name = splitedName[0]
        input_core_id = int(splitedName[1])
        input_freq_khz = int(splitedName[2])
        output_ext = os.path.splitext(os.path.basename(args.input_file))[1]

#override with the other opts
if args.app_name is not '':
    app_name = args.app_name
if args.input_core_id is not -1:
    input_core_id = args.input_core_id
if args.input_freq_khz is not -1:
   input_freq_khz = args.input_freq_khz

if app_name is '':
    print('Invalid input app name. Use either a valid --input_file or override with --app_name')
    sys.exit(1)
if input_core_id is -1:
    print('Invalid input core id. Use either a valid --input_file or override with --input_core_id')
    sys.exit(1)
if input_freq_khz is '':
    print('Invalid input frequency. Use either a valid --input_file or override with --input_freq_khz')
    sys.exit(1)

#trie to read the sys info to get core_arch
if args.sysinfo is not '':
    sysInfo = json.load(open(args.sysinfo))
    assert(input_core_id < sysInfo['core_list_size'])
    assert(input_core_id == sysInfo['core_list'][input_core_id]['core_id'])
    output_core_arch = sysInfo['core_list'][input_core_id]['arch']

output_freq_mhz = input_freq_khz // 1000
#override with the other opts
if args.output_freq_mhz is not -1:
   output_freq_mhz = args.output_freq_mhz

if output_core_arch is '':
    print('Invalid input core arch. Use either a valid --sysinfo in combination wirh a valid --input_file or --input_core_id or override with --output_core_arch')
    sys.exit(1)
if output_freq_mhz is -1:
    print('Invalid output frequency. Use either a valid --input_file or --input_freq_khz or override with --output_freq_mhz')
    sys.exit(1)

output_name = '{}--{}--{}{}'.format(app_name,output_core_arch,output_freq_mhz,output_ext)
if output_path is not '':
    output_name = os.path.join(output_path,output_name)

print(output_name)


