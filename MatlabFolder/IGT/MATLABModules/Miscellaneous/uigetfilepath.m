function [filepath,filterindex] = uigetfilepath(varargin)
%% filepath = uigetfilepath(varargin)
% ------------------------------------------
% FILE   : uigetfilepath.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-23   Created
% ------------------------------------------
% PURPOSE
%   Pretty much the same as the MATLAB function "uigetfile", except that
%   the output combines the file name and file path just to make life
%   easier. In addition, the default is to list all file types in the UI.
%   All other usages are the same as "uigetfile".
% ------------------------------------------
% INPUT
%   SEE "help uigetfile" for details.
% ------------------------------------------
% OUTPUT
%   filepath:       The full file path.
%   filterindex:    The filter index (see "help uigetfile").

%%

if isempty(varargin)
    [filename,pathname,filterindex] = uigetfile('*');
else
    [filename,pathname,filterindex] = uigetfile(varargin{:});
end;

if isnumeric(filename)
    filepath = filename;
else
    filepath = fullfile(pathname,filename);
end

return;
