function visMat = visualizeDphDRRModel(model)
%% visualizeDphDRRModel(model)
% ------------------------------------------
% FILE   : visualizeDphDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-02-27  Created.
% ------------------------------------------
% PURPOSE
%  Visualize 2D diaphragm DRR model (as generated from dphDRRModel.m)
% ------------------------------------------
% INPUT
%   model:      The MATLAB struct that stores the 2D diaphragm DRR model.
%
% OUTPUT
%   visMat:     The visualization matrix in cell format. 1: right, 2: left


%% Compute visualization
visMat{1} = single(zeros([model.ProjSize,length(model.Angles)]));
visMat{2} = single(zeros([model.ProjSize,length(model.Angles)]));

for k = 1:length(model.Angles)
    slice = single(zeros(model.ProjSize));
    slice(model.Idx1D_R{k}) = model.DRR_R{k};
    visMat{1}(:,:,k) = slice;
    
    slice = single(zeros(model.ProjSize));
    slice(model.Idx1D_L{k}) = model.DRR_L{k};
    visMat{2}(:,:,k) = slice;
end