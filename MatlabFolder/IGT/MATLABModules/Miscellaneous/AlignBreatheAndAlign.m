function [tDiff, sBToA, sAToB] = AlignBreatheAndAlign(tB,sB,tA,sA)
%% [tDiff, sBToA, sAToB] = AlignBreatheAndAlign(tB,sB,tA,sA)
% ------------------------------------------
% FILE   : AlignBreatheAndAlign.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-09-11  Created.
% ------------------------------------------
% PURPOSE
%   Align the Breath Well signal with the Align signal.
%
%   The get the time difference in seconds: tDiff
%
%   To plot the BreathWell signal resampled to match Align timestamps:
%   figure; plot(tA, sA, 'r'); % Align signal
%   hold on; plot(sBToA.SyncedTime, sBToA.BreatheWellSignal,'b'); % Breath Well signal
%
%   To plot the Align signal resampled to match BreathWell timestamps:
%   figure; plot(tB, sB, 'b');  % Breath Well signal
%   hold on; plot(sAToB.SyncedTime, sAToB.AlignSignal,'r'); % Align signal
% ------------------------------------------
% INPUT
%   tB:         Timestamps for the Breathe Well signal (in seconds, double).
%   sB:         The Breathe Well signal.
%   tA:         Timestamps for the Align signal (in seconds, double).
%   sA:         The Align signal.
%
% OUTPUT
%   tDiff:      Time difference between Breath Well and Align in seconds.
%               Positive means Breath Well is behind (needs to be shifted to the left).
%   sBToA:      BreathWell signal aligned to Align signal.
%   sAToB:      Align signal aligned to BreathWell signal.

%% 
% Resample Align signal to match Breath Well signal sampling frequency
tA_resamp = tB - tB(1) + tA(1);
sAToB.AlignSignal = interp1(tA, sA, tA_resamp);
tA_resamp(isnan(sAToB.AlignSignal)) = [];
sAToB.AlignSignal(isnan(sAToB.AlignSignal)) = [];

% Normalized cross correlation
nc = normxcorr2(sAToB.AlignSignal(:)', sB(:)');
ind = find(nc == max(nc));

% Find time difference
if ind < length(sAToB.AlignSignal)
    tDiff = tB(1) - tA_resamp(end-ind+1);
else
    tDiff = tB(ind-length(sAToB.AlignSignal)+1) - tA_resamp(1);
end

% Provide resampled version of both signals
sAToB.SyncedTime = tB - tB(1) + tA(1) + tDiff;
sAToB.SyncedTime = sAToB.SyncedTime(1:length(sAToB.AlignSignal));

sBToA.SyncedTime = tA - tA(1) + tB(1) - tDiff;
sBToA.BreatheWellSignal = interp1(tB, sB, tA - tA(1) + tB(1));
sBToA.SyncedTime(isnan(sBToA.BreatheWellSignal)) = [];
sBToA.BreatheWellSignal(isnan(sBToA.BreatheWellSignal)) = [];
