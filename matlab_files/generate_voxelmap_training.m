function generate_voxelmap_training(base_dir, rt_ct_pres_list, work_root, varargin)
%GENERATE_VOXELMAP_TRAINING
% Voxelmap data preparation pipeline - Session-based design for IEC 62304 compliance
%
% MODES:
%   Pipeline   - Full pipeline execution (copy files, run all steps, archive)
%   RerunStep  - Single step re-execution (Python handles validation/purge)
%
% PARAMETERS:
%   base_dir           - Base directory containing source data (READ-ONLY; e.g. PRJ-RPL).
%                        NEVER write or edit anything under base_dir. All outputs go to work_root.
%   rt_ct_pres_list    - Cell array of rt_ct_pres paths (empty for RerunStep)
%   work_root          - Workspace root directory (contains patient folders). All writes go here.
%   LogFile            - Path to log file (required)
%   SkipCopy           - Skip DICOM copy step (default: false)
%   SkipDICOM2MHA      - Skip DICOM to MHA step (default: false)
%   SkipDownsample     - Skip Downsample step (default: false)
%   SkipDRR            - Skip DRR generation (default: false)
%   SkipCompress       - Skip DRR compression (default: false)
%   Skip2DDVF          - Skip 2D DVF generation (default: false)
%   Skip3DLow          - Skip 3D DVF (low-res) generation (default: false)
%   Skip3DFull         - Skip 3D DVF (full-res) generation (default: false)
%   SkipKVPreprocess   - Skip kV Preprocessing step (default: false)
%   Mode               - 'Pipeline' or 'RerunStep' (default: 'Pipeline')
%   StartAt            - Step to rerun (required if Mode='RerunStep')
%

%% ============================================================================
%% PIPELINE DEFINITION (FIXED)
%% ============================================================================
% Central definition of pipeline steps.
global pipeline_steps;

if isempty(pipeline_steps)
    pipeline_steps = containers.Map('KeyType', 'char', 'ValueType', 'any');
    pipeline_steps('copy')         = @copy_training_files;
    pipeline_steps('dicom2mha')    = @DicomToMha_VALKIM;
    pipeline_steps('downsample')   = @downsampleVALKIMvolumes;
    pipeline_steps('drr')          = @getVALKIMprojections;
    pipeline_steps('compress')     = @compressProj;
    pipeline_steps('dvf2d')        = @getVALKIM2DDVF;
    pipeline_steps('dvf3dlow')     = @getVALKIM3DDVF;
    pipeline_steps('dvf3dfull')    = @VALKIM3DDVF_full;
    pipeline_steps('kvpreprocess') = @step_kv_preprocess;
end

%% ============================================================================
%% INPUT PARSING & VALIDATION
%% ============================================================================

p = inputParser;
p.addRequired('base_dir', @(s)ischar(s)||isstring(s));
p.addRequired('rt_ct_pres_list', @(c)ischar(c)||isstring(c)||iscellstr(c)||isstring(c)||isempty(c));
p.addRequired('work_root', @(s)ischar(s)||isstring(s));
p.addParameter('LogFile', '', @(s)ischar(s)||isstring(s));
p.addParameter('SkipDRR', false, @islogical);
p.addParameter('SkipCompress', false, @islogical);
p.addParameter('Skip2DDVF', false, @islogical);
p.addParameter('Skip3DLow', false, @islogical);
p.addParameter('Skip3DFull', false, @islogical);
p.addParameter('SkipCopy', false, @islogical);
p.addParameter('SkipDICOM2MHA', false, @islogical);
p.addParameter('SkipDownsample', false, @islogical);
p.addParameter('SkipKVPreprocess', false, @islogical);
p.addParameter('Mode', 'Pipeline', @(s)ischar(s)||isstring(s));
p.addParameter('StartAt', '', @(s)ischar(s)||isstring(s));
p.parse(base_dir, rt_ct_pres_list, work_root, varargin{:});
opt = p.Results;

