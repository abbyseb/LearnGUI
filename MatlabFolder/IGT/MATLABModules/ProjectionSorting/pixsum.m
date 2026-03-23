function sig = pixsum(inputfl,corners)
%% sig = pixsum(inputfl,corners)
% ------------------------------------------
% FILE   : pixsum.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-30  Created.
% ------------------------------------------
% PURPOSE
%   Perform pixel summation method on the hnc file list and obtain the
%   phase in the order of projection files.
%   Supports only HNC format.
%
%   sig = pixsum(inputfl)
% ------------------------------------------
% INPUT
%   inputfl:    The list of hnc projection files.
%   corners:    The corners of the sum region. Default is the lower half
%               region. (Optional)
%               e.g. [x1,y1;x2,y2]
% ------------------------------------------
% OUTPUT
%   sig:        The signal obtained from the summing the pixels.

%% Get the peaks and troughs

nproj = size(inputfl,1);
[~,~,ext] = fileparts(inputfl(1,:));

if nproj == 1 && strcmpi(ext,'.mha')
    [~,M] = MhaRead(inputfl);
    nproj = size(M,3);
    sig = zeros(1,nproj);
    for k_proj = 1:nproj
        sig(k_proj) = sum(sum(M(1:end/2,end/2+1:end,k_proj)));
    end
else
    sig = zeros(1,nproj);
    
    for k_proj = 1:nproj
        [~,M] = ProjRead(inputfl(k_proj,:));
        if strcmpi(ext,'.hnc')
            M = log(65535./single(M));
            M(isnan(M) | abs(M)==Inf) = 0;
        elseif strcmpi(ext,'.hnd')
            M = log(139000./single(M));
            M(isnan(M) | abs(M)==Inf) = 0;
        elseif strcmpi(ext,'.his')
            M = log(65535./single(65535-M));
            M(isnan(M) | abs(M)==Inf) = 0;
        end
        % sig(k_proj) = sum(sum(M(end/4:3*end/4,end/4:3*end/4)));
        %sig(k_proj) = sum(sum(M(1:end/2,end/2+1:end)));
        if nargin < 2
            corners = [1, size(M,2)/2+1; size(M,1), size(M,2)];
        end
        sig(k_proj) = sum(sum(M(corners(1,1):corners(2,1),corners(1,2):corners(2,2))));
    end
end

return