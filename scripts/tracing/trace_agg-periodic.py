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

import sys
import argparse
import pandas as pd
from pathlib import Path
import numpy as np
from functools import reduce

parser = argparse.ArgumentParser(description='Aggregates and parses traces')                   
parser.add_argument('--noresampling', action='store_true', help='Drop mismatched files instead of resampling')
parser.add_argument('--srcfiles', nargs='+',help='Source files to average and aggregate')
parser.add_argument('--destfile', help='Dest file')                  
args = parser.parse_args()


tracePowerCol = 'power_w'
traceFreqCol = 'freq_mhz'
traceCoreCol = 'core'
traceSampleIdCol = 'sample_id'
traceTimestampCol = 'timestamp'
# When down/up sampling traces, samples for these columns will be the
# weighted average of neighboring samples. Remaining columns will be the
# weighted sum
avgCols = [tracePowerCol,traceFreqCol,traceCoreCol]
# traces must have these columns
mustHaveCols = [traceSampleIdCol,traceTimestampCol] + avgCols

##########################################
### Helpers ##############################
def read_csv(path):
    mustHaveCols = avgCols
    df = pd.read_csv(path,sep=',',encoding='utf-8')
    dfcols = list(df)
    for col in mustHaveCols:
        if col not in dfcols:
            df = pd.read_csv(path,sep=';',encoding='utf-8')
            dfcols = list(df)
            break
    for col in mustHaveCols:
        assert(col in dfcols)
    return df

# It will crash for colums that are not number so we remove them
def cleanupDF(df):
    dfcols = list(df)
    for column in dfcols:
        try:
            df[column].mean()
        except:
            print('Ignoring column {}',column)
            df = df.drop(column, 1)
    return df

# Recalculate samples so dataframe has a specific number of rows
def resample(df,baselineRows):

    assert(len(df) is not baselineRows)

    # How many samples original dataframe has compared to the resampled
    # dataframe
    diff = (len(df) / baselineRows)

    # How resampling approach supports up-to 2x difference
    assert(diff <= 2)
    assert(diff >= 0.5)
    #print(diff)

    # Resamples by taking the weighted avg or sum of the samples that
    # 'align' with the current sample. For instance, whe downsampling
    # from 4 to 3, the first sample is 1*sample0 + 0.333*sample1, the
    # second sample is 0.666*sample1 + 0.666*sample2, and the third samples
    # is 0.333*sample2 + 1*sample3, where sample0,sample1... are samples
    # of the original dataframe.
    # In the reverse case, upsampling from 2 -> 3, would yield
    # 0.666*sample0, 0.3333*sample0 + 0.333 * sample1, and 0.666*sample2
    def _resample(col):
        col = col.astype(float)
        #print(col)
        #print(df[col.name])
        # Do weighted avg or weighted sum
        if col.name in avgCols:
            weighted = diff
        else:
            weighted = 1.0
        currLoc = 0.0
        for i in range(len(col)):
            _val = 0.0
            _currLoc = int(currLoc)
            _currLocWeight = 1 - (currLoc - _currLoc)
            _remaining = diff
            if _currLocWeight > _remaining: _currLocWeight = _remaining
            while _remaining > 1e-8: #rounding errs
                #print(currLoc,i,_currLoc,_remaining,_currLocWeight)
                #print('\t\t',_currLoc, _currLocWeight,df[col.name].iat[_currLoc])
                assert(_currLocWeight <= 1)
                assert(_currLocWeight > 0)
                _val += df[col.name].iat[_currLoc] * _currLocWeight
                _currLoc += 1
                _remaining -= _currLocWeight
                if _remaining > 1:
                    _currLocWeight = 1
                else:
                    _currLocWeight = _remaining

            col.iat[i] = _val / weighted

            #print('\t\t',col.iat[i])

            currLoc  = currLoc + diff

        return col



    # Creates a new DF with the target number of rows
    newDF = df.reindex(range(baselineRows)).bfill()
    newDF = newDF.apply(_resample)

    assert(len(newDF) == baselineRows)

    return newDF
##########################################
##########################################

#finds out the number of rows in each file
inputfiles = dict()
for f in args.srcfiles:
    df = read_csv(f)
    rows = len(df.index)
    if rows not in inputfiles:
        inputfiles[rows] = []
    inputfiles[rows].append(f)

_max = 0
baselineRows = 0
for key in inputfiles:
    cnt = len(inputfiles[key])
    if cnt > _max:
        _max = cnt
        baselineRows = key

assert(baselineRows>0)

print('{} out of {} files have {} rows. Other files will be upsampled/downsampled to {} rows'.format(len(inputfiles[baselineRows]),len(args.srcfiles),baselineRows,baselineRows))


baselineDfs = []
for key in inputfiles:
    for f in inputfiles[key]:
        if key is not baselineRows: continue

        df = read_csv(f)
        df = cleanupDF(df)

        assert(len(df) == key)

        #print('{} {}'.format(f,len(df.index)))
        df.reset_index(level=0, inplace=True)
        #print(df)
        baselineDfs.append(df)

baselineDf = pd.concat(baselineDfs).groupby('index').mean()

upDownSampledDfs = []
for key in inputfiles:
    for f in inputfiles[key]:
        if key is baselineRows: continue

        if args.noresampling:
            print('Droping file {} which has {} samples(noresampling flag set)'.format(f,key))
            continue

        diff = float('%.3f'%(abs(1-(key / baselineRows))))
        if diff > 0.05:
            print('Droping file {} which has too much deviation ({} samples / {}% diff)'.format(f,key,diff*100))
            continue

        df = read_csv(f)
        df = cleanupDF(df)

        assert(len(df) == key)

        #print('Down/upsampling {} {}->{} diff={}%'.format(f,key,baselineRows,diff*100))
        df = resample(df,baselineRows)

        #print('{} {}'.format(f,len(df.index)))

        df.reset_index(level=0, inplace=True)

        # These should be about the same for all dfs
        df[traceSampleIdCol] = baselineDf[traceSampleIdCol]
        df[traceTimestampCol] = baselineDf[traceTimestampCol]

        #print(df)

        #assert(len(df.index) is baselineRows)

        upDownSampledDfs.append(df)

finalDF = pd.concat(baselineDfs + upDownSampledDfs)
finalDF = finalDF.groupby('index').mean()
finalDF.to_csv(args.destfile, sep=';', encoding='utf-8',index=False)

print('{} out of {} files were used'.format(len(baselineDfs)+len(upDownSampledDfs),len(args.srcfiles)))

# save for db
#if(len(upDownSampledDfs) > 0):
#    baselineDf.to_csv(args.destfile+'.baseline.csv', sep=';', encoding='utf-8',index=False)
#    updownDf = pd.concat(upDownSampledDfs).groupby('index').mean()
#    updownDf.to_csv(args.destfile+'.updownsampled.csv', sep=';', encoding='utf-8',index=False)



