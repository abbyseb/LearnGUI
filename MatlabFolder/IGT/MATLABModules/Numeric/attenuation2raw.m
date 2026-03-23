function raw = attenuation2raw(attenuation,type)
%% attenuation = attenuation2raw(raw,type)
% ------------------------------------------
% FILE   : attenuation2raw.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-10   Created.
% ------------------------------------------
% PURPOSE
%   Convert attenuation values to raw HNC or raw HIS intensity values.
%
% ------------------------------------------
% INPUT
%   attenuation:The input attenuation values.
%   type:       (Optional) 'hnc' or 'his'.
%               Default: 'hnc'
% ------------------------------------------
% OUTPUT
%   raw:        The input raw intensity values.

%%

HNCMAX = 65535;
HISMAX = 65535;

if nargin < 2
    type = 'hnc';
end;

if strcmpi(type,'hnc')
    attenuation(attenuation<0) = 0;
    raw = uint16(HNCMAX * exp(-attenuation));
elseif strcmpi(type,'his')
    attenuation(attenuation<0) = 0;
    raw = uint16(HISMAX * (1 - exp(-attenuation)));
else
    error('ERROR: Unsupported type.');
end

return;
