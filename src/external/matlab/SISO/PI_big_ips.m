Mp_t = 0.1;
ks_t = 5;
r_t = exp(-4/ks_t);
theta_t = pi*(log(r_t)/log(Mp_t));
% r = 0.4, theta = 1.0

%little=tf(2.1, [1 -.1834],1);
big=tf(3.62, [1 0.1015],1);

[C_pi,info] = pidtune(big,'PI',1)

%Kp = 0.452, Ki = 0.38

% control = tf([0.1 0], [1 -1], -1) + 0.05;
PIC = feedback(big*C_pi,1);
clpoles = pole(PIC);
dcgain(PIC);

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

