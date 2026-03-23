function result = tv3(M,w)
%% tv3 = tv3(M)
% ------------------------------------------
% FILE   : tv3.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-01  Created.
% ------------------------------------------
% PURPOSE
%   Find the total variation norm of a 3D image according to the following
%   definition:
%   TV = sum(sqrt((f(x,y,z)-f(x-1,y,z))^2+(f(x,y,z)-f(x,y-1,z))^2+(f(x,y,z)-f(x,y,z-1))^2))
% ------------------------------------------
% INPUT
%   M:      The input 3D image.
%   w:      A weighted map if weighted tv is desired.
%           (default: w = 1)
% ------------------------------------------
% OUTPUT
%   result: The 3D total variation.
% ------------------------------------------

%% Calculate

if nargin < 2
    w = 1;
end;

Mdiff = (diff(M(:,2:end,2:end),1,1)).^2 + ...
    (diff(M(2:end,:,2:end),1,2)).^2 + ...
    (diff(M(2:end,2:end,:),1,3)).^2;
Mdiff = sqrt(Mdiff);
result = sum(w(:) .* Mdiff(:));

return;