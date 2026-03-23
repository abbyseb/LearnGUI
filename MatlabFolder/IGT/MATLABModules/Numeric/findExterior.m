function result = findExterior(M,c)
%% result = findExterior(M)
% ------------------------------------------
% FILE   : findExterior.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-12-10   Created.
% ------------------------------------------
% PURPOSE
%   Given a logical map of an "interior region", this function outputs the
%   exterior region, excluding any hollowed part that's surrounded by the
%   interior region. The logical map will be processed slice by slice if
%   the input is a 3D logical image.
%   Support only logical input for now.
%
% ------------------------------------------
% INPUT
%
%  M
%   The input logical map.
%
%  c
%   If within c pixels, there are no interior pixles found, the first pixel
%   will be identified as the edge.
%   (default: 1)
%
% OUTPUT
%
%  result
%   The resultant logical map representing the exterior region.

%% Simply scanning through each slice linearly

if ~islogical(M)
    error('ERROR: The input M must be in logical format.');
end;

if nargin < 2
    c = 1;
end

if c<=0 | mod(c,1)~=0
    error('ERROR: The input c must be an integer.');
end

dim = size(M);

result1 = M;
result2 = M;

% Do this vertically and horizontally, and find the united region
for k = 1:dim(3)
    slice = M(:,:,k);
    if any(slice(:)) % only do non-empty slice
        
        for x = 1:dim(1);
            line_y = slice(x,:);
            ind_y = line_y(1:end-c+1);
            for r = 2:c
                ind_y = ind_y | line_y(r:end-c+r);
            end;
            endpt_y = find(ind_y == false);

            if ~isempty(endpt_y)
                result1(x,endpt_y(1):endpt_y(end)+c-1,k) = false;
            else
                result1(x,:,k) = true;
            end
        end
        
        for y = 1:dim(2);
            line_x = slice(:,y);
            ind_x = line_x(1:end-c+1);
            for r = 2:c
                ind_x = ind_x | line_x(r:end-c+r);
            end;
            endpt_x = find(ind_x == false);

            if ~isempty(endpt_x)
                result2(endpt_x(1):endpt_x(end)+c-1,y,k) = false;
            else
                result2(:,y,k) = true;
            end
        end
    end
end

result = result1 | result2;

return;