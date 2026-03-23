%% LEARN_DBQueries
% Hard-code base directory
base_dir = 'Z:\PRJ-RPL\2RESEARCH\1_ClinicalData';

% List and select clinical trials
trial_list = {'VCU', 'OPTIMAL', 'ROCK-RT', 'MAGIK', 'SPARK', 'RAVENTA', ...
    'SPANC', 'CHIRP', 'VALKIM', 'AVIATOR', 'SMART1'};
[trial_indx,~] = listdlg('ListString',trial_list,...
    'PromptString',{'Select a clinical trial.',''});

% List and select desired centre
centre_list = {'CMN', 'RNSH', 'Liverpool', 'PMC', 'Westmead',...
    'Blacktown', 'Nepean', 'Gosford', 'Mannheim', 'GenesisCare', 'Orange'};
[centre_indx,~] = listdlg('ListString',centre_list,...
    'PromptString',{'Select a centre.',''});

% List and select desired patient planning data
query = ['?trial=', trial_list{trial_indx}, '&centre=' centre_list{centre_indx},];
details = webread(['http://10.65.67.179:8090/prescriptions' query]);
pat_list = {details.prescriptions.patient_trial_id};
[pat_indx,~] = listdlg('ListString',pat_list,...
    'PromptString',[{'Select a patient to generate training data.',...
    '(May not be in alphabetical order)'} {''}]);

% Iterate over patients and pre-process data
dir_list = {details.prescriptions.rt_ct_pres};
for n = pat_indx
    fprintf(['Generating training data for ', pat_list{n}, '\n'])
    
    % Identify directory
    im_dir = dir_list{n};
    im_dir = split(im_dir, '/');
    im_dir{4} = 'MAGIKmodel'; im_dir{end} = 'FilesForModelling'; 
    im_dir = strjoin(im_dir, '\');

    % Download data in present working directory
    fprintf('Copying training files.....')
    tic;
    source = [base_dir, im_dir];
    folder_name = strsplit(im_dir, '\'); folder_name = folder_name{end-1};
    destination = [pwd, '\', folder_name, '\train'];
    copyfile(source, destination);
    
    % Clean files
    magik_dir = dir('MAGIK*');

    fprintf(['done in ', seconds2human(toc), '\n'])

    % Convert DICOM to MHA
    fprintf('Converting DICOM to MHA.....')
    tic;
    cd(destination)
    DicomToMha_VALKIM
    
    % Downsample MHAs
    downsampleVALKIMvolumes
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Forward project MHAs to generate DRRs
    fprintf('Generating DRRs.....')
    tic;
    getVALKIMprojections

    % Downsample DRRs
    compressProj
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Register DRRs to generate 2D DVFs
    fprintf('Generating 2D DVFs.....')
    tic;
    getVALKIM2DDVF
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Register downsampled volumes to generate 3D DVFs
    fprintf('Generating downsampled 3D DVFs.....')
    tic;
    getVALKIM3DDVF
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Register full-resolution volumes to generate 3D DVFs
    fprintf('Generating full-resolution 3D DVFs.....')
    tic;
    getVALKIM3DDVF_full
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Save exhale CT as source
    copyfile('CT_06.mha', 'source.mha')

    % Clean up MHAs and DICOMs
    mha_list = dir('*mha');
    for n = 1:length(mha_list)
        if ~startsWith(mha_list(n).name, 'sub') || ~startsWith(mha_list(n).name, 'source')
            delete(mha_list(n).name)
        end
    end

    dcm_list = dir('*dcm');
    for n = 1:length(dcm_list)
        delete(dcm_list(n).name)
    end
    
    % Return to patient directory
    cd('../')

    % Create test directory and enter
    fprintf('Copying test files.....')
    tic;
    im_dir{4} = 'Treatment files'; im_dir{1} = ''; im_dir{end} = ''; 
    im_dir = strjoin(im_dir{2:end-1}, '\');
    source = [base_dir, im_dir];
    copyfile(source, 'test');
    cd('test')
    fprintf(['done in ', seconds2human(toc), '\n'])

    % Iterate over fractions
    fprintf('Pre-processing kV images.....')
    tic;
    fx_list = dir('Fx*');
    for n = 1:length(fx_list)
        cd(fx_list(n).name)
        cd('kV')
        
        % Convert tiff files to bin
        compressTiff
        cd('../..')
    end
    fprintf(['done in ', seconds2human(toc), '\n'])
    
    % Return to home directory
    cd('../..')
end