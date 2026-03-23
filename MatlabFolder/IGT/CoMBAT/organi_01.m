clear all
close all
clc

% mkdir('organi');

[~, img]=MhaRead('sub_CT.mha');
organs=unique(img);
save organs organs

% for i=1:length(organs)
%     f=find(img==organs(i));
%     organo{i,1}=zeros(size(img));
%     organo{i,1}(f)=1;
% %     writemha(['organi\organo_' num2str(i) '.mha'],organo{i,1},imgInfo.Offset,imgInfo.ElementSpacing,'short');
% end

