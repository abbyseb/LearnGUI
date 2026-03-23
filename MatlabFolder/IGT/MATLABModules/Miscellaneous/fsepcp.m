function result = fsepcp
%% result = fsepcp
% ------------------------------------------
% FILE   : fsepcp.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-02-27   Creation
% ------------------------------------------
% PURPOSE
%   This gives the correct slash for file separator ("\" on Windows and "/"
%   on Linux). For cross platform computing.
%
%   result = fsepcp
% ------------------------------------------
% OUTPUT
%   result:     Just the slash, backwards or forwards.

%%

os = computer;

if strcmp(os(1:5),'PCWIN')
    result = '\';
elseif strcmp(os(1:4),'GLNX')
    result = '/';
else
    result = '\';
end

return;
