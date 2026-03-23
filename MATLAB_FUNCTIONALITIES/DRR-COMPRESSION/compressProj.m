function compressProj
fprintf('Processing.....\n')

% List projections
proj_dir = dir('*.hn*');

tic;
for n = 1:length(proj_dir)
    
    % Load hnc
    proj_name = proj_dir(n).name;
    [~, ~, ext] = fileparts(proj_name);

    if all(ext == '.hnc')
        [~, P] = HncRead(proj_name);
    else
        [~, P] = HndRead(proj_name);
    end
      
    % Transpose, resize and reshape
    P = single(P');
    P = log(65536 ./ (P + 1));
    P = P(1:size(P,1)/128:end, 1:size(P,2)/128:end);
    P = reshape(P,[128*128,1]);

    % Save as binary file
    fid = fopen([proj_name(1:end-4), '.bin'],'w');
    fwrite(fid, P, 'single');
    fclose(fid);

    % Delete hnc
    delete(proj_name)

end
fprintf(['=====Projections completed in ', seconds2human(toc),'=====\n'])