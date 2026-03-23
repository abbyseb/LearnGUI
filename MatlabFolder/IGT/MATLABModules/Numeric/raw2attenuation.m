function attenuation = raw2attenuation(raw,type)
%% attenuation = raw2attenuation(raw,type)
% ------------------------------------------
% FILE   : raw2attenuation.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-10   Created.
% ------------------------------------------
% PURPOSE
%   Convert raw HNC or raw HIS values to attenuation values
%
% ------------------------------------------
% INPUT
%   raw:        The input raw intensity values.
%   type:       (Optional) 'hnc' or 'his'.
%               Default: 'hnc'
% ------------------------------------------
% OUTPUT
%   attenuation:Attenuation values.

%%

HNCMAX = 65535;
HISMAX = 65535;

if nargin < 2
    type = 'hnc';
end;

if strcmpi(type,'hnc')
    raw(raw<=0) = 1;    raw(raw>HNCMAX) = HNCMAX;
    attenuation = log(single(HNCMAX)) - log(single(raw));
elseif strcmpi(type,'his')
    raw(raw>=HISMAX) = HISMAX-1;
    attenuation = single(log(HISMAX) - log(HISMAX-single(raw)));
else
    error('ERROR: Unsupported type.');
end

return;
