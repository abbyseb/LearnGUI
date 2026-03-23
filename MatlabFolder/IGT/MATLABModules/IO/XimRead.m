function [info,M] = XimRead(filename)
%% [info M] = XimRead(filename)
% ------------------------------------------
% FILE   : XimRead.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-10-11  Created.
% ------------------------------------------
% PURPOSE
%   Read Varian XIM image and header.
%   This program was adapted from the original version developed by
%   Fredrik Nordström in 2015.
% ------------------------------------------
% INPUT
%   filename : The full path to the xim file.
% ------------------------------------------
% OUTPUT
%   info  : Header information in a struct.
%   M     : The image stored in a 2D matrix.
% ------------------------------------------

%% Input filename
if nargin < 1
    filename = uigetfilepath( {'*.xim;*.XIM;','Varian XIM Files (*.xim,*.XIM)';
        '*.xim',  'Varian XIM Files (*.xim)'; ...
        '*.XIM',  'Varian XIM Files (*.XIM)'}, ...
        'Select an image file', ...
        pwd);
end

if isnumeric(filename)
    M = []; info = struct([]);
    error('ERROR: No file selected. \n');
end

%% Read header

fid = fopen(filename,'r');

% Decode header
info.file_name = filename;
info.file_format_identifier = fread(fid,8,'*char')';
info.file_format_version = fread(fid,1,'*int32');
info.image_width = fread(fid,1,'*int32');
info.image_height = fread(fid,1,'*int32');
info.bits_per_pixel = fread(fid,1,'*int32');
info.bytes_per_pixel = fread(fid,1,'*int32');
info.compression_indicator = fread(fid,1,'*int32');

%% Read image
if nargout < 2
    read_pixel_data = 0;
else
    read_pixel_data = 1;
end

% Decode pixel data
if info.compression_indicator == 1
    lookup_table_size = fread(fid,1,'int32');    
    lookup_table = fread(fid,lookup_table_size*4,'ubit2=>uint8');
    compressed_pixel_buffer_size = fread(fid,1,'int32');
    if read_pixel_data==1
        % Decompress image
        pixel_data = int32(zeros(info.image_width*info.image_height,1));
        pixel_data(1:info.image_width+1) = fread(fid,info.image_width+1,'*int32');
        lookup_table_pos = 1;
        for image_pos = (info.image_width+2):(info.image_width*info.image_height)      
            if lookup_table(lookup_table_pos) == 0
                diff = int32(fread(fid,1,'*int8'));
            elseif lookup_table(lookup_table_pos) == 1
                diff = int32(fread(fid,1,'*int16'));
            else
                diff = int32(fread(fid,1,'*int32'));
            end
            pixel_data(image_pos) = ...
                diff+pixel_data(image_pos-1) + ...
                pixel_data(image_pos-info.image_width) - ...
                pixel_data(image_pos-info.image_width-1);
            lookup_table_pos = lookup_table_pos + 1;
        end
        
        if info.bytes_per_pixel == 2
            info.pixel_data = ...
                int16(reshape(pixel_data,info.image_width,info.image_height));
        else
            info.pixel_data = ...
                reshape(pixel_data,info.image_width,info.image_height);
        end
    else
        fseek(fid,compressed_pixel_buffer_size,'cof');
    end
    uncompressed_pixel_buffer_size = fread(fid,1,'*int32');
else
    uncompressed_pixel_buffer_size = fread(fid,1,'*int32');
    if read_pixel_data == 1        
        switch info.bytes_per_pixel
            case 1
                pixel_data = fread(fid,uncompressed_pixel_buffer_size,'*int8');
            case 2
                pixel_data = fread(fid,uncompressed_pixel_buffer_size/2,'*int16');
            otherwise
                pixel_data = fread(fid,uncompressed_pixel_buffer_size/4,'*int32');
        end
        info.pixel_data = reshape(pixel_data,info.image_width,info.image_height);
    else
        fseek(fid,uncompressed_pixel_buffer_size,'cof');
    end
end

% Decode histogram
number_of_bins_in_histogram = fread(fid,1,'*int32');
if number_of_bins_in_histogram>0
    info.histogram.number_of_bins_in_histogram=number_of_bins_in_histogram;
    info.histogram.histogram_data = fread(fid,number_of_bins_in_histogram,'*int32');
end

% Decode properties
number_of_properties = fread(fid,1,'*int32');
if number_of_properties>0
    info.properties=[];
end
for property_nr=1:number_of_properties
    property_name_length = fread(fid,1,'*int32');
    property_name = fread(fid,property_name_length,'*char')';
    property_type = fread(fid,1,'*int32');
    switch property_type
        case 0
            property_value = fread(fid,1,'*int32');
        case 1
            property_value = fread(fid,1,'double');
        case 2
            property_value_length = fread(fid,1,'*int32');
            property_value = fread(fid,property_value_length,'*char')';
        case 4
            property_value_length = fread(fid,1,'*int32');
            property_value = fread(fid,property_value_length/8,'double');
        case 5
            property_value_length = fread(fid,1,'*int32');
            property_value = fread(fid,property_value_length/4,'*int32');
        otherwise
            disp(' ')
            disp([property_name ': Property type ' num2str(property_type) ' is not supported! Aborting property decoding!']);
            fclose(fid);
            return;
    end
    info.properties=setfield(info.properties,property_name,property_value);
end

if nargout > 1
    M = info.pixel_data;
    rmfield(info,'pixel_data');
end

fclose(fid);