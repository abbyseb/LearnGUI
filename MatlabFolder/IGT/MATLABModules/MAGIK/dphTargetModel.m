function dphTargetModel(gtvExhPos,gtvInhPos,outputDir)
%% dphTargetModel(gtvExhPos,gtvInhPos,outputDir)
% ------------------------------------------
% FILE   : dphTargetModel.m
% AUTHOR : Andy Shieh, ACRF Image-X Institute, The University of Sydney
% DATE   : 2019-03-07 Created.
% ------------------------------------------
% PURPOSE
%   Compute and save the diaphragm-target model.
%
% ------------------------------------------
% INPUT
%   gtvExhPos:      1x3 vector of the GTV 3D position at end-exhale.
%   gtvInhPos:      1x3 vector of the GTV 3D position at end-inhale.
%   outputDir:      The output directory.

%% Linear regression
for dim = 1:3
    [model.LMCoef(dim,:),S] = ...
        polyfit([0;1],[gtvExhPos(dim);gtvInhPos(dim)],1);
    model.Normr(dim,1) = S.normr;
end

%% Write to file
if ~exist(outputDir,'dir')
    mkdir(outputDir);
end
save(fullfile(outputDir,'DphTargetModel.mat'));

fid = fopen(fullfile(outputDir,'DphTargetModel.txt'),'w');
fprintf(fid,'LMCoeff: %f, %f; %f, %f; %f, %f\n',...
    [model.LMCoef(1,1),model.LMCoef(1,2),model.LMCoef(2,1),model.LMCoef(2,2),model.LMCoef(3,1),model.LMCoef(3,2)]);
fclose(fid);