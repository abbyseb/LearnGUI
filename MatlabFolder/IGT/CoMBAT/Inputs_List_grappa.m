
%all parameters required for simulation are listed here.
%parameters for simulated image

%______________________________________
%Flags details
%flag for deciding type of array
%type: integer
%value - 0: non-linear array
%value - 1: linear array
Flags.lin_flag=1;% may change for every simulation
%------------------------------------
%flag to select polynomial filtering
%type: integer
%to select, value =1;
Flags.poly_tick=0;

%_______________________________________
% Receiver  details
%--required for image data simulation--
%number of coils
%type: integer
Receiver.num_coils=8;
%always greater than subsampling_factor.
%------------------------------------
%factor determining coil localization
%type: integer
%value: 1 through 20
%if variance_factor=1, then coil is highly localised
%if variance_factor=20, coil is not very localised
Receiver.variance_factor=2;

%------------------------------------
%maximum value of the coil sensitivity. With 1,
% the sensitvity maps are like normalized
% type:rational
% value: 
Receiver.max_sensitivity=1;

%-----------------------------------
%Reference SNR: required when simulating any image data
% type:rational
% value:
Receiver.SNR=40;
%-----------------------------------

%____________________________________
%Sampling details
%------------------------------------
%reduction factor
%integer values
Sampling.subsampling_factor=2;
%always less than number of coils, num_coils
%------------------------------------
Sampling.direction='X'; %can be 'X' or 'Y'.

Sampling.center_locs=192-8:192+8;

%-----------------------------------
%number of lines (acquired+extra) for sensitivity estimation
%type: integer
Sampling.center_lines=length(Sampling.center_locs);
%____________________________________


save parameters;
%_______________________________________
