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

# Takes as input idle power measurements and a sys_info.json file
# and generates a new idle_power.json with per-core/per-frequency
# average idle power.
# To be used together with get_idle_power.sh
# Usage info:
#   python make_idle_power.py -h

import sys
import argparse
import pandas as pd
import json

parser = argparse.ArgumentParser(description='Generates an idle_power.json containing sys_info and average idle power information')             
parser.add_argument('idlepowerdir', help='Output directory produced when using the IdlePowerChecker system.')
args = parser.parse_args()

tracePowerCol = 'power_w'
traceFreqCol = 'freq_mhz'
traceSampleIdCol = 'sample_id'

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


# reads the platform description json file
sysInfo = json.load(open(sysInfoJson))

# we need a map of core_arch -> df with idle power per frequency
idlePowers = dict()


for fdm in sysInfo['freq_domain_list']:
    #read the power and freq domain idle measurements
    freqDf = read_csv('{}/idle_trace.freq_domain.{}.csv'.format(args.idlepowerdir,fdm['domain_id']),[traceFreqCol,traceSampleIdCol])
    freqDf[traceSampleIdCol] = freqDf[traceSampleIdCol].astype(int)
    freqDf[traceFreqCol] = freqDf[traceFreqCol].astype(int)

    idleDf = None

    for pdId in fdm['power_domains']:
        pdm = sysInfo['power_domain_list'][pdId]
        powerDf = read_csv('{}/idle_trace.power_domain.{}.csv'.format(args.idlepowerdir,pdm['domain_id']),[tracePowerCol,traceSampleIdCol])
        powerDf[traceSampleIdCol] = powerDf[traceSampleIdCol].astype(int)
        
        # all cores in the same domain are supposed to be idle
        # so we can make the power per core now
        powerDf[tracePowerCol] = powerDf[tracePowerCol] / pdm['core_cnt']
        
        #combine freq and pow measuments
        assert(len(powerDf) == len(freqDf))
        _idleDf = powerDf.merge(freqDf[[traceFreqCol,traceSampleIdCol]],on=traceSampleIdCol)
        _idleDf[traceFreqCol] = _idleDf[traceFreqCol].apply(lambda val: int(round(val, ildeTraceFreqRounding)))
        
        if idleDf is None:
            idleDf = _idleDf
        else:
            idleDf.append(_idleDf, ignore_index=True)

    #find the core arch
    coreArch = None
    for coreId in fdm['cores']:
        if coreArch is None:
            coreArch = sysInfo['core_list'][coreId]['arch']
        elif coreArch != sysInfo['core_list'][coreId]['arch']:
            print('WARNING: frequency domain {} has cores with different architectures. Idle power data will be wrong !'.format(fdm['domain_id']))
    
    if coreArch in idlePowers:
        idlePowers[coreArch].append(idleDf, ignore_index=True)
    else:
        idlePowers[coreArch] = idleDf

#group multiple measurements for same frequency
for coreArch in idlePowers:
    idlePowers[coreArch] = idlePowers[coreArch].groupby(traceFreqCol)[tracePowerCol].mean().reset_index()
    #print('For arch {}'.format(coreArch))
    #print(idlePowers[coreArch])
        
        
# add to the json and save it
sysInfo['idle_power'] = dict()
for coreArch in idlePowers:
    sysInfo['idle_power'][coreArch] = dict()
    for index, row in idlePowers[coreArch].iterrows():
        sysInfo['idle_power'][coreArch][int(row[traceFreqCol])] = row[tracePowerCol]

with open('{}.json'.format(args.idlepowerdir), 'w') as outfile:  
    json.dump(sysInfo, outfile, indent=4)

