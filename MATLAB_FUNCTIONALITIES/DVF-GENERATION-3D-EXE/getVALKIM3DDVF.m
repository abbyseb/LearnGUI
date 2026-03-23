function getVALKIM3DDVF
% List mha files
mha_list = dir('sub*.mha');

% Select moving image (exhale CT)
movImgFile = mha_list(6).name; 

% Scroll through fixed images
for n = 1:length(mha_list)
    tic;
    % Select fixed image
    fixImgFile = mha_list(n).name;
    
    % Perform registration
    paraFile = 'Elastix_BSpline_Sliding';
    elastixRegistration(movImgFile,fixImgFile,pwd,which([paraFile, '.txt']))
    
    % Generate DVF
    transformFile = [movImgFile(1:end-4), '_', paraFile, '_', fixImgFile(1:end-4)];
    dvfName = ['DVF_', fixImgFile(7:end)];
    elastixTransform([transformFile, '.txt'],'',false,dvfName)
    
    % Delete fixed image, warped image, transform text
    delete([transformFile, '.mha'])
    delete([transformFile, '.txt'])
    
    fprintf(['DVF ', num2str(n), ' processed in ', seconds2human(toc),'\n'])
end

fprintf('=====DVF Generation complete=====\n')   