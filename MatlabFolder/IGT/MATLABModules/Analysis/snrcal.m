function [snrval,nval,mask] = snrcal(img,slice,mask)
%% [snrval,nval,mask] = snrcal(img,slice,mask)
% ------------------------------------------
% FILE   : snrcal.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-05-04  Created
% ------------------------------------------
% PURPOSE
%   Calculate the signal-to-noise ratio of an object.
%	The ROI for SNR calculation is specified by the input logical mask. If
%	it is not input or input as an empty field, the user will be prompt for
%	ROI selection.
%   This code assumes that img has the 3D dimension of that directly read
%   in from a RTK output image format.
%
%   [snrval,nval,mask] = snrcal(img,slice,mask)
% ------------------------------------------
% INPUT
%   img:            The 3D img matrix compatible with RTK output format.
%                   (coronal,axial,sagittal)
%   slice:          The number(s) of axial slices. e.g. 50:60
%                   This input is ignored for a non-empty input of mask_obj
%                   or mask_bg.
%   mask:         The logical mask specifying the ROI.
%                   Input '[]' to manually draw the mask on the image.
% ------------------------------------------
% OUTPUT
%   snrval:         The snr value.
%   nval:           The noise value (rMSE)
%   mask_obj:       The logical mask specifying the ROI.

%% Input check

if ~(all(mod(slice,1)==0) && isvector(slice) && all(slice<=size(img,2)))
    error('ERROR: "slice" must be an integer vector input with all slice numbers not exceeding the img dimension.');
end

if nargin < 3
    mask = [];
end;

if ~isempty(mask) && ~isequal(size(mask),size(img))
    error('ERROR: The dimension of "mask" is not compatible with "img".');
end

% Permute vector
pv = [1 3 2];

%% Acquire mask_bg and mask_obj

if isempty(mask)
    % Permute the image such that axial is in the third dimension for display
    img_axial = permute(img,pv);
    
    mask_tmp = false(size(img_axial));
    
    fig_handle = figure;
    
    for k_slice = slice(1):slice(end)
        imagesc(flipud(img_axial(:,:,k_slice)')); colormap gray; axis image;
        title(num2str(k_slice,'#%03d slice of the image.'));

        disp(num2str(k_slice,'Contour the ROI in #%03d slice of the image.'));
        tmp = [];
        while isempty(tmp)
            tmp = roipoly;
            if isempty(tmp)
                disp('   Try again.');
            end;
        end;
        mask_tmp(:,:,k_slice) = tmp(end:-1:1,:)';
    end
    
    close(fig_handle);
    
    mask = permute(mask_tmp,pv);
    
end

%% Calculate snr

snrval = mean(img(mask)) / std(img(mask));
nval = std(img(mask));

return