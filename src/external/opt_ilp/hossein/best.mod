
param NumTask, integer, >0;
param NumSpm, integer, >0 ;
param NumPage, integer, >0 ;    #I should change it to have a different page for different tasks

set TASK := 1..NumTask;
set PAGE := 1..NumPage;
set SPM := 1..NumSpm;

param capacity {SPM}, integer, >= 0;
param cost {TASK, SPM} >=0;
param access {TASK,PAGE}, integer, >=0;

var Map {TASK,PAGE, SPM} >=0;

minimize Total_Cost:
	sum {i in TASK, j in PAGE, k in SPM} Map[i, j, k] * cost[i, k] * access[i, j]; 
	
subject to unique {i in TASK, j in PAGE}:
	sum {k in SPM} Map[i, j, k] = 1; 
subject to cap {k in SPM}:
	sum {i in TASK, j in PAGE} Map [i, j, k] <= capacity [k]; 
	

