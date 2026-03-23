function result = tvgrad4(M,bwmask,epsilon)
%% result = tvgrad4(M,bwmask,epsilon)
% ------------------------------------------
% FILE   : tvgrad4.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-09   Created.
% ------------------------------------------
% PURPOSE
%   Find the 4D TV gradient.
% ------------------------------------------
% INPUT
%   M:         The input 4D image.
%   bwmask:    (Optional) A binary mask specifying the region within which
%              tv gradient will be calculated. Input 1 if tv gradient is to
%              be calculated for the entire image.
%              Default: tv gradient will be calculated for the entire image
%              region.
%   epsilon:   (Optional) A tiny number to prevent singular values. If not
%              input, the default is taken as "eps" defined in MATLAB.
% ------------------------------------------
% OUTPUT
%   result:    The gradient 4D matrix of the 4D total variation with respect to the image.
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

% If only a subregion is needed for calculation, truncate M
if ~isequal(bwmask,1)
    ind1D = find(bwmask);
    [ind4D(1),ind4D(2),ind4D(3),ind4D(4)] = ind2sub(dim,ind1D);
    xmin = max(min(ind4D(:,1)) - 1,1); xmax = min(max(ind4D(:,1)) + 1,dim(1));
    ymin = max(min(ind4D(:,2)) - 1,1); ymax = min(max(ind4D(:,2)) + 1,dim(2));
    zmin = max(min(ind4D(:,3)) - 1,1); zmax = min(max(ind4D(:,3)) + 1,dim(3));
    tmin = max(min(ind4D(:,4)) - 1,1); tmax = min(max(ind4D(:,4)) + 1,dim(4));
    M = M(xmin:xmax,ymin:ymax,zmin:zmax,tmin:tmax);
else
    xmin = 1; xmax = dim(1);
    ymin = 1; ymax = dim(2);
    zmin = 1; zmax = dim(3);
    tmin = 1; tmax = dim(4);
end;

if iscudacompatible && existsOnGPU(M)
    dx = diff(M,1,1);
    dy = diff(M,1,2);
    dz = diff(M,1,3);
    dt(:,:,:,2:tmax-tmin+1) = diff(M,1,4);
    dt(:,:,:,1) = M(:,:,:,1) - M(:,:,:,end);
    clear M;
    
    dfax = dx(1:end-1,2:end-1,2:end-1,:);
    dfay = dy(2:end-1,1:end-1,2:end-1,:);
    dfaz = dz(2:end-1,2:end-1,1:end-1,:);
    dfat = dt(2:end-1,2:end-1,2:end-1,:);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = ...
        (dfax + dfay + dfaz + dfat) ./ sqrt(dfax.^2 + dfay.^2 + dfaz.^2 + dfat.^2 + epsilon);
    clear dfax dfay dfaz dfat;
    
    dfbx = dx(2:end,2:end-1,2:end-1,:);
    dfby = dy(3:end,1:end-1,2:end-1,:);
    dfbz = dz(3:end,2:end-1,1:end-1,:);
    dfbt = dt(3:end,2:end-1,2:end-1,:);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax)...
        - dfbx ./ sqrt(dfbx.^2 + dfby.^2 + dfbz.^2 + dfbt.^2 + epsilon);
    clear dfbx dfby dfbz dfbt;
    
    dfcx = dx(1:end-1,3:end,2:end-1,:);
    dfcy = dy(2:end-1,2:end,2:end-1,:);
    dfcz = dz(2:end-1,3:end,1:end-1,:);
    dfct = dt(2:end-1,3:end,2:end-1,:);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax)...
        - dfcy ./ sqrt(dfcx.^2 + dfcy.^2 + dfcz.^2 + dfct.^2 + epsilon);
    clear dfcx dfcy dfcz dfct;
    
    dfdx = dx(1:end-1,2:end-1,3:end,:);
    dfdy = dy(2:end-1,1:end-1,3:end,:);
    dfdz = dz(2:end-1,2:end-1,2:end,:);
    dfdt = dt(2:end-1,2:end-1,3:end,:);
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax)...
        - dfdz ./ sqrt(dfdx.^2 + dfdy.^2 + dfdz.^2 + dfdt.^2 + epsilon);
    clear dfdx dfdy dfdz dfdt;
    
    dfex = dx(1:end-1,2:end-1,2:end-1,[2:tmax-tmin+1 1]);
    dfey = dy(2:end-1,1:end-1,2:end-1,[2:tmax-tmin+1 1]);
    dfez = dy(2:end-1,2:end-1,1:end-1,[2:tmax-tmin+1 1]);
    dfet = dt(2:end-1,2:end-1,2:end-1,[2:tmax-tmin+1 1]);
    clear dx dy dz;
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = ...
        result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax)...
        - dfet ./ sqrt(dfex.^2 + dfey.^2 + dfez.^2 + dfet.^2 + epsilon);
    clear dfex dfey dfez dfet;
