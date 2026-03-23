function [D,uee,uzz] = diffcurv2(u)
%% [D,uee,uzz] = diffcurv2(u)
% ------------------------------------------
% FILE   : diffcurv2.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-08-01  Created.
% ------------------------------------------
% PURPOSE
%  Calculate the 2D difference curvature and uee & uzz described in Chen et
%  al. Image and Vision Computing (2010). If u is a 3D matrix, the function
%  treats the third dimension as the slice number and calculate for each
%  slice.
%   [D,uee,uzz] = diffcurv2(u)
% ------------------------------------------
% INPUT
%   u:      The input 2D/3D image.
% ------------------------------------------
% OUTPUT
%   D:          The difference curvature. Same dimension as u.
%   uee:        The partial derivative in the direction of grad(u).
%   uzz:        The partial derivative in the direction perpendicular to
%               grad(u).
% ------------------------------------------

%% Calculate

ux = partialD(u,1); uy = partialD(u,2);
uxy = 0.5 * (partialD(ux,2) + partialD(uy,1));
uxx = partialD(ux,1);   uyy = partialD(uy,2);

uee = (uxx.*ux.^2 + 2*ux.*uy.*uxy + uyy.*uy.^2) ./ (ux.^2 + uy.^2);
uzz = (uxx.*uy.^2 - 2*ux.*uy.*uxy + uyy.*ux.^2) ./ (ux.^2 + uy.^2);

D = abs(abs(uee) - abs(uzz));

D(isnan(D) | D==Inf | D==-Inf) = 0;
uee(isnan(uee) | uee==Inf | uee==-Inf) = 0;
uzz(isnan(uzz) | uzz==Inf | uzz==-Inf) = 0;

return;