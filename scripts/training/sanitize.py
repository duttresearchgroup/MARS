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

# To be used together with run_training*.sh scripts.
# The training app runs multiple ubenchs separated by a idle period. This results
# in a trace file with series of samples separated by idle periods, in which each
# series represents a trace for a single ubench. This script aggregates all the
# samples in each series into a single sample and generates a trace file where each
# sample corresponds to one ubench in the training application
#
# Usage:
#   sanitize.py --srcfile <src_trace_csv> --destfile <dest_trace_csv>

import sys
import argparse
import pandas as pd
from pathlib import Path
import numpy
from functools import reduce

parser = argparse.ArgumentParser(description='Generates a trace where each sample correspond to one ubench training combination')             
parser.add_argument('--srcfile', help='Source files to sanitize')
parser.add_argument('--destfile', help='Where to save the result')                  
args = parser.parse_args()


# Column used to search for boundaries between samples of different ubench combinations.
# There is an idle period between each combination, identified by samples with 0 instructions
# executed. We generate the aggregated sample for each combination by combining all samples
# between these idle periods. The value of the beatsTgt0 is used to identify which ubench 
# combination was executing.
# These columns must be present in the trace
checkerColId = 'beatsTgt0'
checkerColIntr = 'totalInstr'

# When aggregating we sumup the values of samples for most columns,
# except the ones listed here, which the mean value is taken.
# The average is a wighted mean of the value and the busy time
colsToAvg = ['freq_mhz','power_w',]

# These columns are handled separately and must all be present in the trace
# 'sample_id' is set by a separate counter
# 'timestamp' is set to the current sum of 'total_time_s'
# 'core' must be the same across all samples
specialCols = ['sample_id','timestamp','total_time_s','busy_time_s','core']

# Combined list of columns that should appear in the trace
reqCols = [checkerColId,checkerColIntr] + colsToAvg + specialCols


df = pd.read_csv(args.srcfile,sep=',',encoding='utf-8')
dfcols = list(df)
if len(dfcols) < len(reqCols):
    df = pd.read_csv(args.srcfile,sep=';',encoding='utf-8')
    dfcols = list(df)

#it will crash later for colums that are not number so we remove them now
for column in dfcols:
    try:            
        df[column].mean()       
    except:
        print('Ignoring column {}',column)
        df = df.drop(column, 1)

# These conditions are mandatory
assert(set(reqCols).issubset(dfcols))

currentId = 0
currentTimestamp = 0
currData = dict()
currentSampleCnt = 0
currCoreVal = -1

def checkDomains(row):
    global currCoreVal
    assert(row['core'] != -1)
    if currCoreVal == -1:
        currCoreVal = row['core']
    else:
        if currCoreVal != row['core']:
            print('{}: task execution on core {} detected, previously detected core  {}.'.format(args.srcfile,row['core'],currCoreVal))
            currCoreVal = row['core']

# Immediately Drop rows that have either beatsTgt or instr == 0
# Or have invalid data
def notNull(row):
    for col in dfcols:
        if pd.isnull(row[col]):
            return False
    if row[checkerColIntr] == 0:
        return False
    return True

def addRow(row):
    global currentSampleCnt
    global currentTimestamp
    global dfcols
    global currData
    global currentId

    if (currentId != 0) and (row[checkerColId] != 0) and (row[checkerColId] != currentId):
        commitRows()
        currentSampleCnt = 0
        currData = dict()
    
    if row[checkerColId] != 0:
        currentId = row[checkerColId]
    currentSampleCnt += 1
    for col in dfcols:
        val = row[col]
        if col in colsToAvg:
            val *= row['busy_time_s']
        if col not in currData:
            currData[col] = val
        else:
            currData[col] += val

    if currentSampleCnt > 1:
        checkDomains(row)
        
    
def commitRows():
    global currentSampleCnt
    global currentTimestamp
    global dfcols
    global currData
    global currentId
    global sanitized_df

    assert(currData['busy_time_s'] > 0)

    for col in dfcols:
        assert(col in currData)
        if col in colsToAvg:
            currData[col] /= currData['busy_time_s']

    assert(currentId > 0)
    assert(currCoreVal >= 0)
    assert(currentTimestamp >= 0)

    currentTimestamp += currData['total_time_s']

    currData['timestamp'] = currentTimestamp
    currData['sample_id'] = currentId
    currData['core'] = currCoreVal
    
    sanitized_df = sanitized_df.append(currData, ignore_index=True)
    

sanitized_df = pd.DataFrame(columns=dfcols)


for index, row in df.iterrows():
    if notNull(row):
        addRow(row)
        
if currentSampleCnt > 0:
    commitRows()

sanitized_df.to_csv(args.destfile, sep=';', encoding='utf-8',index=False)


