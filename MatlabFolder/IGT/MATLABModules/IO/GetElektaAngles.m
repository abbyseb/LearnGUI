function angles = GetElektaAngles(uid,imageFile,frameFile)
%% angles = GetElektaAngles(uid,imageFile,frameFile)
% ------------------------------------------
% FILE   : GetElektaAngles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-02   Created.
% ------------------------------------------
% PURPOSE
%   Get a list of angles for the elekta projection data.
%
% ------------------------------------------
% INPUT
%   uid:            The DICOM UID of the projection dataset (string).
%   imageFile:      Filepath to the image.dbf file.
%   frameFile:      Filepath to the frame.dbf file.
% ------------------------------------------
% OUTPUT
%   angles:         The list of angles in degrees.

%% 

txtFile = [tempname,'.txt'];

% Use rtkgetelektaangles to generate the text file
fail = system(['rtkgetelektaangles',...
    ' -i ','"',imageFile,'"',...
    ' -f ','"',frameFile,'"',...
    ' -u ','"',uid,'"',...
    ' -o ','"',txtFile,'"']);
if fail;    error('ERROR: "rtkgetelektaangles" failed for unknown reason.');    end;

% Read angles from text file
angles = textread(txtFile);
angles = angles * 180 / pi;

% Delete temporary files
delete(txtFile);

return