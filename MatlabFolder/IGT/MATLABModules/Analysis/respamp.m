function [meanamp, rmsamp, meanbase, rmsbase, meanT, rmsT] = respamp(d,mpd)
%% [meanamp, rmsamp, meanbase, rmsbase, meanT, rmsT] = respamp(d,mpd)
% ------------------------------------------
% FILE   : respamp.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-05-20  Created.
% ------------------------------------------
% PURPOSE
%   Calculate the mean and rms of respiratory amplitude, baseline position,
%   and period.
%
%   [meanamp, rmsamp, meanbase, rmsbase, meanT, rmsT] = respamp(d,mpd)
% ------------------------------------------
% INPUT
%   d:          The displacement profile.
%   mpd:        (Optional) The minimum peak distance. Specifying this would
%               help the code find the peaks in displacement profile more correctly.
% ------------------------------------------
% OUTPUT
%   meanamp:    The mean amplitude (peak to trough) of the respiratory
%               cycle.
%   rmsamp:     The RMS of the amplitudes.
%`  meanbase:   The mean of the basline positions (doesn't mean much though)
%   rmsbase:    The RMS of baseline positions.
%   meanT:      The mean period in the unit of timeframes.
%   rmsR:       The RMS of periods in the unit of timeframes.

%% Get the peaks and troughs

SMOOTHING = 1600;
fpeak = 0;      % A parameter for finding proper peak separation

if nargin < 2
    mpd = 3;
end
mpd = round(mpd);
if mpd < 1
    mpd = 1;
end

nproj = length(d);

[trend,res] = hpfilter(d,SMOOTHING);
% Get a first estimation of the peaks and troughs
[pv,pp] = findpeaks(res,'MINPEAKDISTANCE',mpd);
[nv,np] = findpeaks(-res,'MINPEAKDISTANCE',mpd);  nv = -nv;
% Get a better estimation using the preliminary results
[pv,pp] = findpeaks(res,'MINPEAKDISTANCE',max(round(mean(diff(pp))-fpeak*std(diff(pp))),mpd));
[nv,np] = findpeaks(-res,'MINPEAKDISTANCE',max(round(mean(diff(np))-fpeak*std(diff(np))),mpd));
nv = -nv;

% Make sure we have peak and trough alternating
kp1 = 1; kp2 = 2;
while kp2 <= length(np)
    if sum(pp > np(kp1) & pp < np(kp2)) == 0
        np(kp2) = [];
        nv(kp2) = [];
    elseif sum(pp > np(kp1) & pp < np(kp2)) >= 1
        kp1 = kp2;  kp2 = kp2 + 1;
    end
end

% Calculate the amplitudes, baseline positions and periods
ampvec = zeros(length(np)-1,1);
basevec = zeros(length(np)-1,1);
for k = 1:length(np)-1
    pp_tmp = pp(pp > np(k) & pp < np(k+1));
    pv_tmp = res(pp_tmp);
    ampvec(k) = max(pv_tmp) - nv(k);
    basevec(k) = 0.5 * (max(pv_tmp) + nv(k));
end
periodvec = diff(np);

meanamp = mean(ampvec);
rmsamp = std(ampvec);
meanbase = mean(basevec);
rmsbase = std(basevec);
meanT = mean(periodvec);
rmsT = std(periodvec);
    
return