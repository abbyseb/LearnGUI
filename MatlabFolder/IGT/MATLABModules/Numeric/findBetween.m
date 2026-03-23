function result = findBetween(M)
%% result = findBetween(M)
% ------------------------------------------
% FILE   : findBetween.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-03-10   Created.
% ------------------------------------------
% PURPOSE
%   Find the region between the two lungs in the axial slices.
%   Note that the left-right direction needs to be in the x (the first
%   index) direction.
%   The logical map will be processed slice by slice if the input is a 3D
%   logical image.
%   Support only logical input for now.
%
% ------------------------------------------
% INPUT
%
%  M
%   The input logical map of the lungs.
%
% OUTPUT
%
%  result
%   The resultant logical map representing the region between the two lungs.

%% Simply scanning through each slice linearly

if ~islogical(M)
    error('ERROR: The input M must be in logical format.');
end;

dim = size(M);
result = false(dim);

for k = 1:dim(3)
    slice = M(:,:,k);
    if any(slice(:)) % only do non-empty slice
        for y = 1:dim(2);
            line_x = slice(:,y);
            
            if sum(line_x) >= 2
                ind = find(line_x == true);
                d_ind = diff(ind);
                r = find(d_ind == max(d_ind));
                result(ind(r):ind(r+1),y,k) = true;
            end
        end
    end
end

return;