%clear all;

%Reading inputs (U) and Outputs (Y) of the controlling system.
clear u;
clear y;

% Control inputs Frequecny (F) and Number of Cores(NC)
up1 = xlsread('mimotraining.clusterBig.ubench-freq-cores.xlsx','Sheet1','H2:H1201'); % F
up2 = xlsread('mimotraining.clusterBig.ubench-freq-cores.xlsx','Sheet1','N2:N1201'); % NC

% Control outputs Power (P) and instructions per second (IPS)
yp1 = xlsread('mimotraining.clusterBig.ubench-freq-cores.xlsx','Sheet1','G2:G1201'); % P
yp2 = xlsread('mimotraining.clusterBig.ubench-freq-cores.xlsx','Sheet1','D2:D1201'); % IPS

% scaling down large values to keep the priorities and sensetivity of the
% controller balanced in porportion to the values
up1 = up1./1000; 
yp2 = yp2./1000000000;

% Calculating means
mu1 = mean(up1(1:end-1))
mu2 = mean(up2(1:end-1))

my1 = mean(yp1(2:end))
my2 = mean(yp2(2:end))

% Shifting mean towards zero
u1 = up1 - mu1;
u2 = up2 - mu2;

y1 = yp1 - my1;
y2 = yp2 - my2;

min(u1);
max(u2);

min(u2);
max(u2);

min(y1);
max(y1);

min(y2);
max(y2);

% Merging both inputs to one variable
u(:,1) = u1;
u(:,2) = u2;

% Merging both outputs to one variable
y(:,1) = y1;
y(:,2) = y2;