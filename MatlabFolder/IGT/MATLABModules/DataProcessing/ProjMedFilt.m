function ProjMedFilt(inputfl,outputdir,fsize)
%% ProjMedFilt(inputfl,outputdir,fsize)
% ------------------------------------------
% FILE   : ProjMedFilt.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-20  Created
%          2013-11-15  Modified to support att format.
% ------------------------------------------
% PURPOSE
%   Apply median filter to all the hnc files in the file list and save to
%   the output directory.
%
%   ProjMedFilt(inputfl,outputdir)
%   ProjMedFilt(inputfl,outputdir,fsize)
% ------------------------------------------
% INPUT
%   inputfl:        The input file list (can be obtained using lscp('*.hnc'))
%   outputdir:      The output directory.
%   fsize:          The size of the median filter (e.g. [3,3]), a 1*2
%                   vector. Default is [3,3]. (Optional)
% ------------------------------------------

%% 
if nargin < 3
    fsize = [3,3];
end

nproj = size(inputfl);  nproj = nproj(1);

for k = 1:nproj
    [info,M] = ProjRead(inputfl(k,:));
    [~,name,ext] = fileparts(inputfl(k,:));
    ProjWrite(info,medfilt2(M,fsize),fullfile(outputdir,[name,ext]));
end

return;