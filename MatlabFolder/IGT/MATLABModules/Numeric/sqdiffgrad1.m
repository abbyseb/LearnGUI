function result = sqdiffgrad1(s)
%% result = sqdiffgrad1(s)
% ------------------------------------------
% FILE   : sqdiffgrad1.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-05-30  Created.
% ------------------------------------------
% PURPOSE
%   Find the gradient of the squared difference of a 1D signal.
% ------------------------------------------
% INPUT
%   s:      The input 1D signal.
% ------------------------------------------
% OUTPUT
%   result: The gradient of the squared difference of a 1D signal.
% ------------------------------------------

%% Calculate
s = [s(1); s(:); s(end)];

term_i = s(2:end-1) - s(1:end-2);
term_ip1 = s(3:end) - s(2:end-1);

result = term_i - term_ip1;

return;