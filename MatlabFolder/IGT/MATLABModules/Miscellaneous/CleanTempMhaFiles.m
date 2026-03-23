function CleanTempMhaFiles
%% CleanTempMhaFiles
% ------------------------------------------
% FILE   : CleanTempMhaFiles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-04
% ------------------------------------------
% PURPOSE
%   Delete all temporary .mha files in the temporary folder.
%
% ------------------------------------------

%% 

tempDir = fileparts(tempname);
fl = lscp(fullfile(tempDir,'*.mha'));
for k = 1:size(fl,1);
    fail = system(['del "',fl(k,:),'"']);
end;

return;