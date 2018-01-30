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

import argparse
import pandas as pd
from pathlib import Path
import numpy

parser = argparse.ArgumentParser(description='Aggregates and parses traces')                   
parser.add_argument('--srcfiles', nargs='+',help='Source files to average and aggregate')
parser.add_argument('--destfile', help='Dest file')                  
args = parser.parse_args()


vals = dict()

for f in args.srcfiles:
    df = pd.read_csv(f,sep=';')
    #should have one row
    assert(len(df)==1)
    
    for column in df:
        try:
            #it will crash for colums that are not number so we ignore them
            df[column].mean()
            df[column].count()
            if column not in vals:
                vals[column] = (0,0)
            acc,cnt = vals[column]
            acc += df[column].mean()
            cnt += df[column].count()
            vals[column] = acc,cnt
        except:
            pass
        
avgVals = dict()
for column in vals:
    acc,cnt = vals[column]
    avgVals[column] = acc/cnt
    
with open(args.destfile, 'w') as text_file:
    for column in sorted(avgVals):
        text_file.write('{};'.format(column))
    text_file.write('\n')
    for column in sorted(avgVals):
        text_file.write('{};'.format(avgVals[column]))
    text_file.write('\n')
