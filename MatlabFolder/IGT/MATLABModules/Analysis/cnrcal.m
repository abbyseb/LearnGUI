function [result,mask_bg,mask_obj] = cnrcal(img,slice,mask_bg,mask_obj)
%% [result,mask_bg,mask_obj] = cnrcal(img,slice,mask_bg,mask_obj)
% ------------------------------------------
% FILE   : cnrcal.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-04-24  Created
% ------------------------------------------
% PURPOSE
%   Calculate the contrast-to-noise ratio of an object.
%	The object and the background is specified by the input logical mask
%	mask_obj and mask_bg. If an empty field is input for either one of
%	those fields, the user will be prompt for selecting a polygon for each
%	slice.
%   This code assumes that img has the 3D dimension of that directly read
%   in from a RTK output image format.
%
%   [result,mask_bg,mask_obj] = cnrcal(img,slice,mask_bg,mask_obj)
% ------------------------------------------
% INPUT
%   img:            The 3D img matrix compatible with RTK output format.
%                   (coronal,axial,sagittal)
%   slice:          The number(s) of axial slices. e.g. 50:60
%                   This input is ignored for a non-empty input of mask_obj
%                   or mask_bg.
%   mask_bg:         The logical mask specifying the background.
%                   Input '[]' to manually draw the mask on the image.
%   mask_obj:       The logical mask specifying the object.
%                   Input '[]' to manually draw the mask on the image.
% ------------------------------------------
% OUTPUT
%   result:         The CNR value.
%   mask_obj:       The logical mask specifying the object.
%   mask_bg:         The logical mask specifying the background.

%% Input check

if ~(all(mod(slice,1)==0) && isvector(slice) && all(slice<=size(img,2)))
    error('ERROR: "slice" must be an integer vector input with all slice numbers not exceeding the img dimension.');
end

switch nargin
    case 2
        mask_bg = [];   mask_obj = [];
    case 3
        mask_obj = [];
    otherwise
end;

if ~isempty(mask_bg) && ~isequal(size(mask_bg),size(img))
    error('ERROR: The dimension of "mask_bg" is not compatible with "img".');
end

if ~isempty(mask_obj) && ~isequal(size(mask_obj),size(img))
    error('ERROR: The dimension of "mask_obj" is not compatible with "img".');
end

% Permute vector
pv = [1 3 2];

%% Acquire mask_bg and mask_obj

if isempty(mask_bg) || isempty(mask_obj)
    % Permute the image such that axial is in the third dimension for display
    img_axial = permute(img,pv);
    
    mask_bg_tmp = false(size(img_axial));
    mask_obj_tmp = false(size(img_axial));
    
    fig_handle = figure;
    
    for k_slice = slice(1):slice(end)
        imagesc(flipud(img_axial(:,:,k_slice)')); colormap gray; axis image;
        title(num2str(k_slice,'#%03d slice of the image.'));
        
        if isempty(mask_bg)
            disp(num2str(k_slice,'Contour the background in #%03d slice of the image.'));
            tmp = [];
            while isempty(tmp)
                tmp = roipoly;
                if isempty(tmp)
                    disp('   Try again.');
                end;
            end;
            mask_bg_tmp(:,:,k_slice) = tmp(end:-1:1,:)';
        end
        
        if isempty(mask_obj)
            disp(num2str(k_slice,'Contour the object in #%03d slice of the image.'));
            tmp = [];
            while isempty(tmp)
                tmp = roipoly;
                if isempty(tmp)
                    disp('   Try again.');
                end;
            end;
            mask_obj_tmp(:,:,k_slice) = tmp(end:-1:1,:)';
        end
    end
    
    close(fig_handle);
    
    if isempty(mask_bg)
        mask_bg = permute(mask_bg_tmp,pv);
    end
    
    if isempty(mask_obj)
        mask_obj = permute(mask_obj_tmp,pv);
    end
    
end

%% Calculate CNR

result = (mean(img(mask_obj)) - mean(img(mask_bg))) / sqrt(var(img(mask_obj)) + var(img(mask_bg)));

return