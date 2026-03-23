function imshow_hnc(filepath)
% IMSHOW_HNC Visualise a Varian .hnc projection file
%   IMSHOW_HNC(filepath) loads and displays the .hnc file.
%   If no filepath is provided, a file picker will open.

if nargin < 1
    [file, path] = uigetfile('*.hnc', 'Select an .hnc file');
    if isequal(file, 0)
        disp('User selected Cancel');
        return;
    end
    filepath = fullfile(path, file);
end

try
    [info, M] = HncRead(filepath);
    
    % Display metadata
    fprintf('File: %s\n', filepath);
    if isfield(info, 'dCTProjectionAngle')
        fprintf('Gantry Angle: %.2f deg\n', info.dCTProjectionAngle);
    end
    fprintf('Resolution: %d x %d\n', info.uiSizeX, info.uiSizeY);

    % Visualize
    figure;
    imagesc(M'); 
    colormap gray; 
    axis image; 
    axis off;
    
    [~, name, ext] = fileparts(filepath);
    title_str = sprintf('File: %s%s', name, ext);
    if isfield(info, 'dCTProjectionAngle')
        title_str = sprintf('%s | Angle: %.2f°', title_str, info.dCTProjectionAngle);
    end
    title(title_str, 'Interpreter', 'none');
    
    % Add colorbar to see intensity range
    colorbar;
    
catch e
    fprintf('Error visualizing .hnc file:\n%s\n', e.message);
end

end
