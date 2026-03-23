function fl_out = lscp(rexp,dirmode)
%% fl = lscp(regexp)
% ------------------------------------------
% FILE   : lscp.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-02-27   Creation
% ------------------------------------------
% PURPOSE
%   A cross-platform ls function.
%   This function overlaps with the MATLAB built-in ls function, but adjust
%   the output format so that on both Windows and Linux the file list is
%   output in the format of [path\file1;path\file2;path\file3;...].
%
%   fl = lscp(rexp)
% ------------------------------------------
% INPUT
%   rexp:         The regular expression. rexp can contain file path as
%                 well. e.g. '\Projection\bin01\SPI*'
%   dirmode:      If the user input 'dir' for the second input, then only
%                 directories will be considered.
% ------------------------------------------
% OUTPUT
%   fl:             The file list.

%%
if nargin < 2;  dirmode = '';   end;

[dirname,rexp,ext] = fileparts(rexp);
rexp = [rexp,ext];

currentDir = pwd;
if ~isempty(dirname);
    try
        cd (dirname);
    catch
        fl_out = '';
        cd (currentDir);
        return;
    end
end;

if ispc
    fl = ls(rexp);
elseif isunix || ismac
    fl_s = dir(rexp);
    n = length(fl_s);
    for k = 1:n
        l = length(fl_s(k).name);
        fl(k,1:l) = fl_s(k).name;
    end
else
    fl = ls(rexp);
end

if isempty(fl)
    fl_out = [];
    cd (currentDir);
    return;
end;

% Exclude . and ..
ind = find(fl(:,1)=='.');
fl(ind,:) = [];

if isempty(fl)
    cd (currentDir);
    fl_out = [];
    return;
end;

if strcmp(dirmode,'dir')
    count = 0;
    for k = 1:size(fl,1)
        if ~isdir(fl(k-count,:))
            fl(k-count,:) = [];
            count = count + 1;
        end
    end;
end;    

% Add in the directory path
% fl = [ char(ones(size(fl,1),1) * [pwd,fsepcp]) , fl];
if size(fl,1) > 0
    for k = 1:size(fl,1)
        fl_out(k,:) = fullfile(pwd,fl(k,:));
    end;
else
    fl_out = [];
end;

if ~isempty(dirname);
    cd (currentDir);
end;

return;
