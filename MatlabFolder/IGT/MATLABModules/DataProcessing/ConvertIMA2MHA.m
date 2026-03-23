function ConveretIMA2MHA(fl,outputFile)
%% ConveretIMA2MHA(fl,outputFile)
% ------------------------------------------
% FILE   : ConveretIMA2MHA.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-04-08 Created.
% ------------------------------------------
% PURPOSE
%   Convert a list of Seimens IMA file to a single MHA volume with RTK
%   compatible geometry (IEC standard)
%
% ------------------------------------------
% INPUT
%   fl:         List of IMA files in cells.
%   outputFile: Path to output mha file.
% ------------------------------------------

%% 

parts = regexp(fl{1},'\.','split');
nameHead = ...
    [parts{1},'.',parts{2},'.',parts{3},'.',parts{4},'.'];

for k = 1:length(fl)
    parts = regexp(fl{k},'\.','split');
    fileNo(k) = str2double(parts{5});
end
fileNo = sort(fileNo);

for k = 1:length(fileNo);
    filename = lscp([nameHead,num2str(fileNo(k),'%d'),'.*.IMA']);
    M(:,:,k) = dicomread(filename);
end

M = permute(M,[2 3 1]);

info = load(which('mhaHeaderTemplate.mat'));
info = info.mhaHeaderTemplate;
info.Dimensions = size(M);
info.PixelDimensions = [0.47,0.47,0.47];
info.Offset = -(info.Dimensions - 1)/2 .* info.PixelDimensions;
info.DataType = class(M);
MhaWrite(info,M,outputFile);

return;