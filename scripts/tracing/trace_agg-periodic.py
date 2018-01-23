import sys
import argparse
import pandas as pd
from pathlib import Path
import numpy
from functools import reduce

parser = argparse.ArgumentParser(description='Aggregates and parses traces')                   
parser.add_argument('--srcfiles', nargs='+',help='Source files to average and aggregate')
parser.add_argument('--destfile', help='Dest file')                  
args = parser.parse_args()



#get only file with the same number of rows
_inputfiles = dict()
for f in args.srcfiles:
    df = pd.read_csv(f,sep=';')
    rows = len(df.index)
    if rows not in _inputfiles:
        _inputfiles[rows] = [] 
    _inputfiles[rows].append(f)

inputfiles = []
_max = 0
for key in _inputfiles:
    cnt = len(_inputfiles[key])
    if cnt > _max:
        _max = cnt
        inputfiles = _inputfiles[key]

if len(inputfiles)>1:
    print('{} out of {} files have the same number of rows'.format(len(inputfiles),len(args.srcfiles)))
else:
    print(_inputfiles)
    sys.exit('At least 2 input files have to have the same number of rows. Try increassing the number of tracing runs')



dfs = []
for f in inputfiles:
    df = pd.read_csv(f,sep=';')
    #it will crash later for colums that are not number so we remove them now
    dfcols = list(df)
    for column in dfcols:
        try:            
            df[column].mean()       
        except:
            print('Ignoring column {}',column)
            df = df.drop(column, 1)
    df.reset_index(level=0, inplace=True)
    dfs.append(df)


df = pd.concat(dfs)

df = df.groupby('index').mean()

df.to_csv(args.destfile, sep=';', encoding='utf-8',index=False)


