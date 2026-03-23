function angles = ProjAngles(projlist)
%% angles = ProjAngles(projlist)
% ------------------------------------------
% FILE   : ProjAngles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-04-10  Created
%          2013-11-15  Modified to support att format.
% ------------------------------------------
% PURPOSE
%   Get a list of angles (within 0 to 360 degrees) from a list of
%   projection files in hnc or att format.
%
% ------------------------------------------
% INPUT
%   projlist:       The list of hnc file names.
% ------------------------------------------
% OUTPUT
%   angles:         The list of angles in degrees.

%% 
nfile = size(projlist);  nfile = nfile(1);
angles = zeros(nfile,1);

for k = 1:nfile
    [~,~,ext] = fileparts(projlist(k,:));
    if strcmpi(ext(2:end),'hnc')
        info = HncRead(projlist(k,:));
     elseif strcmpi(ext(2:end),'hnd')
        info = HndRead(projlist(k,:));
    elseif strcmpi(ext(2:end),'att')
        info = AttRead(projlist(k,:));
    else
        error('Unsupported projection format.');
    end
    angles(k) = mod(info.dCTProjectionAngle,360);
end

return;