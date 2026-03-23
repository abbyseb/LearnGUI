clear all
close all
clc

load organs
L=organs;

% These are the organs present in the XCAT phantom
% 1: background
% 2: air lung/bowel
% 3: fat
% 4: body (water) 
% 5: red marrow
% 6: bowel
% 7: pancreas
% 8: muscle, lesion
% 9: kydney
% 10: heart
% 11: liver
% 12: spleen
% 13: blood
% 14: thyroid --- body
% 15: cartilage
% 16: spine bone
% 17: skull
% 18: rib bone

% parameters to adapt
n=1;%0;%number of respiratory phases
TE=1.26; %echo time
alpha=68*pi/180; %flip angle

%% T1, T2, M0 -> TrueFISP SIGNAL

T1_despot=[0 0 376 376 276 122 909 825 921 1032 506 1466 1500 376 588 753 753 753];
T2_despot=[0 0 30 30 13 8 28 28 40 20 30 52 20 30 16 36 36 36];
M0_despot=[0 0 1336 1336 432 117 1138 3195 1972 1346 2023 1428 12766 1336 1100 1041 1041 1041]; %blood changed

T1=T1_despot;
T2=T2_despot;
M0=M0_despot;

ct_list = dir('*.mha');%('MeanCycle/*.mha');


for i=1:n
    [info,img]=MhaRead(fullfile(ct_list(i).folder, ct_list(i).name));

    S_trueFISP{i,1}=zeros(size(img));

    disp(['bin: ' num2str(i)]);
    clear indici
    for k=1:length(L)
        indici=find(img(:)==L(k));

        S_trueFISP{i,1}(indici)=(M0(k)*sin(alpha))/((T1(k)/T2(k)+1)-cos(alpha)*(T1(k)/T2(k)-1))*exp(-TE/T2(k)); % TrueFISP EQUATION!!!!!!!
              
        a=isnan(S_trueFISP{i,1});
        f=find(a==1);
        S_trueFISP{i,1}(f)=0;
    end        
end

S_trueFISP_all=S_trueFISP;

save S_trueFISP_all S_trueFISP_all -v7.3

%for i=1:n
%    MhaWrite(info,S_trueFISP_all{i,1},['MeanCycle/S_trueFISP_' num2str(i,'%02i') '.mha'])
%end










