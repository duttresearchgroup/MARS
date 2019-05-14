% Sample script to design a PI controller for controlling IPS of CPU
% cluster

Mp_t = 0.1;
ks_t = 5;
r_t = exp(-4/ks_t);
theta_t = pi*(log(r_t)/log(Mp_t));
% r = 0.4, theta = 1.0

% transfer function is extracted in system identification phase
big=tf(3.62, [1 0.1015],1);

% Discrete-time PI controller in parallel form.

[C_pi,info] = pidtune(big,'PI',1)

% Kp and Ki are represented at this point

% control = tf([0.1 0], [1 -1], -1) + 0.05;
PIC = feedback(big*C_pi,1);
clpoles = pole(PIC);
dcgain(PIC);

% Demonstrating Root Locus and Step response
[r,index] = max(abs(clpoles));
ks = -4/log(r)
subplot(1,2,1);
rlocus(PIC)
subplot(1,2,2);
step(PIC)

theta = angle(clpoles(index));
% check pole too close to zero
if abs(theta)< 0.00 mp =0;
% check for negative pole
elseif abs(theta - pi)< 0.001 mp = r;
% largest pole is complex
else mp = r^(pi/theta);
end
mp;

