function ang = ReadElektaXMLAngles(frameFile)
%% ang = ReadElektaXMLAngles(frameFile)
% ------------------------------------------
% FILE   : ReadElektaXMLAngles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-09-03  Created.
% ------------------------------------------
% PURPOSE
%   Read gantry angles from Elekta _Frames.xml file.
% ------------------------------------------
% INPUT
%   frameFile:      Filepath to the _Frames.xml file.
%
% OUTPUT
%   ang:            The angle values in degree.
%
%%

tag = '<GantryAngle>';
ang = [];

fid = fopen(frameFile,'r');

tline = fgetl(fid);
while ischar(tline)
    tline = strtrim(tline);
    if length(tline) >= length(tag) && strcmpi(tline(1:length(tag)),tag)
        ind = regexp(tline,{'>','<'});
        ang(end+1) = str2double(tline(ind{1}(1)+1:ind{2}(end)-1));
    end
    tline = fgetl(fid);
end
fclose(fid);

return;