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

# Compares two CSV and prints mean difference in the values of each column
# Usage:
#   compare_csvs.py <csv0> <csv1>

import sys
import argparse
import pandas as pd
from pathlib import Path
import numpy as np
from functools import reduce

parser = argparse.ArgumentParser(description='Compares two csv files')             
parser.add_argument('csv0', type=str, help='First CSV to compare')
parser.add_argument('csv1', type=str, help='Second CSV to compare')
args = parser.parse_args()

csv0 = pd.read_csv(args.csv0,sep=',',encoding='utf-8')
if len(list(csv0)) < 2:
    csv0 = pd.read_csv(args.csv0,sep=';',encoding='utf-8')
    
csv1 = pd.read_csv(args.csv1,sep=',',encoding='utf-8')
if len(list(csv1)) < 2:
    csv1 = pd.read_csv(args.csv1,sep=';',encoding='utf-8')
    

rowsDiff = len(csv0) != len(csv1)

if rowsDiff:
    print('CSV have a different number of rows')
    print('Rows will be aggregated to compare')
    csv0 = csv0.sum()
    csv1 = csv1.sum()

csvDiff = ((csv1 - csv0) / csv0)*100
csvDiff = csvDiff.replace([np.inf, -np.inf], np.nan).dropna(how="all")

pd.options.display.float_format = '{:.2f}'.format
if rowsDiff:
    print(csvDiff)
else:
    print(csvDiff.mean())
