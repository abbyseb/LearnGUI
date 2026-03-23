function angles = ReadHncAnglesFromTextFiles(textFiles)
%% angles = ReadHncAnglesFromTextFiles(textFiles)
% ------------------------------------------
% FILE   : ReadHncAnglesFromTextFiles.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-05-16 Created.
% ------------------------------------------
% PURPOSE
%   Get a list of angles (within 0 to 360 degrees) from HNC header text files.
%
% ------------------------------------------
% INPUT
%   textFiles:      The list of text files in cell format.
% ------------------------------------------
% OUTPUT
%   angles:         The list of angles in degrees.

%%

for k = 1:length(textFiles)
    fid = fopen(textFiles{k},'r');
    line = fgetl(fid);
    while(line~=-1);
        if(strfind(line,'Projection Angle:'));
            line = regexp(line,'Projection Angle:','split');
            line = regexp(line{2},'deg','split');
            angles(k) = str2double(strtrim(line{1}));
            break;
        end;
        line = fgetl(fid);
    end;
    fclose(fid);
end;

angles = mod(angles,360);

return;