% Normalize inputs
base_dir  = char(base_dir);
work_root = char(work_root);

if isempty(rt_ct_pres_list)
    rt_ct_pres_list = {};
elseif ischar(rt_ct_pres_list) || isstring(rt_ct_pres_list)
    rt_ct_pres_list = {char(rt_ct_pres_list)};
end

mode = upper(strtrim(string(opt.Mode)));

% Validate mode
if mode ~= "PIPELINE" && mode ~= "RERUNSTEP"
    error('generate_voxelmap_training:InvalidMode', ...
          'Invalid Mode: "%s". Must be "Pipeline" or "RerunStep".', mode);
end

% Validate RerunStep requirements
if mode == "RERUNSTEP"
    if strlength(strtrim(string(opt.StartAt))) == 0
        error('generate_voxelmap_training:MissingStartAt', ...
              'When Mode="RerunStep", you must provide StartAt parameter.');
    end
    start_key = normalize_start_key(opt.StartAt);
    
    % Rerun of COPY or KVPREPROCESS requires rt_ct_pres_list
    if (strcmpi(start_key, 'copy') || strcmpi(start_key, 'kvpreprocess')) && isempty(rt_ct_pres_list)
         error('generate_voxelmap_training:MissingRtCtPresForRerun', ...
               'Rerun of %s step requires original rt_ct_pres_list.', start_key);
    end
    
else
    start_key = '';
    if isempty(rt_ct_pres_list)
        error('generate_voxelmap_training:MissingRtCtPres', ...
              'Pipeline mode requires non-empty rt_ct_pres_list.');
    end
end

% Safety check: work_root must not be inside base_dir
base_dir_canonical  = canonical_path(base_dir);
work_root_canonical = canonical_path(work_root);

if startsWith(work_root_canonical, base_dir_canonical, 'IgnoreCase', true)
    error('generate_voxelmap_training:UnsafeWorkRoot', ...
          'Safety check failed: work_root must not be inside base_dir.');
end

% PRJ-RPL / base_dir is READ-ONLY: never write or edit anything under base_dir.
assert_not_under_base_dir(work_root_canonical, base_dir_canonical);

%% ============================================================================
%% LOGGING SETUP
%% ============================================================================
% Python host captures stdout. No diary needed.
t_all = tic;

% Store original directory and ensure we return to it
orig_cwd = pwd;
cleanup_cwd = onCleanup(@()cd_safe(orig_cwd, true)); % Use cd_safe to return

%% ============================================================================
%% MAIN EXECUTION LOOP
%% ============================================================================

if mode == "PIPELINE"
    num_cases = numel(rt_ct_pres_list);
else
    num_cases = 1;  % RerunStep processes one patient folder
end

