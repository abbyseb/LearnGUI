function [result,pixcoord] = crmarker(inputfl,roi,bgcorners,refcoord)
%% [result,pixcoord] = crmarker(inputfl,roi,bgcorners,refcoord)
% ------------------------------------------
% FILE   : crmarker.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-03-06   Created.
% ------------------------------------------
% PURPOSE
%   Calculate the contrast ratio of the Metaimage files in the input
%   file list, output the result in a vector. This codes work similarly to
%   cr.m, but detect the marker position automatically in the roi
%   specified. If multiple markers are in the ROI, the code selects the one
%   closer to the reference coordinate.
%
%   [result,pixcoord] = crmarker(inputfl,roi,bgcorners,refcoord)
% ------------------------------------------
% INPUT
%   inputfl:        The input file list (can be obtained using ls('*.mha'))
%   roi:            The two corners specifying the 3D region for marker
%                   searching.
%   bgcorners:      The 3D pixel coordinates of the two corners of the background
%                   rectangle used to calculate the CR, in the format of
%                   [x1,y1,z1; x1,y1,z1] in terms of pixels.
%   refcoord:       The 3D reference coordinate which specify the 
%                   point around where the marker is likely to be.
% ------------------------------------------
% OUTPUT
%   result:         The results in a vector corresponding to each file.
%   pixcoord:       The pixel coordinates of the marker in the format of
%                   [x1,y1,z1;x2,y2,z2;...] in terms of pixels.

%%

nfile = size(inputfl);  nfile = nfile(1);

% The maximum possible size of a marker in terms of pixels
D_max = 3;

NEG_IMPOSSIBLE = -99999;

bgcorners = round(bgcorners);
roi = round(roi);
roi_x = min(roi(:,1)):max(roi(:,1));
roi_y = min(roi(:,2)):max(roi(:,2));
roi_z = min(roi(:,3)):max(roi(:,3));
bg_x = min(bgcorners(:,1)):max(bgcorners(:,1));
bg_y = min(bgcorners(:,2)):max(bgcorners(:,2));
bg_z = min(bgcorners(:,3)):max(bgcorners(:,3));
    
result = zeros(nfile,1);
pixcoord = zeros(nfile,3);
    
for k_file = 1:nfile
    [info,M] = MhaRead(inputfl(k_file,:));
    
    M_bg = M(bg_x,bg_y,bg_z);
    I_bg = mean(M_bg(:));
    
    M_roi = M(roi_x,roi_y,roi_z);
    
    % Search for the appropriate initial marker position
    if k_file > 1
        % For the remaining files, try to make sure marker positions are continuosly connected
        refcoord = pixcoord(k_file-1,:);
    end
    
    while norm(pixcoord(k_file,:) - refcoord) > D_max
        I_m = max(M_roi(:));
        
        tag = 0;
        for kx = 1:length(roi_x)
            for ky = 1:length(roi_y)
                for kz = 1:length(roi_z)
                    if M_roi(kx,ky,kz) == I_m
                        pixcoord(k_file,:) = [kx,ky,kz];
                        tag = 1;
                        break;
                    end
                end
                if tag; break; end;
            end
            if tag; break;  end;
        end
        M_roi(kx,ky,kz) = NEG_IMPOSSIBLE;
        pixcoord(k_file,:) = pixcoord(k_file,:) + [roi_x(1)-1,roi_y(1)-1,roi_z(1)-1];
    end
    
    result(k_file,1) = (I_m - I_bg) / I_m;
    
end

return