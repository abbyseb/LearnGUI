function Hnc2Att(hncList,outputDir)
%% Hnc2Att(hncList,outputDir)
% ------------------------------------------
% FILE   : Hnc2Att.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-02   Created.
% ------------------------------------------
% PURPOSE
%   Convert a set of Hnc files to Att files.
%
% ------------------------------------------
% INPUT
%   hncList:        The hnc file list obtained using "lscp".
%   outputDir:      The path to the output directory.

%%

HNCMAX = 65535;

if exist(outputDir,'dir') == 0
    mkdir(outputDir);
end

for k = 1:size(hncList,1)
    [info,M] = HncRead(hncList(k,:));
    M = single(M);  M(M==0) = 1;    M(M>HNCMAX) = HNCMAX;
    info.uiFileLength = 512 + 4 * numel(M);
    info.uiActualFileLength = info.uiFileLength;
    AttWrite(info,single(log(HNCMAX./M)),fullfile(outputDir,num2str(k,'Proj%05d.att')));
end;