for idx = 1:num_cases
    try
        %% ====================================================================
        %% PATH RESOLUTION
        %% ====================================================================
        
        rt_ct_pres = '';
        if ~isempty(rt_ct_pres_list)
             rt_ct_pres = char(rt_ct_pres_list{idx});
        end
        
        if mode == "PIPELINE"
            log_msg('Processing rt_ct_pres: %s', rt_ct_pres);
            [patient_root, train_dir, src_train] = resolve_pipeline_paths(...
                rt_ct_pres, base_dir, work_root);
            
        else % RerunStep mode
            patient_root = find_patient_folder(work_root);
            if isempty(patient_root)
                error('generate_voxelmap_training:NoPatientFolder', ...
                      'Could not find patient folder in work_root for rerun.');
            end
            train_dir = fullfile(patient_root, 'train');
            
            % Re-resolve src_train path for 'COPY' or 'KV' step rerun
            if strcmpi(start_key, 'copy') || strcmpi(start_key, 'kvpreprocess')
                [~, ~, src_train] = resolve_pipeline_paths(rt_ct_pres, base_dir, work_root);
            else
                 src_train = ''; % Not needed for other reruns
            end
            log_msg('Found patient folder for rerun: %s', patient_root);
        end
        
        % Validate train directory
        if ~exist(train_dir, 'dir')
            if mode == "PIPELINE"
                mkdir_safe(patient_root);
                mkdir_safe(train_dir);
            else
                % For rerun, Python should have ensured dir exists if needed
                error('generate_voxelmap_training:NoTrainDir', ...
                      'train/ directory not found: %s', train_dir);
            end
        end
        log_msg('Using train directory: %s', train_dir);
        
        %% ====================================================================
        %% MODE-SPECIFIC EXECUTION
        %% ====================================================================
        
        if mode == "RERUNSTEP"
            %% ----------------------------------------------------------------
            %% RERUN STEP MODE
            %% ----------------------------------------------------------------
            cd_safe(train_dir); % Change to train dir for execution
            assert_in_workroot(train_dir, work_root_canonical);
            
            log_msg('Executing step: %s', upper(start_key));
            log_msg('Prerequisites and outputs managed by Python host.');
            
            % Execute the single step
            run_single_step(start_key, opt, src_train, train_dir, rt_ct_pres, base_dir, patient_root, work_root_canonical);
            
            cd_safe(orig_cwd); % Return to original dir
            log_msg('Rerun step complete: %s', upper(start_key));
            
        else
            %% ----------------------------------------------------------------
            %% PIPELINE MODE
            %% ----------------------------------------------------------------
            
            % Navigate to train directory
            cd_safe(train_dir); % Change to train dir for execution
            assert_in_workroot(train_dir, work_root_canonical);
            
            % Step 1-N: Execute full pipeline
            log_msg('EXECUTING PIPELINE STEPS');
            run_full_pipeline(opt, src_train, train_dir, rt_ct_pres, base_dir, patient_root, work_root_canonical);
            
            cd_safe(orig_cwd); % Return to original dir
            log_msg('CASE COMPLETE: %s', basename(patient_root));
        end
        
    catch ME
        cd_safe(orig_cwd, true); % Force return to original dir on error
        log_msg('ERROR in case %d: %s', idx, ME.message);
        warning('generate_voxelmap_training:CaseFailed', ...
                'Case %d failed: %s', idx, ME.message);
        fprintf(2, '%s\n', getReport(ME, 'extended'));
        exit(1);
    end
end

%% ============================================================================
%% SESSION COMPLETION
%% ============================================================================

log_msg('Total processing time: %s', seconds2human(toc(t_all)));

end % main function

%% ============================================================================
%% PIPELINE EXECUTION FUNCTIONS (REWRITTEN)
%% ============================================================================

