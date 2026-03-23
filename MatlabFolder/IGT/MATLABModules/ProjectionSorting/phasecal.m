function phase = phasecal(d,mpd)
%%phase = phasecal(d,mpd)
% ------------------------------------------
% FILE   : phasecal.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-02-19  Created.
% ------------------------------------------
% PURPOSE
%   Calculate the phase according to the input displacement profile.
%
%   phase = phasecal(d)
%   phase = phasecal(d,mpd)
% ------------------------------------------
% INPUT
%   d:          The displacement profile.
%   mpd:        (Optional) The minimum peak distance. Specifying this would
%               help the code find the peaks in displacement profile more correctly.
% ------------------------------------------
% OUTPUT
%   phase:      The phase values ranging from 0 to 1;

%% Get the peaks and troughs

SMOOTHING = 1600;

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
% Calculate the average baseline position
meanBase = 0.5*median(pv) + 0.5*median(nv);
% Get a better estimation using the estimated baseline position
[pv,pp] = findpeaks(res,'MINPEAKDISTANCE',mpd,'MINPEAKHEIGHT',meanBase);
[nv,np] = findpeaks(-res,'MINPEAKDISTANCE',mpd,'MINPEAKHEIGHT',-meanBase);  nv = -nv;

% Eliminate excessive troughs
kp1 = 1; kp2 = 2;
while kp2 <= length(np)
    if sum(pp > np(kp1) & pp < np(kp2)) == 0
        if nv(kp2) > nv(kp1)
            np(kp2) = [];
            nv(kp2) = [];
        elseif nv(kp2) < nv(kp1)
            np(kp1) = [];
            nv(kp1) = [];
        elseif nv(kp2) == nv(kp1)
            np(kp1) = 0.5*np(kp1) + 0.5*np(kp2);
            np(kp2) = [];
            nv(kp2) = [];
        end
    elseif sum(pp > np(kp1) & pp < np(kp2)) >= 1
        kp1 = kp2;  kp2 = kp2 + 1;
    end
end

% Eliminate excessive peaks
kp1 = 1; kp2 = 2;
while kp2 <= length(pp)
    if sum(np > pp(kp1) & np < pp(kp2)) == 0
        if pv(kp2) < pv(kp1)
            pp(kp2) = [];
            pv(kp2) = [];
        elseif pv(kp2) > pv(kp1)
            pp(kp1) = [];
            pv(kp1) = [];
        elseif pv(kp2) == pv(kp1)
            pp(kp1) = 0.5*pp(kp1) + 0.5*pp(kp2);
            pp(kp2) = [];
            pv(kp2) = [];
        end
    elseif sum(np > pp(kp1) & np < pp(kp2)) >= 1
        kp1 = kp2;  kp2 = kp2 + 1;
    end
end

% Eliminate cycles that are too small

% Construct a signal with only peak and trough positions first
res_tmp = zeros(1,length(pp) + length(np) + 2);
if np(1) < pp(1)
    res_tmp(1) = pv(1);
    res_tmp(2:2:2*length(np)) = nv;
    res_tmp(3:2:2*length(pp)+1) = pv;
else
    res_tmp(1) = nv(1);
    res_tmp(2:2:2*length(pp)) = pv;
    res_tmp(3:2:2*length(np)+1) = nv;
end
if np(end) > pp(end)
    res_tmp(end) = pv(end);
else
    res_tmp(end) = nv(end);
end;

% Calculate expected motion range
amp = median(pv) - median(nv);
% Use half of this range as the threshold and find valid peaks and troughs
% again
thr_amp = 0.25*amp;
[~,pp_ind] = findpeaks(res_tmp,'THRESHOLD',thr_amp);
[~,np_ind] = findpeaks(-res_tmp,'THRESHOLD',thr_amp);

% Update pp,np,pv, and nv
if np(1) < pp(1)
    pp_ind = (pp_ind-1) / 2;
    np_ind = np_ind / 2;
