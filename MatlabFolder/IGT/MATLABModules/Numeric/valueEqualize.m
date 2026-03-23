function [sHandle,M_scaled] = valueEqualize(M,M_ref,option)
%% [sHandle,M_scaled] = valueEqualize(M,M_ref,option)
% ------------------------------------------
% FILE   : valueEqualize.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-07-04   Created.
% ------------------------------------------
% PURPOSE
%   Equalize the intensity using different options.
%
% ------------------------------------------
% INPUT
%
%  M
%   The input data to be scaled.
%
%  M_ref
%   The reference data to be scaled with respect to.
%
%  option
%   (Optional) Option of intensity equalization.
%   1. If user inputs a vector of two numbers >=0 and <=100, e.g. [10,90], 
%   then the percentile method will be used, with the two numbers being the
%   percentile used in the scaling.
%   2. If user inputs a single positive integer, then polyfit will be used
%   to scale the data (order N = input integer)
%   default: percentile method using [10,90]
%
% OUTPUT
%
%  sHandle
%   The function handle of the calculated scaling transform.
%
%  M_scaled
%   The scaled data.

%% Some simple input check and calculation

if nargin < 3
    option = [10,90];
end

if length(option) >= 2 && all(option >= 0 & option <= 100)
    % Percentile method
    for k = 1:length(option)
        a(k) = prctile(M(:),option(k));
        b(k) = prctile(M_ref(:),option(k));
    end;
    pcoef = polyfit(a,b,length(option)-1);
elseif isscalar(option) && mod(option,1)==0 && option > 0
    % Polyfit method
    if ~isequal(size(M),size(M_ref))
        error('ERROR: M and M_ref must have the same size if linear regression method is used.');
    end;
    pcoef = polyfit(M(:),M_ref(:),option);
else
    error('ERROR: Unsupported input of option.');
end

sHandle = @(x) polyval(pcoef,x);

if nargout > 1
    M_scaled = sHandle(M);
end

return;