function run_full_pipeline(opt, src_train, train_dir, rt_ct_pres, base_dir, patient_root, work_root_canonical)
    % This function now iterates through a defined order of steps,
    % calling the functions from the centralized pipeline_steps map.
    
    global pipeline_steps;
    
    % Define the execution order for a full pipeline run
    step_order = {'copy', 'dicom2mha', 'downsample', 'drr', 'compress', 'dvf2d', 'dvf3dlow', 'dvf3dfull', 'kvpreprocess'};
    
    % Define which steps are controlled by 'Skip' options
    skip_flags = containers.Map;
    skip_flags('copy')         = opt.SkipCopy;
    skip_flags('dicom2mha')    = opt.SkipDICOM2MHA;
    skip_flags('downsample')   = opt.SkipDownsample;
    skip_flags('drr')          = opt.SkipDRR;
    skip_flags('compress')     = opt.SkipCompress;
    skip_flags('dvf2d')        = opt.Skip2DDVF;
    skip_flags('dvf3dlow')     = opt.Skip3DLow;
    skip_flags('dvf3dfull')    = opt.Skip3DFull;
    skip_flags('kvpreprocess') = opt.SkipKVPreprocess;
    
    % Define user-friendly names for logging
    step_names = containers.Map;
    step_names('copy')         = 'Copying training files...';
    step_names('dicom2mha')    = 'Converting DICOM to MHA...';
    step_names('downsample')   = 'Downsampling MHAs...';
    step_names('drr')          = 'Generating DRRs...';
    step_names('compress')     = 'Compressing DRRs...';
    step_names('dvf2d')        = 'Generating 2D DVFs...';
    step_names('dvf3dlow')     = 'Generating 3D DVFs (downsampled)...';
    step_names('dvf3dfull')    = 'Generating 3D DVFs (full resolution)...';
    step_names('kvpreprocess') = 'Finalize & Process kV...';
    
    for i = 1:numel(step_order)
        step_key = step_order{i};
        
        % Check if the step should be skipped
        if isKey(skip_flags, step_key) && skip_flags(step_key)
            log_msg('Skipping %s (per option)', upper(step_key));
            continue;
        end
        
        % Execute the step
        log_msg(step_names(step_key));
        t = tic;
        
        % --- MODIFIED: Handle steps with different arguments ---
        step_function = pipeline_steps(step_key);
        switch step_key
            case 'copy'
                step_function(src_train, train_dir);
            case 'kvpreprocess'
                step_function(rt_ct_pres, base_dir, patient_root, work_root_canonical);
            otherwise
                step_function(); % Execute standard function handle
        end
        % --- END MODIFIED ---
        
        log_msg('%s: done in %s', upper(step_key), seconds2human(toc(t)));
        
        % --- FIX: REMOVED FAULTY CONDITIONAL LOGIC ---
        
        fprintf('%%STEP_COMPLETE:%s%%\n', upper(step_key));
    end
end

function run_single_step(step_key, opt, src_train, train_dir, rt_ct_pres, base_dir, patient_root, work_root_canonical)
    % This function now dynamically looks up the step function
    % from the centralized map instead of using a switch-case block.
    
    global pipeline_steps;
    
    log_msg('Executing single step: %s', upper(step_key));
    
    step_key_lower = lower(step_key);
    
    if ~isKey(pipeline_steps, step_key_lower)
        error('generate_voxelmap_training:UnknownStep', ...
              'Unknown step: %s', step_key);
    end
    
    % Execute the step function from the map
    t = tic;
    
    % --- MODIFIED: Handle steps with different arguments ---
    step_function = pipeline_steps(step_key_lower);
    switch step_key_lower
        case 'copy'
            step_function(src_train, train_dir);
        case 'kvpreprocess'
            step_function(rt_ct_pres, base_dir, patient_root, work_root_canonical);
        otherwise
            step_function(); % Execute standard function handle
    end
    % --- END MODIFIED ---
    
    log_msg('%s: done in %s', upper(step_key), seconds2human(toc(t)));
    
    % --- FIX: REMOVED FAULTY CONDITIONAL LOGIC ---
    
    fprintf('%%STEP_COMPLETE:%s%%\n', upper(step_key));
end

%% ============================================================================
%% PIPELINE STEP FUNCTIONS (NEW/MODIFIED)
%% ============================================================================

function copy_training_files(src_train, train_dir)
% Step 1: Copy training files from source to train directory
    log_msg('Source: %s', src_train);
    log_msg('Destination: %s', train_dir);
    if isempty(src_train)
        error('generate_voxelmap_training:MissingSrcTrain', ...
              'Source training path is empty, cannot copy.');
    end
    if ~exist(src_train, 'dir')
        error('generate_voxelmap_training:SrcTrainNotFound', ...
              'Source training directory not found: %s', src_train);
    end
    copyfile(src_train, train_dir);
end

