function downsampleVALKIMvolumes
fprintf('Processing.....\n')
% List compressed volumes
vol_list = dir('CT_*.mha');

tic;
for n = 1:length(vol_list)
   
    % Load mha
    vol_name = vol_list(n).name;
    [info, M] = MhaRead(vol_name);
    %delete(vol_name)
    
    % Recale
    M = imresize3(M, [512 256 512]);
    M = M(1:4:end, 1:2:end, 1:4:end);
    M(M < 0) = 0;
    info.Dimensions = [128, 128, 128];
    info.PixelDimensions = [1, 1, 1];
    info.Offset = [0, 0, 0];
    
    % Save as mha
    MhaWrite(info, M, ['sub_CT', vol_name(end-6:end)])


end
fprintf(['=====Volumes processed in ', seconds2human(toc),'=====\n'])