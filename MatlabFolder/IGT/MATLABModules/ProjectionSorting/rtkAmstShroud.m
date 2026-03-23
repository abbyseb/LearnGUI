function sig = rtkAmstShroud(projDir,rexp)
%% sig = rtkAmstShroud(projDir,rexp)
% ------------------------------------------
% FILE   : rtkAmstShroud.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-26  Created.
% ------------------------------------------
% PURPOSE
%   Using rtkamsterdamshroud and rtkextractshroudsignal to generate an
%   Amsterdam shroud respiratory binning signal.
% ------------------------------------------
% INPUT
%   projDir:    Path to the projection directory.
%   rexp:       Regular expression of the projection files (Optional).
% ------------------------------------------
% OUTPUT
%   sig:        The amsterdam shroud signal.

%% 

if nargin < 2
    rexp = '""';
else
    rexp = ['"',rexp,'"'];
end;

projDir = ['"',projDir,'"'];

imgFile = ['"',tempname,'.mha"'];
txtFile = ['"',tempname,'.txt"'];

% Generate amsterdam shroud image
fail = system(['rtkamsterdamshroud',...
    ' -p ',projDir,...
    ' -r ',rexp,...
    ' -o ',imgFile]);
if fail;    error('ERROR: "rtkamsterdamshroud" failed for unknown reason.');    end;

% Extract signal
fail = system(['rtkextractshroudsignal',...
    ' -i ',imgFile,...
    ' -o ',txtFile,...
    ' -a 40']);
if fail;    error('ERROR: "rtkextractshroudsignal" failed for unknown reason.');    end;
sig = textread(txtFile(2:end-1));

% Delete temporary files
if exist(imgFile,'file')
    delete(imgFile);
end;
if exist(txtFile,'file')
    delete(txtFile);
end;

return