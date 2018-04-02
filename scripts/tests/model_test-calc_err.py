import glob
import argparse
import pandas as pd
import numpy as np

parser = argparse.ArgumentParser(description='Parses the output of model_test.sh')
parser.add_argument('outdir', help='Output directory produced when running model_test.sh')
parser.add_argument('--taskname', help='Task to look for')
args = parser.parse_args()

tgtTasksCSVs = glob.glob('{}/trace.pid*{}*.csv'.format(args.outdir,args.taskname))
otherTasksCSVs = list(set(glob.glob('{}/trace.pid*.csv'.format(args.outdir))).difference(set(tgtTasksCSVs)))
coresCSVs = glob.glob('{}/trace.core*.csv'.format(args.outdir))
fdsCSVs = glob.glob('{}/trace.fd*.csv'.format(args.outdir))
pdsCSVs = glob.glob('{}/trace.pd*.csv'.format(args.outdir))

columns = ['ipc', 'cpu', 'instr', 'busyTime', 'totalTime', 'load', 'freq', 'power']

def read_csv(path):
    df = pd.read_csv(path, sep=',', encoding='utf-8')
    if(len(list(df)) < 3):
        df = pd.read_csv(path, sep=';', encoding='utf-8')
    assert(len(list(df)) >= 3)
    return df

def computeErrors(fileList, fileType):
    print('Errors for {}'.format(fileType))
    for col in columns:
        maxErrFile = ''
        maxErr = 0.0
        avgErr = 0.0
        medErr = 0.0
        avgAggErr = 0.0
        cnt = 0
        for f in fileList:
            df = read_csv(f)
            sensedCol = '{}_sensed'.format(col)
            predCol = '{}_pred'.format(col)
            errCol = '{}_pred_err'.format(col)
            if (sensedCol not in list(df)) or (predCol not in list(df)):
                continue

            # remove first two samples
            df = df.iloc[2:]

            if 'instr' in sensedCol:
                # drop all rows with 0 instructions
                df = df[df[sensedCol] != 0]

            df[errCol] = ((df[predCol] - df[sensedCol])/df[sensedCol])*100
            df[errCol] = df[errCol].abs()
            df = df.replace([np.inf, -np.inf], np.nan).dropna(axis=0)

            # filter outliers
            q = df[errCol].quantile(0.99)
            df = df[df[errCol] < q]

            if len(df) == 0:
                continue

            if(df[errCol].mean() > maxErr):
                maxErr = df[errCol].mean()
                maxErrFile = f
            avgErr += df[errCol].mean()
            medErr += df[errCol].median()
            avgAggErr += abs(((df[predCol].sum() - df[sensedCol].sum())/df[sensedCol].sum())*100)
            cnt += 1

        if cnt > 0:
            print('\t{:10.10}\tmean={}\tmeanAgg={}\tmed={}\tmaxMean={}\tmaxMeanFile= {}'.format(col,int(avgErr/cnt),int(avgAggErr/cnt),int(medErr/cnt),int(maxErr),maxErrFile))

computeErrors(coresCSVs,'cores')
computeErrors(fdsCSVs,'freq domains')
computeErrors(pdsCSVs,'power domains')
computeErrors(tgtTasksCSVs,'{} tasks'.format(args.taskname))
computeErrors(otherTasksCSVs,'Other tasks')
            
            
            
            
