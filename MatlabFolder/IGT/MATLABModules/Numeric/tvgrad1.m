function result = tvgrad1(s,epsilon)
%% result = tvgrad1(s,epsilon)
% ------------------------------------------
% FILE   : tvgrad1.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-05-30  Created.
% ------------------------------------------
% PURPOSE
%   Find the gradient of the total variation norm of a 1D signal.
% ------------------------------------------
% INPUT
%   s:      The input 1D signal.
% ------------------------------------------
% OUTPUT
%   result: The gradient of 1D total variation.
% ------------------------------------------

%% Input check

if nargin < 2
    epsilon = eps;
end

%% Calculate
s = [s(1); s(:); s(end)];

term_i = s(2:end-1) - s(1:end-2);
term_ip1 = s(3:end) - s(2:end-1);

result = term_i ./ (abs(term_i) + epsilon) - term_ip1 ./ (abs(term_ip1) + epsilon);

return;