function [trj3D,trj3D_Prior,theta,t] = poly2DTo3DOpt(trj2D,G,trj2D_Prior,G_Prior,order,NPast,verbose)
%% [trj3D,trj3D_Prior,theta,t] = poly2DTo3DOpt(trj2D,G,trj2D_Prior,G_Prior,order,NPast,verbose)
% ------------------------------------------
% FILE   : poly2DTo3DOpt.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-09-18  Created.
% ------------------------------------------
% PURPOSE
%   Convert 2D trajectory to 3D using a polynomial model between the SI and
%   the other two components.
%   This is the computationally optimized version for poly2DTo3D.m
% ------------------------------------------
% INPUT
%   trj2D:      The input 2D trajectory (N by 2). Positive x and y
%               corresponds to the [right,down] direction of the 2D image.
%   G:          The geometry matrices read from a RTK geometry file.
%               3 by 4 by N
%   trj2D_Prior:The 2D trajectory during the CBCT session as a prior to
%               initaite the model.
%   G_Prior:    The geometry matrices for trj2D_Prior
%   order:      The order of the polynomial model. Default: 2
%   NPast:      Number of past data points used to update the model.
%               Default is 100.
%   verbose:    Whether to display images for current progress.
%               Default is false.
%
% OUTPUT
%   trj3D:      The output 3D trajectory.
%   trj3D_Prior:The 3D trajectory during the CBCT session.
%   model:      The most updated polynomial model.
%   t:          The time to build the model and for each tracking frame.

%% Input check
if nargin < 4
    error('ERROR: trj2D, G, trj2D_Prior, G_Prior must be provided.');
end

if nargin < 5
    order = 2;
end

if nargin < 6
    NPast = 100;
end

if nargin < 7
    verbose = false;
end

convEps = 1e-6;
maxIter = 100;

%% Initiate model using trj2D_Prior and G_Prior
if verbose
    disp('Initiating the polynomial model ......');
end

t = zeros(size(trj2D,1)+1,1);

% Initial guess for theta is a linear model for all dimensions
theta = zeros((order+1)*3,1);
theta(order:3:end) = 1;
t = zeros(size(trj2D,1)+1,1);

if verbose; tic; end;
trj3D_Est = zeros(size(trj2D_Prior,1),3);
trj2D_Est = zeros(size(trj2D_Prior,1),2);
l_Est = zeros(size(trj2D_Prior,1),1);
eval3D(theta,trj2D_Prior,G_Prior);

[theta,mse_Prior] = conjGradOpt(@(x)objFunc(x,trj2D_Prior,G_Prior),...
    @(x)gradFunc(x,trj2D_Prior,G_Prior),@(x)hesFunc(x,trj2D_Prior,G_Prior),...
    theta,convEps,maxIter);
eval3D(theta,trj2D_Prior,G_Prior);
trj3D_Prior = trj3D_Est;
if verbose; t(1) = toc; end;

%% Track and continues to update model
trj3D = zeros(size(trj2D,1),3);
for k = 1:size(trj2D,1)
    if verbose
        disp(['Updating for frame#',num2str(k,'%05d'),' ......']);
    end
    if verbose; tic; end;
    if NPast > k
        trj2D_this = [trj2D_Prior(max(1,size(trj2D_Prior,1)-NPast+k+1):end,:);trj2D(1:k,:)];
        G_this = G_Prior(:,:,max(1,size(trj2D_Prior,1)-NPast+k+1):end);
        G_this(:,:,end+1:end+k) = G(:,:,1:k);
    else
        trj2D_this = trj2D(k-NPast+1:k,:);
        G_this = G(:,:,k-NPast+1:k);
    end
    
    trj3D_Est = zeros(size(trj2D_this,1),3);
    trj2D_Est = zeros(size(trj2D_this,1),2);
    l_Est = zeros(size(trj2D_this,1),1);
    eval3D(theta,trj2D_this,G_this);
    
    [theta,mse] = conjGradOpt(@(x)objFunc(x,trj2D_this,G_this),...
        @(x)gradFunc(x,trj2D_this,G_this),@(x)hesFunc(x,trj2D_this,G_this),...
        theta,convEps,maxIter);
    for dim = 1:3
        trj3D(k,dim) = polyval(theta((dim-1)*(order+1)+1:dim*(order+1)),trj2D_this(end,2));
    end
    if verbose; t(1+k) = toc; end;
end

    function eval3D(theta,trj2D,G)
        for fdim = 1:3
            trj3D_Est(:,fdim) = polyval(theta((fdim-1)*(order+1)+1:fdim*(order+1)),trj2D(:,2));
        end
        [trj2D_Est, l_Est] = rtk3DTo2D(G,trj3D_Est);
    end

    function result = objFunc(theta,trj2D,G)
        eval3D(theta,trj2D,G);
        result = 0.5 / size(trj2D,1) * sum((trj2D_Est(:) - trj2D(:)).^2);
    end

    function result = gradFunc(theta,trj2D,G)
        eval3D(theta,trj2D,G);
        result = zeros(size(theta));
        L = length(theta) / 3;
        for n = 1:size(trj2D,1)
            for i = 1:3
                for j = 1:L
                    result((i-1)*L+j) = result((i-1)*L+j) + ...
                        trj2D(n,2)^(L-j) / l_Est(n) * ...
                        ( (trj2D_Est(n,1) - trj2D(n,1)) * G(1,i,n) + ...
                        (trj2D_Est(n,2) - trj2D(n,2)) * G(2,i,n) );
                    %                     ( (trj2D_Est(n,1) - trj2D(n,1)) * (G(1,i,n) - trj2D_Est(n,1) * G(3,i,n)) + ...
                    %                       (trj2D_Est(n,2) - trj2D(n,2)) * (G(2,i,n) - trj2D_Est(n,2) * G(3,i,n)) );
                end
            end
        end
        result = result / size(trj2D,1);
    end

    function result = hesFunc(theta,trj2D,G)
        result = zeros(length(theta),length(theta));
        L = length(theta) / 3;
        for n = 1:size(trj2D,1)
            for i = 1:3
                for j = 1:L
                    for o = 1:3
                        for p = 1:L
                            result((i-1)*L+j,(o-1)*L+p) = result((i-1)*L+j,(o-1)*L+p) + ...
                                trj2D(n,2)^(2*L-j-p) / l_Est(n)^2 * ...
                                ( G(1,i,n) * G(1,o,n) + G(2,i,n) * G(2,o,n) );
                        end
                    end
                end
            end
        end
        result = result / size(trj2D,1);
    end

end