function QuickRTKImageDisplay(M,orientation,slice)
% Assume RTK image output format

gmin = 0;
gmax = 0.02;
spacing = [0.5,2,0.5];

switch orientation
    case 'coronal'
        % Do nothing
    case 'sagittal'
        M = permute(M,[3,2,1]);
    case 'axial'
        M = permute(M,[1 3 2]);
    otherwise
        error('ERROR: The input field "orientation" has to be either "coronal", "sagittal", or "axial".');
end
             
if nargin < 3
    slice = 0.5* size(M,3);
end

slice = round(slice);
    
switch orientation
    case 'coronal'
        s = M(10:end-37,14:end-15,slice)';
        s = imresize(s,[round(size(s,1)*spacing(2)/spacing(1)),size(s,2)]);
        imagesc(s,[gmin,gmax]); colormap gray; axis image; axis off;
    case 'sagittal'
        s = M(5:end-80,20:end-10,slice)';
        s = imresize(s,[round(size(s,1)*spacing(2)/spacing(3)),size(s,2)]);
        imagesc(s,[gmin,gmax]); colormap gray; axis image; axis off;
%         imagesc(s(145:175,117:154),[gmin,gmax]); colormap gray; axis image; axis off;
    case 'axial'
        s = flipud(M(10:end-25,20:end-70,slice)');
        s = imresize(s,[round(size(s,1)*spacing(3)/spacing(1)),size(s,2)]);
        imagesc(s,[gmin,gmax]); colormap gray; axis image; axis off;
    otherwise
        error('ERROR: The input field "orientation" has to be either "coronal", "sagittal", or "axial".');
end

return