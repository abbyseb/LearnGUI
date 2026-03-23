function angles = HncAngles(hnclist)
%% angles = HncAngles(hnclist)
% ------------------------------------------
% FILE   : HncAngles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-04-10  Created
% ------------------------------------------
% PURPOSE
%   Get a list of angles (within 0 to 360 degrees) from a list of Varian
%   hnc files.
%
%   angles = HncAngles(hnclist)
% ------------------------------------------
% INPUT
%   hnclist:        The list of hnc file names.
% ------------------------------------------
% OUTPUT
%   angles:         The list of angles in degrees.

%% 
nfile = size(hnclist);  nfile = nfile(1);
angles = zeros(nfile,1);

for k = 1:nfile
    [info,M] = HncRead(hnclist(k,:));
    angles(k) = mod(info.dCTProjectionAngle,360);
end

return;