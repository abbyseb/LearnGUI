function getVALKIMprojections
vol_dir = dir('CT*.mha');

for n = 1:length(vol_dir)
    vol_name = vol_dir(n).name;
    vol_num = split(vol_name, '_'); vol_num = split(vol_num{2}, '.'); vol_num = vol_num{1};
    dirPath = pwd;
    projDir = pwd;
    projDim = [1024,768];
    projSpacing = [0.388,0.388];
    rtkf = ['"',which('rtkforwardprojections.exe'),'"'];
    geometryFile = which('RTKGeometry_360.xml');
    if isempty(geometryFile)
        error('ERROR: Could not find RTKGeometry_360.xml on the MATLAB path.');
    end
    angles = ReadAnglesFromRTKGeometry(geometryFile)';

    tic;
    for nf = 1:length(angles)
        fprintf(['    Forward projecting frame ', num2str(nf), '.....'])
       
        % Tempoary sort file for simulation
        s = zeros(size(angles));
        s(nf) = 1;
        csvwrite(fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),s(:));
        
        % RTK forward projection
        [failed,msg] = system([rtkf,' -g "',geometryFile,'" ',...
            '-i "',fullfile(dirPath,vol_name),'" ',...
            '-o "',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),'" ',...
            '--sortfile "',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),'" ',...
            '--dimension ',num2str(projDim,'%d,%d'),' --spacing ',num2str(projSpacing,'%f,%f')]);
        if failed
            error(['ERROR: rtkforwardprojections failed to simulate ',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),' with the following error message: ',msg]);
        end
        [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),' with the following error message: ',msg]);
        end
        
        % Convert projections to HNC
        templateFile = which('hncHeaderTemplate.mat');
        if isempty(templateFile)
            error('ERROR: Could not find hncHeaderTemplate.mat on the MATLAB path.');
        end
        load(templateFile); 
        pInfo = hncHeaderTemplate;
        [~,Proj] = MhaRead(fullfile(dirPath,num2str(nf,'Proj_%05d.mha')));
        Proj = 65535 * exp(-Proj);
        Proj(Proj>65535) = 65535; Proj(Proj<0) = 0;
        Proj = uint16(round(Proj));
        pInfo.dCTProjectionAngle = angles(nf);
        pInfo.dCBCTPositiveAngle = mod(angles(nf) + 270, 360);
        pInfo.uiSizeX = projDim(1);
        pInfo.uiSizeY = projDim(2);
        pInfo.dIDUResolutionX = projSpacing(1);
        pInfo.dIDUResolutionY = projSpacing(2);
        pInfo.uiFileLength = 512 + 2 * prod(projDim);
        pInfo.uiActualFileLength = pInfo.uiFileLength;
        HncWrite(pInfo,Proj,fullfile(projDir,[vol_num,'_Proj_',num2str(nf, '%05d'),'.hnc']))
        %[failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),' with the following error message: ',msg]);
        end
        fprintf('done! \n')
    end
fprintf(['=====Projections completed in ', seconds2human(toc),'!=====\n'])
end