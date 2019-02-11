%up = importdata('F_4_IPS_big_c.txt'); %Frequency
%yp = importdata('IPS_from_F_big_c.txt'); %Power

up = xlsread('mimotraining.clusterBig.ubench3.xlsx','variable','H4:H2604'); % F
yp = xlsread('mimotraining.clusterBig.ubench3.xlsx','variable','G4:G2604'); % P

up = up./1000;

mu = mean(up(1:end-1))
my = mean(yp(2:end))
u = up - mu;
y = yp - my;

% S = zeros(5,1);
% S(1) = sum(y(1:end-1).^2);
% S(2) = sum(u(1:end-1).*y(1:end-1));
% S(3) = sum(u(1:end-1).^2);
% S(4) = sum(y(1:end-1).*y(2:end));
% S(5) = sum(u(1:end-1).*y(2:end));
% 
% a = (S(3)*S(4)-S(2)*S(5))/(S(1)*S(3)-(S(2))^2);
% b = (S(1)*S(5)-S(2)*S(4))/(S(1)*S(3)-(S(2))^2);
% 
% subplot(1,3,1);
% yhat = a*y(1:end-1) + b*u(1:end-1);
% Rsq1 = rsquare(y(2:end),yhat)
% plot(y(2:end),yhat, ' * ',y,y,' -');
% title(Rsq1)
% 
% subplot(1,3,2);
% yhat2 = 0.9892*y(1:end-1) + 3.4964e+04*u(1:end-1);
% plot(y(2:end),yhat2, ' * ',y,y,' -');
% Rsq2 = rsquare(y(2:end),yhat2)
% title(Rsq2)
% 
% subplot(1,3,3);
% yhat3 = 0.06881*y(1:end-2) - 1.151*u(1:end-2) + 0.9237*y(2:end-1) + 1.147*u(2:end-1);
% plot(y(3:end),yhat3, ' * ',y,y,' -');
% Rsq3 = rsquare(y(3:end),yhat3)
% title(Rsq3)