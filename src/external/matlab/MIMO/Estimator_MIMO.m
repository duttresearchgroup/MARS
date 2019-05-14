%%%%%%%%% Estimator design %%%%%%%%%%

% ss7 is just one state space example that is extracted from system identification phase. 
% number of outputs
ny = 2;

% number of inputs
nx = 7;

% augmented model:
sys_augss = ss(ss7.A,[ss7.B ss7.K],ss7.C,[ss7.D eye(ny)],ss7.Ts);
%sys_augss = ss(ss_big_ips_variable.A,[ss_big_ips_variable.B ss_big_ips_variable.K],ss_big_ips_variable.C,[ss_big_ips_variable.D eye(ny)],ss_big_ips_variable.Ts);

% noise variance of the model
Qn = ss7.NoiseVariance;

% Sensor noise. assigned to zero. alternative can be Rn = 1e-6*eye(ny)
Rn = zeros(ny);

% estimator build. 
Kest = kalman(sys_augss, Qn, Rn); 

%%%%%%%%% Optimal trakcer and LQG controller design %%%%%%%%%%
% extract the deterministic part of the model
sys_detss = ss(ss7.A,ss7.B,ss7.C,ss7.D,ss7.Ts);

Q = [1 0;  % IPS
     0  1]; % P 
R = [1 0; % NC
     0 10]; % Freq
%100*eye(2);
%R = eye(2);
Qa = blkdiag(0*eye(nx),Q);

% optimal tracker:
Ktrk = lqi(sys_detss, Qa, R);

% lqg optimal controller
Klqg_mimo2x2 = lqgtrack(Kest,Ktrk,'1dof');