else
    dx = diff(M,1,1);
    dfax = dx(1:end-1,2:end-1,2:end-1,:);
    dfbx = dx(2:end,2:end-1,2:end-1,:);
    dfcx = dx(1:end-1,3:end,2:end-1,:);
    dfdx = dx(1:end-1,2:end-1,3:end,:);
    dfex = dx(1:end-1,2:end-1,2:end-1,[2:tmax-tmin+1 1]);
    clear dx;
    
    dy = diff(M,1,2);
    dfay = dy(2:end-1,1:end-1,2:end-1,:);
    dfby = dy(3:end,1:end-1,2:end-1,:);
    dfcy = dy(2:end-1,2:end,2:end-1,:);
    dfdy = dy(2:end-1,1:end-1,3:end,:);
    dfey = dy(2:end-1,1:end-1,2:end-1,[2:tmax-tmin+1 1]);
    clear dy;
    
    dz = diff(M,1,3);
    dfaz = dz(2:end-1,2:end-1,1:end-1,:);
    dfbz = dz(3:end,2:end-1,1:end-1,:);
    dfcz = dz(2:end-1,3:end,1:end-1,:);
    dfdz = dz(2:end-1,2:end-1,2:end,:);
    dfez = dz(2:end-1,2:end-1,1:end-1,[2:tmax-tmin+1 1]);
    clear dz;
    
    dt(:,:,:,2:tmax-tmin+1) = diff(M,1,4);
    dt(:,:,:,1) = M(:,:,:,1) - M(:,:,:,end);
    dfat = dt(2:end-1,2:end-1,2:end-1,:);
    dfbt = dt(3:end,2:end-1,2:end-1,:);
    dfct = dt(2:end-1,3:end,2:end-1,:);
    dfdt = dt(2:end-1,2:end-1,3:end,:);
    dfet = dt(2:end-1,2:end-1,2:end-1,[2:tmax-tmin+1 1]);
    clear dt;
    
    result = single(zeros(dim));
    result(xmin+1:xmax-1,ymin+1:ymax-1,zmin+1:zmax-1,tmin:tmax) = (dfax + dfay + dfaz + dfat) ./ sqrt(dfax.^2 + dfay.^2 + dfaz.^2 + dfat.^2 + epsilon) ...
        - dfbx ./ sqrt(dfbx.^2 + dfby.^2 + dfbz.^2 + dfbt.^2 + epsilon) ...
        - dfcy ./ sqrt(dfcx.^2 + dfcy.^2 + dfcz.^2 + dfct.^2 + epsilon) ...
        - dfdz ./ sqrt(dfdx.^2 + dfdy.^2 + dfdz.^2 + dfdt.^2 + epsilon) ...
        - dfet ./ sqrt(dfex.^2 + dfey.^2 + dfez.^2 + dfet.^2 + epsilon);
end

return;