function binInfo = phase2bin(phase,N)
%% binInfo = phase2bin(phase,N)
% ------------------------------------------
% FILE   : signal2bin.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-11-25   Created
% ------------------------------------------
% PURPOSE
%   Convert a phase signal into the binInfo that's compatible to the input 
%   of the function "ProjSort".
%
% ------------------------------------------
% INPUT
%
%  phase
%   The phase signal (vector), which has to be between 0 and 1.
%
%  N (optional)
%   The number of bins.
%   (default: 10)

%% Check input
if ~isvector(phase)
    error('ERROR: The phase signal has to be a vector.');
else
    if ~all(phase>=0) || ~all(phase<=1)
        error('ERROR: The phase signal has to be between 0 and 1. Please normalize the phase signal if it ranges from 0 to 2pi');
    end;
end;

if nargin < 2
    N = 10;
else
    if ~isnumeric(N) || ~isscalar(N) || N<1 || mod(N,1)~=0
        error('ERROR: The number of bins "N" must be a positive integer.');
    end;
end;

%% Convert phase signal to binInfo

phase = mod(phase + 0.5/N,1);
binInfo = floor(phase*N) + 1;
binInfo(binInfo > N) = N;

return;