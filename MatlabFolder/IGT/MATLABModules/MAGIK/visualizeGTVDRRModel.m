function visMat = visualizeGTVDRRModel(model)
%% visualizeGTVDRRModel(model)
% ------------------------------------------
% FILE   : visualizeGTVDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-06  Created.
% ------------------------------------------
% PURPOSE
%  Visualize GTV DRR model (as generated from gtvDRRModel.m)
% ------------------------------------------
% INPUT
%   model:      The MATLAB struct that stores the GTV DRR model.
%
% OUTPUT
%   visMat:     The visualization matrix.


%% Compute visualization
visMat = single(zeros([model.ProjSize,length(model.Angles)]));

for k = 1:length(model.Angles)
    if isempty(model.DRR{k})
        continue;
    end
    slice = single(zeros(model.ProjSize));
    drrDim = model.Corners(2,:,k) - model.Corners(1,:,k) + 1;
    slice(model.Corners(1,1,k):model.Corners(2,1,k),...
        model.Corners(1,2,k):model.Corners(2,2,k)) = ...
        reshape(model.DRR{k},drrDim);
    slice(model.ContourIdx1D{k}) = 1;
    visMat(:,:,k) = slice;
end