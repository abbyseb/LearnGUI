function ind = findStruct(s,field,target,substr)
%% ind = findStruct(s,field,target,substr)
% ------------------------------------------
% FILE   : findStruct.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-09-09  Created.
% ------------------------------------------
% PURPOSE
%   Find the index of the struct that has a certain field equal to the
%   specified target value.
% ------------------------------------------
% INPUT
%   s      :  The array of struct.
%   field: :  The field of the struct to be compared, e.g. ROIName
%   target :  The target value to be matched, e.g. 'GTV'
%	substr :  True if look for substring. False if not.
%		      Default: false.
% ------------------------------------------
% OUTPUT
%   ind:   :  The indices of the struct that has "field" = "target"
% ------------------------------------------

%%
if nargin < 4
	substr = false;
end
if ischar(target)
	if substr
		eqFunc = @(x) ~isempty(strfind(x,target));
	else
		eqFunc = @(x) strcmpi(x,target);
	end
else
    eqFunc = @(x) x == target;
end

ind = [];
for k = 1:length(s)
    if eqFunc(s(k).(field))
        ind(end+1) = k;
    end
end
