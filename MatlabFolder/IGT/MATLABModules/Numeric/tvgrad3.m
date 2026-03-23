function result = tvgrad3(M,bwmask,epsilon,cudatag)
%% result = tvgrad3(M,bwmask,epsilon,cudatag)
% ------------------------------------------
% FILE   : tvgrad3.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-02  Created.
%          2014-04-15  Include C version. CUDA version to be completed.
%          2014-05-12  Add "bwmask" to the input. However, the C version
%                      hasn't been modified to incorporate bwmask, yet. 
%          2014-05-26  Implement CUDA acceleration. However there seems to
%                      be a lot of room for improvement...
%          2014-06-09  Remove C option.
% ------------------------------------------
% PURPOSE
%   Find the gradient of the total variation norm with respect to the 3D image.
%   See tv3.m for the definition of the tv norm.
% ------------------------------------------
% INPUT
%   M:         The input 3D image.
%   bwmask:    (Optional) A binary mask specifying the region within which
%              tv gradient will be calculated. Input 1 if tv gradient is to
%              be calculated for the entire image.
%              Default: tv gradient will be calculated for the entire image
%              region.
%   epsilon:   (Optional) A tiny number to prevent singular values. If not
%              input, the default is taken as "eps" defined in MATLAB.
%   cudatag:   "true" if using gpu. Default is false.
% ------------------------------------------
% OUTPUT
%   result:    The gradient 3D matrix of the 3D total variation with respect to the image.
% ------------------------------------------

%% Calculate

dim = size(M);

if nargin < 2 || isequal(bwmask,1)
    bwmask = 1;
elseif ~isequal(size(bwmask),dim)
    error('ERROR: The input "bwmask" must have the same dimension as "M".');
end;

if nargin < 3
    epsilon = single(eps);
end;

if nargin < 4
    cudatag = false;
elseif ~islogical(cudatag)
    error('ERROR: "cudatag" must be a logical input.');
end

% If only a subregion is needed for calculation, truncate M
if ~isequal(bwmask,1)
    ind1D = find(bwmask);
    [ind3D(1),ind3D(2),ind3D(3)] = ind2sub(dim,ind1D);
    xmin = max(min(ind3D(:,1)) - 1,1); xmax = min(max(ind3D(:,1)) + 1,dim(1));
    ymin = max(min(ind3D(:,2)) - 1,1); ymax = min(max(ind3D(:,2)) + 1,dim(2));
    zmin = max(min(ind3D(:,3)) - 1,1); zmax = min(max(ind3D(:,3)) + 1,dim(3));
    M = M(xmin:xmax,ymin:ymax,zmin:zmax);
else
    xmin = 1; xmax = dim(1);
    ymin = 1; ymax = dim(2);
    zmin = 1; zmax = dim(3);
end;

if iscudacompatible && cudatag
    dx = diff(M,1,1);
    dy = diff(M,1,2);
    dz = diff(M,1,3);
    clear M;
    
    dfax = dx(1:end-1,2:end-1,2:end-1);
    dfay = dy(2:end-1,1:end-1,2:end-1);
    dfaz = dz(2:end-1,2:end-1,1:end-1);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1) = ...
        (dfax + dfay + dfaz) ./ sqrt(dfax.^2 + dfay.^2 + dfaz.^2 + epsilon);
    clear dfax dfay dfaz;
    
    dfbx = dx(2:end,2:end-1,2:end-1);
    dfby = dy(3:end,1:end-1,2:end-1);
    dfbz = dz(3:end,2:end-1,1:end-1);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1)...
        - dfbx ./ sqrt(dfbx.^2 + dfby.^2 + dfbz.^2 + epsilon);
    clear dfbx dfby dfbz;
    
    dfcx = dx(1:end-1,3:end,2:end-1);
    dfcy = dy(2:end-1,2:end,2:end-1);
    dfcz = dz(2:end-1,3:end,1:end-1);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1)...
        - dfcy ./ sqrt(dfcx.^2 + dfcy.^2 + dfcz.^2 + epsilon);
    clear dfcx dfcy dfcz;
    
    dfdx = dx(1:end-1,2:end-1,3:end);
    dfdy = dy(2:end-1,1:end-1,3:end);
    dfdz = dz(2:end-1,2:end-1,2:end);
    clear dx dy dz;
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1)...
        - dfdz ./ sqrt(dfdx.^2 + dfdy.^2 + dfdz.^2 + epsilon);
    clear dfdx dfdy dfdz;
else
    dx = diff(M,1,1);
    dfax = dx(1:end-1,2:end-1,2:end-1);
    dfbx = dx(2:end,2:end-1,2:end-1);
    dfcx = dx(1:end-1,3:end,2:end-1);
    dfdx = dx(1:end-1,2:end-1,3:end);
    clear dx;
    
    dy = diff(M,1,2);
    dfay = dy(2:end-1,1:end-1,2:end-1);
    dfby = dy(3:end,1:end-1,2:end-1);
    dfcy = dy(2:end-1,2:end,2:end-1);
    dfdy = dy(2:end-1,1:end-1,3:end);
    clear dy;
    
    dz = diff(M,1,3);
    dfaz = dz(2:end-1,2:end-1,1:end-1);
    dfbz = dz(3:end,2:end-1,1:end-1);
    dfcz = dz(2:end-1,3:end,1:end-1);
    dfdz = dz(2:end-1,2:end-1,2:end);
    clear dz;
    
    result = single(zeros(dim));
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1) = (dfax + dfay + dfaz) ./ sqrt(dfax.^2 + dfay.^2 + dfaz.^2 + epsilon) ...
        - dfbx ./ sqrt(dfbx.^2 + dfby.^2 + dfbz.^2 + epsilon) ...
        - dfcy ./ sqrt(dfcx.^2 + dfcy.^2 + dfcz.^2 + epsilon) ...
        - dfdz ./ sqrt(dfdx.^2 + dfdy.^2 + dfdz.^2 + epsilon);
end

return;