function step_kv_preprocess(rt_ct_pres, base_dir, patient_root, work_root_canonical)
% Step 9: Finalize run (archive, set source) AND process kV images
    
    % --- CORRECTED FINALIZATION LOGIC ---
    log_msg('Finalizing run: Setting exhale reference...');
    choose_exhale_reference();
    
    log_msg('Finalizing run: Archiving intermediate files...');
    archive_intermediates(patient_root, work_root_canonical);
    % --- END FINALIZATION ---
    
    if isempty(rt_ct_pres)
         error('generate_voxelmap_training:MissingRtCtPresForKV', ...
               'rt_ct_pres is required for KV Preprocessing step.');
    end
    
    % Step 9a: Copy test data
    log_msg('Copying test (Treatment files)...');
    t = tic;
    
    im_dir_parts = split(rt_ct_pres, '/');
    im_dir_parts{4} = 'Treatment files';
    im_dir_parts{1} = '';
    im_dir_parts{end} = '';
    im_dir_test_rel = strjoin(im_dir_parts(2:end-1), filesep);
    src_test = fullfile(base_dir, im_dir_test_rel);
    
    test_dir = fullfile(patient_root, 'test');
    mkdir_safe(test_dir);
    
    log_msg('Source: %s', src_test);
    log_msg('Destination: %s', test_dir);
    copyfile(src_test, test_dir);
    log_msg('Copying test: done in %s', seconds2human(toc(t)));

    % Step 9b: Pre-process kV images
    log_msg('Pre-processing kV images (TIFF->BIN)...');
    t = tic;
    kv_preprocess(test_dir, work_root_canonical);
    log_msg('Pre-processing kV: done in %s', seconds2human(toc(t)));
end


%% ============================================================================
%% INTERMEDIATE FILE MANAGEMENT
%% ============================================================================

function archive_intermediates(patient_root, work_root_canonical)
    train_dir = fullfile(patient_root, 'train');
    archive_dir = fullfile(patient_root, 'intermediates');
    
    assert_in_workroot(train_dir, work_root_canonical);
    assert_in_workroot(archive_dir, work_root_canonical);
    
    if ~exist(archive_dir, 'dir')
        mkdir_safe(archive_dir);
    end
    
    log_msg('Archiving intermediate files to intermediates/...');
    archived_count = 0;
    
    % Archive DICOMs with directory structure preservation
    dcm_patterns = {'*.dcm', '*.dicom', '*.ima'};
    for i = 1:numel(dcm_patterns)
        files = dir(fullfile(train_dir, '**', dcm_patterns{i}));
        for j = 1:numel(files)
            if files(j).isdir
                continue;
            end
            
            src = fullfile(files(j).folder, files(j).name);
            
            % Create relative path for intermediates
            rel_path_parts = strsplit(files(j).folder, filesep);
            train_idx = find(strcmp(rel_path_parts, 'train'), 1, 'last');
            if isempty(train_idx) || train_idx == numel(rel_path_parts)
                rel_path = '';
            else
                rel_path = fullfile(rel_path_parts{train_idx+1:end});
            end
            
            dest_dir = fullfile(archive_dir, rel_path);
            if ~exist(dest_dir, 'dir')
                mkdir_safe(dest_dir);
            end
            
            dest = fullfile(dest_dir, files(j).name);
            try
                movefile(src, dest, 'f');
                archived_count = archived_count + 1;
            catch ME
                warning('generate_voxelmap_training:ArchiveMoveFailed', ...
                        'Failed to archive %s: %s', files(j).name, ME.message);
            end
        end
    end
    
    % Archive full-resolution MHA files
    mha_list = dir(fullfile(train_dir, '*.mha'));
    for k = 1:numel(mha_list)
        nm = mha_list(k).name;
        
        if startsWith(nm, 'sub_', 'IgnoreCase', true) || ...
           startsWith(nm, 'source', 'IgnoreCase', true)
            continue;
        end
        
        src = fullfile(train_dir, nm);
        dest = fullfile(archive_dir, nm);
        try
            movefile(src, dest, 'f');
            archived_count = archived_count + 1;
        catch ME
            warning('generate_voxelmap_training:ArchiveMoveFailed', ...
                    'Failed to archive %s: %s', nm, ME.message);
        end
    end
    
    log_msg('Archived %d files to intermediates/', archived_count);
