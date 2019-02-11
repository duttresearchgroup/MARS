% Reading data from the experiment traces

% Reading input (Frequency)
up = xlsread('mimotraining.clusterBig.ubench3.xlsx','variable','H4:H2604'); % F

% Reading the output variable (Power)
yp = xlsread('mimotraining.clusterBig.ubench3.xlsx','variable','G4:G2604'); % P

% Scaling down the input 
up = up./1000;

% Finding mean value for each variable
mu = mean(up(1:end-1))
my = mean(yp(2:end))

%Moving the data points towards zero by reducing mean value
u = up - mu;
y = yp - my;
