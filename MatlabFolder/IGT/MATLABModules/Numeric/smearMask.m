function result = smearMask(M,smearSize)
%% result = smearMask(M,smearSize)
% ------------------------------------------
% FILE   : smearMask.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-12-10   Created.
% ------------------------------------------
% PURPOSE
%   Smear the input logical map along all of its dimension. The number of
%   pixels that the map is smeared out with is specified in smearSize.
%   Support only logical input for now.
%
% ------------------------------------------
% INPUT
%
%  M
%   The input logical map. Can be any dimension.
%
%  smearSize
%   The input logical map will be smeared along the first dimension for
%   number of pixels equal to smearSize(1), second dimension for number of
%   pixels equal to smearSize(2), and so on. The number of entries in the
%   smearSize vector must match the number of dimension in M.
%
% OUTPUT
%
%  result
%   The resultant logical map.

%% Simply add up the logical map using OR
if ~islogical(M)
    error('ERROR: The input M must be in logical format.');
end;

dim = size(M);

if ~isnumeric(smearSize) || ~isvector(smearSize) || any(smearSize < 0) || any(mod(smearSize,1) ~= 0)
    error('ERROR: The input smearSize must be a vector with positive integer or zero values.');
end

if length(smearSize) ~= length(dim)
    error('ERROR: The dimension of the input logical map and the number of elements in smearSize must match.');
end

if any(smearSize(:)' > dim-1)
    error('ERROR: The smearSize value in each dimension cannot exceed the input logical map size in the corresponding dimension.');
end

result = M;

% Loop through all dimensions

for n_dim = 1:length(dim)
    % Need to initialize dimensional permutation vector first
    % We do this in the loop since p has to be reset to normal at the start
    % of every loop
    p = 1:length(dim);
    
    % The permutation vector needs to be changed such that the current
    % dimension is swapped to the first dimension
    p(1) = n_dim;   p(n_dim) = 1;
    result = permute(result,p);
    
    for k = 1 : smearSize(n_dim);
        result(1:end-k,:) = result(k+1:end,:) | result(1:end-k,:);
        result(k+1:end,:) = result(1:end-k,:) | result(k+1:end,:);
    end;
    
    % Permute back
    result = permute(result,p);
end;

return;