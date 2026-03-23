function result = tv1(s)
%% result = tv1(s)
% ------------------------------------------
% FILE   : tv1.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-05-30  Created.
% ------------------------------------------
% PURPOSE
%   Find the total variation norm of a 1D signal.
% ------------------------------------------
% INPUT
%   s:      The input 1D signal.
% ------------------------------------------
% OUTPUT
%   result: The 1D total variation.
% ------------------------------------------

%% Calculate

result = sum(abs(diff(s)));

return;