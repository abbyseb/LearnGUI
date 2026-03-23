function result = ReadIVTResults(filename)
%% result = ReadIVTResults(filename)
% ------------------------------------------
% FILE   : ReadIVTResults.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-08-25   Creation
% ------------------------------------------
% PURPOSE
%   Read and process IVT results.
%
% ------------------------------------------
% INPUT
%   filename:   The file to be read.
% ------------------------------------------
% OUTPUT
%   out:        IVT result struct.
%
%% 
if nargin < 1
    result = ProcessIVTResults(ReadCSVFileWithHeader);
else
    result = ProcessIVTResults(ReadCSVFileWithHeader(filename));
end