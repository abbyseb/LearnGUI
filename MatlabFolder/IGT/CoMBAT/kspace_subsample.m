function [subsampled_kspace_data, header]  = kspace_subsample(kspace_data, header)

% The index for ACS lines in the PE direction
header.center_locs =transpose( [floor(header.Npe/2 - header.central_kspace/2 + 1):floor(header.Npe / 2 + header.central_kspace/2)]); 

% The index for subsampled PE lines in the row direction
header.sampling_location = 1:header.subsampling_factor:header.Npe;
if ismember(header.center_locs(1) - 1, header.sampling_location)
    header.center_locs = union(header.center_locs(1) - 1, header.center_locs);
end    
if ismember(header.center_locs(end) + 1, header.sampling_location)
    header.center_locs = union(header.center_locs, header.center_locs(end) + 1);
end    

% The index of reference blocks within ACS lines.
header.temp = intersect(header.sampling_location, header.center_locs);

% The index of subsampled PE and ACS lines
header.sampling_location = union(header.sampling_location, header.center_locs);

% Subsampling k-space data along row-direction. ACS lines are included in the subsampled data. The size of PE lines is incremented by one in order to reconstruct data in the final block
subsampled_kspace_data = zeros(header.Npe + 1, header.Nfe, header.num_coils);
for index = 1: header.num_coils
    subsampled_kspace_data(header.sampling_location, :, index) = kspace_data(header.sampling_location, :, index);
end
return;
