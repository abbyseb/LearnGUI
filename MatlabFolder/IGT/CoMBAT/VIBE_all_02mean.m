clear all
close all
clc

load organs
L=organs;

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

n=10;%number phases
TE=1.77; %echo time
TR=4.88; %repetition time
alpha=10*pi/180; %flip angle

%% T1, T2, M0 -> TrueFISP SIGNAL


% se non faccio la FAT SATURATION -> metti il grasso (#3) con T1:273 T2:16 T3:2377
T1_despot=[0 0 376 376 276 122 909 825 921 1032 506 1466 1500 376 588 753 753 753];
T2_despot=[0 0 30 30 13 8 28 28 40 20 30 52 20 30 16 36 36 36];
% M0_despot=[0 0 2377 1336 432 117 1138 3195 1972 1346 2023 1428 12766 1336 1100 1041 1041 1041]; %blood changed
M0_despot=[0 0 1336 1336 432 117 1138 3195 1972 1346 2023 1428 3526 1336 1100 1041 1041 1041]; %blood changed


T1=T1_despot;
T2=T2_despot;
M0=M0_despot;


for i=1:n
    [img imgInfo]=readmha(['MeanCycle/Phantom_' num2str(i,'%02i') '.mha']);
%     parametriMRI.T1{i,1}=zeros(size(img));
%     parametriMRI.T2{i,1}=zeros(size(img));
%     parametriMRI.M0{i,1}=zeros(size(img));
    S_VIBE{i,1}=zeros(size(img));

    disp(['bin: ' num2str(i)]);
    clear indici
    for k=1:length(L)
        indici=find(img(:)==L(k));
       
        
        S_VIBE{i,1}(indici)=(M0(k)*sin(alpha)*(1-exp(-TR/T1(k))))/(1-cos(alpha)*exp(-TR/T1(k)))*exp(TE/T2(k)); % FLASH (gradient echo) EQUATION!!!!!!!
        a=isnan(S_VIBE{i,1});
        f=find(a==1);
        S_VIBE{i,1}(f)=0;
        
    end        
end

S_VIBE_all=S_VIBE;

save S_VIBE_all S_VIBE_all -v7.3
% movefile('S_VIBE_all.mat','E:\MRXCAT_RTworkflow\P0101\FB\MotionModel\MeanCycle')

% for i=1:n
%     writemha(['4DMRI_T1/VIBE_' num2str(i,'%02i') '.mha'],S_VIBE{i,1},[0 0 0], [1 1 1], 'float');
% end

% figure;imshow(S_trueFISP{1,1}(:,:,100),[])









