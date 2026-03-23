function His2Att(hisList,angles,outputDir)
%% His2Att(hisList,angles,outputDir)
% ------------------------------------------
% FILE   : His2Att.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-02   Created.
% ------------------------------------------
% PURPOSE
%   Convert a set of His files to Att files.
%
% ------------------------------------------
% INPUT
%   hisList:        The his file list obtained using "lscp".
%   angles:         The angular values.
%   outputDir:      The path to the output directory.

%%

HISMAX = 65535;

if ~isequal(size(hisList,1),length(angles))
    error('ERROR: The number of angle values does not match the number of his files.');
end;

if exist(outputDir,'dir') == 0
    mkdir(outputDir);
end

for k = 1:size(hisList,1)
    [info,M] = HisRead(hisList(k,:));
    info.dCTProjectionAngle = angles(k)+90;
    info.dCBCTPositiveAngle = mod(info.dCTProjectionAngle+270,360);
    M = single(M); M(M>=HISMAX) = HISMAX-1;
    AttWrite(info,single(log(HISMAX) - log(HISMAX-M)),fullfile(outputDir,num2str(k,'Proj%05d.att')));
end;