else
    pp_ind = pp_ind / 2;
    np_ind = (np_ind - 1) / 2;
end
pp = pp(pp_ind);    pv = pv(pp_ind);
np = np(np_ind);    nv = nv(np_ind);

% Eliminate excessive peaks and troughs again
% Eliminate excessive troughs
kp1 = 1; kp2 = 2;
while kp2 <= length(np)
    if sum(pp > np(kp1) & pp < np(kp2)) == 0
        if nv(kp2) > nv(kp1)
            np(kp2) = [];
            nv(kp2) = [];
        elseif nv(kp2) < nv(kp1)
            np(kp1) = [];
            nv(kp1) = [];
        elseif nv(kp2) == nv(kp1)
            np(kp1) = 0.5*np(kp1) + 0.5*np(kp2);
            np(kp2) = [];
            nv(kp2) = [];
        end
    elseif sum(pp > np(kp1) & pp < np(kp2)) >= 1
        kp1 = kp2;  kp2 = kp2 + 1;
    end
end

% Eliminate excessive peaks
kp1 = 1; kp2 = 2;
while kp2 <= length(pp)
    if sum(np > pp(kp1) & np < pp(kp2)) == 0
        if pv(kp2) < pv(kp1)
            pp(kp2) = [];
            pv(kp2) = [];
        elseif pv(kp2) > pv(kp1)
            pp(kp1) = [];
            pv(kp1) = [];
        elseif pv(kp2) == pv(kp1)
            pp(kp1) = 0.5*pp(kp1) + 0.5*pp(kp2);
            pp(kp2) = [];
            pv(kp2) = [];
        end
    elseif sum(np > pp(kp1) & np < pp(kp2)) >= 1
        kp1 = kp2;  kp2 = kp2 + 1;
    end
end

% Calculate the phase
phase = zeros(1,nproj);
for k = 1:length(np)-1
    pp_tmp = pp(pp > np(k) & pp < np(k+1));
    phase(np(k):pp_tmp) = linspace(0,0.5,pp_tmp-np(k)+1);
    phase(pp_tmp:np(k+1)-1) = linspace(0.5,1,np(k+1)-pp_tmp);
end

% Extrapolate the start and end points
if np(1) < pp(1)
    ph_frac = np(1) / mean(diff(np));
    if ph_frac > 1; ph_frac = 1;    end
    phase(1:np(1)-1) = linspace(1-ph_frac,1,np(1)-1);
else
    pp_tmp = pp(pp < np(1));
    pv_tmp = res(pp_tmp);
    pp_tmp = pp_tmp(pv_tmp == max(pv_tmp));
    pp_tmp = round(mean(pp_tmp));
    phase(pp_tmp:np(1)-1) = linspace(0.5,1,np(1)-pp_tmp);
    ph_halffrac = pp_tmp / mean(diff(np));
    if ph_halffrac > 0.5; ph_halffrac = 0.5;    end
    phase(1:pp_tmp) = linspace(0.5-ph_halffrac,0.5,pp_tmp);
end
    
if np(end) > pp(end)
    ph_frac = (nproj - np(end)) / mean(diff(np));
    if ph_frac > 1; ph_frac = 1;    end
    phase(np(end)+1:end) = linspace(0,ph_frac,nproj-np(end));
else
    pp_tmp = pp(pp > np(end));
    pv_tmp = res(pp_tmp);
    pp_tmp = pp_tmp(pv_tmp == max(pv_tmp));
    pp_tmp = round(mean(pp_tmp));
    phase(np(end):pp_tmp) = linspace(0,0.5,pp_tmp-np(end)+1);
    ph_halffrac = (nproj-pp_tmp) / mean(diff(np));
    if ph_halffrac > 0.5; ph_halffrac = 0.5;    end
    phase(pp_tmp:end) = linspace(0.5,0.5+ph_halffrac,nproj-pp_tmp+1);
end
    
return