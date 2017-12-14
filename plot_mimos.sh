#!/bin/bash

for var in "$@"
do
    scripts/gem5_stats_parsing/plot_csv.py $var ";" timestamp bets_ref beats_filtered &
    scripts/gem5_stats_parsing/plot_csv.py $var ";" timestamp power_ref power_filtered &
done



