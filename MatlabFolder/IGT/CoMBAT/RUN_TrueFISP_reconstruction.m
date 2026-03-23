%% RUN RECONSTRUCTION
clear all
close all
clc


Files.data=0;%DO NOT CHANGE
Image_size.dim=0;%DO NOT CHANGE
load S_trueFISP_g; 
Files.image=S_trueFISP_g;
num_bin=1;%0; % TO CHANGE

TrueFISP_all_RECONSTRUCTION

S_trueFISP_g_recon=I_END;
save S_trueFISP_g_recon S_trueFISP_g_recon -v7.3
% k_space_cineMRI=k_space_end;
% save k_space_cineMRI k_space_cineMRI

% clear all
% close all
% clc