end

%% ============================================================================
%% PATH RESOLUTION HELPERS
%% ============================================================================

function [patient_root, train_dir, src_train] = resolve_pipeline_paths(rt_ct_pres, base_dir, work_root)
    im_dir_parts = split(rt_ct_pres, '/');
    if numel(im_dir_parts) < 5
        error('generate_voxelmap_training:MalformedRtCtPres', ...
              'Malformed rt_ct_pres (expected at least 5 parts): %s', rt_ct_pres);
    end
    
    im_dir_parts{4} = 'MAGIKmodel';
    im_dir_parts{end} = 'FilesForModelling';
    im_dir_train_rel = strjoin(im_dir_parts, filesep);
    src_train = [base_dir, im_dir_train_rel];
    
    folder_name_parts = strsplit(im_dir_train_rel, filesep);
    if numel(folder_name_parts) < 2
        error('generate_voxelmap_training:UnexpectedPathDepth', ...
              'Unexpected path depth: %s', im_dir_train_rel);
    end
    patient_folder = folder_name_parts{end-1};
    
    patient_root = fullfile(work_root, patient_folder);
    train_dir = fullfile(patient_root, 'train');
end

function patient_folder = find_patient_folder(work_root)
    patient_folder = '';
    
    d = dir(work_root);
    for i = 1:numel(d)
        if d(i).isdir && ...
           ~startsWith(d(i).name, '.') && ...
           ~strcmp(d(i).name, 'logs') && ...
           ~strcmp(d(i).name, '_matlab_prefs') && ...
           ~strcmp(d(i).name, '..') && ...
           ~strcmp(d(i).name, '.')
            patient_folder = fullfile(work_root, d(i).name);
            return;
        end
    end
end

%% ============================================================================
%% POST-PROCESSING (now called conditionally)
%% ============================================================================

function choose_exhale_reference()
    if exist('CT_06.mha', 'file')
        try
            copyfile('CT_06.mha', 'source.mha', 'f');
            log_msg('Set source.mha from CT_06.mha (exhale reference)');
        catch ME
            warning('generate_voxelmap_training:SourceCopyFailed', ...
                    'Failed to set source.mha: %s', ME.message);
        end
    end
end

function kv_preprocess(test_dir, work_root_canonical)
    if ~exist(test_dir, 'dir')
        log_msg('  Test directory not found, skipping kV preprocess: %s', test_dir);
        return;
    end
    d = dir(fullfile(test_dir, 'Fx*'));
    
    for i = 1:numel(d)
        kvdir = fullfile(test_dir, d(i).name, 'kV');
        
        if exist(kvdir, 'dir')
            assert_in_workroot(kvdir, work_root_canonical);
            
            cwd2 = pwd;
            cd_safe(kvdir); % Use safe cd
            
            try
                compressTiff;
                log_msg('  Processed kV images: %s', d(i).name);
            catch ME
                warning('generate_voxelmap_training:CompressTiffFailed', ...
                        'compressTiff failed in %s: %s', kvdir, ME.message);
            end
            
            cd_safe(cwd2); % Use safe cd
        end
    end
end

%% ============================================================================
%% UTILITY FUNCTIONS
%% ============================================================================

function log_msg(fmt, varargin)
%LOG_MSG Log a message without a timestamp (Python host handles timestamping)
    fprintf('%s\n', sprintf(fmt, varargin{:}));
end

function mkdir_safe(p)
    if ~exist(p, 'dir')
        [status, msg] = mkdir(p);
        if ~status
            error('generate_voxelmap_training:MkdirFailed', ...
                  'Failed to create directory: %s. Reason: %s', p, msg);
        end
    end
end

