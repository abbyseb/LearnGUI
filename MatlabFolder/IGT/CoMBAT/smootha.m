clear all
close all
clc

n=1;%0;
load S_trueFISP_all

for i=1:n
    S_trueFISP_g{i,1}=imgaussfilt(S_trueFISP_all{i,1},0.6);
end

save S_trueFISP_g S_trueFISP_g -v7.3


% n=10;
% load S_VIBE_all
% 
% for i=1:n
%     S_VIBE_g{i,1}=imgaussfilt(S_VIBE_all{i,1},0.6);
% end
% 
% save S_VIBE_g S_VIBE_g -v7.3