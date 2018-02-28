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


# The 'power_w' has the total power consumed by the domain the tasks
# executed on. When tracing the other cores in the same domain are
# supposed to be idle, the 'power_w' includes the idle power of those
# cores.
# This script takes idle power measurements and deducts the idle power from 
# the 'power_w' in the traced file so it includes only the power consumed by
# the core used by the traced application.
# See scripts/training/get_idle_power.sh to obtain idle powers.
#
# Usage:
#   remove_idle_power.py --srcfile <src_trace_csv> --destfile <dest_trace_csv> --idlepowerdir <idle_power_dir>

import sys
import argparse
import pandas as pd
import json

parser = argparse.ArgumentParser(description='Ajusts the power_w column in a trace so it contains only the power of the used core')             
parser.add_argument('--srcfile', help='Source trace file')
parser.add_argument('--idlepowerdir', help='Output directory produced when using the IdlePowerChecker system')
parser.add_argument('--destfile', help='Where to save the result')                  
args = parser.parse_args()

tracePowerCol = 'power_w'
traceFreqCol = 'freq_mhz'
traceSampleIdCol = 'sample_id'
traceCoreCol = 'core'

#frequency is rounded to the nearest 100's of MHz (in the idle power trace) using the round function
ildeTraceFreqRounding = -2

sysInfoJson = '{}/sys_info.json'.format(args.idlepowerdir)

def read_csv(path,mustHaveCols):
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


# read trace
df = read_csv(args.srcfile,[tracePowerCol,traceFreqCol])

# gets the trace core
def getCore(df):
    currCore = -1
    coreSum = 0
    coreCnt = 0
    for index, row in df.iterrows():
        assert(row[traceCoreCol] != -1)
        if currCore == -1:
            currCore = row[traceCoreCol]
        else:
            if currCore != row[traceCoreCol]:
                print('{}: task execution on core {} detected, previously detected core  {}.'.format(args.srcfile,row[traceCoreCol],currCore))
                print('Tasks should be pinned to the same core while tracing !!!!!')
                currCore = row[traceCoreCol]
        coreSum += row[traceCoreCol]
        coreCnt += 1
    return int(round(coreSum/coreCnt))
traceCore = getCore(df)

# reads the platform description json file
sysInfo = json.load(open(sysInfoJson))

#get the trace core's power and freq domain
assert(traceCore < sysInfo['core_list_size'])
assert(traceCore == sysInfo['core_list'][traceCore]['core_id'])

traceFreqDomain = sysInfo['core_list'][traceCore]['freq_domain_id']
tracePowerDomain = sysInfo['core_list'][traceCore]['power_domain_id']
tracePowerDomainCoreCnt = sysInfo['power_domain_list'][tracePowerDomain]['core_cnt']

#read the power and freq domain idle measurements
powerDf = read_csv('{}/idle_trace.power_domain.{}.csv'.format(args.idlepowerdir,traceFreqDomain),[tracePowerCol,traceSampleIdCol])
freqDf = read_csv('{}/idle_trace.freq_domain.{}.csv'.format(args.idlepowerdir,tracePowerDomain),[traceFreqCol,traceSampleIdCol])
assert(len(powerDf) == len(freqDf))
powerDf[traceSampleIdCol] = powerDf[traceSampleIdCol].astype(int)
freqDf[traceSampleIdCol] = freqDf[traceSampleIdCol].astype(int)
freqDf[traceFreqCol] = freqDf[traceFreqCol].astype(int)
idleDf = powerDf.merge(freqDf[[traceFreqCol,traceSampleIdCol]],on=traceSampleIdCol)
idleDf[traceFreqCol] = idleDf[traceFreqCol].apply(lambda val: int(round(val, ildeTraceFreqRounding)))

#group multiple measurements for same frequency
idleDf = idleDf.groupby(traceFreqCol)[tracePowerCol].mean().reset_index()


#deduct power idle
def findPower(freqVal):
    global idleDf
    aux = idleDf.ix[(idleDf[traceFreqCol]-int(freqVal)).abs().argsort()[:2]]
    idleFreqClose = aux.iloc[0][traceFreqCol]
    idlePowerClose = aux.iloc[0][tracePowerCol]
    idleFreqFar = aux.iloc[1][traceFreqCol]
    idlePowerFar = aux.iloc[1][tracePowerCol]
    if int(freqVal) != idleFreqClose:
        print('{}: idle power for frequency {} not found. Interpolating'.format(args.srcfile,freqVal))
        closeFactor = 1 - (abs(idleFreqClose-freqVal) / (abs(idleFreqClose-freqVal)+abs(idleFreqFar-freqVal)))
        farFactor = 1 - (abs(idleFreqFar-freqVal) / (abs(idleFreqClose-freqVal)+abs(idleFreqFar-freqVal)))
        return (idlePowerClose*closeFactor) + (idlePowerFar*farFactor)
    else:
        return idlePowerClose

def ajustPower(powerVal,freqVal):
    global tracePowerDomainCoreCnt
    idlePower = findPower(freqVal)
    # all cores in the same domain are supposed to be idle
    # so deduct the power of those cores
    return powerVal - ((idlePower/tracePowerDomainCoreCnt)*(tracePowerDomainCoreCnt-1))

df[tracePowerCol] = df.apply(lambda row: ajustPower(row[tracePowerCol], row[traceFreqCol]), axis=1)

df.to_csv(args.destfile, sep=';', encoding='utf-8',index=False)