function cd_safe(p, is_cleanup)
%CD_SAFE Change directory safely, with retries for filesystem lag.
    if nargin < 2
        is_cleanup = false;
    end

    for i = 1:5
        if exist(p, 'dir')
            try
                cd(p);
                return; % Success
            catch ME_cd
                 if is_cleanup
                    warning('generate_voxelmap_training:CleanupCdFailed', ...
                            'Cleanup cd to %s failed: %s', p, ME_cd.message);
                    return; % Don't error during cleanup
                 else
                    error('generate_voxelmap_training:CdFailed', ...
                          'Failed to cd to %s: %s', p, ME_cd.message);
                 end
            end
        end
        % Directory not found, pause and retry
        pause(0.2);
    end
    
    % Failed after retries
    if is_cleanup
        warning('generate_voxelmap_training:CleanupDirNotFound', ...
                'Cleanup directory not found: %s', p);
    else
        error('generate_voxelmap_training:DirectoryNotFound', ...
              'Directory does not exist: %s', p);
    end
end

function s = seconds2human(sec)
    if sec < 60
        s = sprintf('%.1f s', sec);
        return;
    end
    
    m = floor(sec / 60);
    s_rem = sec - 60 * m;
    
    if m < 60
        s = sprintf('%d min %.0f s', m, s_rem);
        return;
    end
    
    h = floor(m / 60);
    m = m - 60 * h;
    s = sprintf('%d h %d min', h, m);
end

function full = canonical_path(p)
    try
        full = char(java.io.File(p).getCanonicalPath());
    catch
        full = char(p);
    end
end

function assert_in_workroot(path_to_check, work_root_canonical)
    here = canonical_path(path_to_check);
    
    if ~startsWith(here, work_root_canonical, 'IgnoreCase', true)
        error('generate_voxelmap_training:SafetyCheckFailed', ...
              'Safety check failed: path not in work_root: %s', here);
    end
end

% PRJ-RPL / base_dir is READ-ONLY. Ensure no write target is under base_dir.
function assert_not_under_base_dir(path_to_check, base_dir_canonical)
    here = canonical_path(path_to_check);
    if startsWith(here, base_dir_canonical, 'IgnoreCase', true)
        error('generate_voxelmap_training:PRJ_RPL_ReadOnly', ...
              'Safety check failed: must never write under base_dir (PRJ-RPL read-only): %s', here);
    end
end

function name = basename(path)
    [~, name, ~] = fileparts(path);
    if isempty(name)
        parts = strsplit(path, {'\', '/'});
        parts = parts(~cellfun(@isempty, parts));
        if ~isempty(parts)
            name = parts{end};
        else
            name = path;
        end
    end
end

function key = normalize_start_key(s)
    s = lower(strtrim(string(s)));
    s = strrep(s, ' ', '');
    s = strrep(s, '-', '');
    s = strrep(s, '_', '');
    s = strrep(s, '->', '');
    s = strrep(s, '→', '');
    
    switch s
        case {"copy", "copytrainingfiles"}
            key = 'copy';
        case {"dicom2mha", "dicommha", "dicomtomha", "dicom→mha"}
            key = 'dicom2mha';
        case {"downsample", "downsamplemha", "downsamplevolumes"}
            key = 'downsample';
        case {"drr", "drrs", "generatedrr", "generatedrrs"}
            key = 'drr';
        case {"compress", "compressdrr", "compressdrrs"}
            key = 'compress';
        case {"dvf2d", "2ddvf", "2ddvfs"}
            key = 'dvf2d';
        case {"dvf3dlow", "3ddvflow", "3ddvflowres"}
            key = 'dvf3dlow';
        case {"dvf3dfull", "3ddvffull", "3ddvffullres"}
            key = 'dvf3dfull';
        case {"kvpreprocess", "kvtiff", "kvbin", "kvtiff→bin", "finalize&processkv"}
            key = 'kvpreprocess';
        otherwise
            error('generate_voxelmap_training:UnknownStartAt', ...
                  'Unknown StartAt value: %s', s);
    end
end

