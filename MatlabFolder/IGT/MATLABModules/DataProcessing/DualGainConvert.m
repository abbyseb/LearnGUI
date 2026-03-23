function DualGainConvert(inputfl,outputdir,option,deadcols)
%% DualGainConvert(inputfl,outputdir,option,deadcols)
% ------------------------------------------
% FILE   : DualGainConvert.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2012-10-26  Created
% ------------------------------------------
% PURPOSE
%   Convert a Varian Hnc dual gain images to normal hnc format and write it 
%   to the output folder.
%
%   DualGainConvert(inputfl,outputdir,option)
%   DualGainConvert(inputfl,outputdir,option,deadcols)
% ------------------------------------------
% INPUT
%   inputfl:        The input file list (can be obtained using ls('*.hnc'))
%   outputdir:      The output directory.
%   option:         Whether to use the 'highgain','lowgain', or 'combine'.
%                   Default is 'highgain'.
%   deadcols:       A matrix containing dead columnss information.
%                       [ColumnNumber1 StartRow1 EndRow1;
%                        ColumnNumber2 StartRow2 EndRow2; ...]
%                   (optional)
% ------------------------------------------

%% Combining high and low gain images
fsep = fsepcp;
if length(outputdir) == 0
    error('ERROR: Please select an output directory different to the input directory');
    return
end

nproj = size(inputfl);  nproj = nproj(1);

for k = 1:nproj
  
    % Open the file
    [info,M] = HncRead(inputfl(k,:));
    pixeltype = class(M);
    
    height = info.uiSizeY / 2;
    
    high_gain = M(:,[1:2:(height-1) (height+2):2:2*height]);
    low_gain = M(:,[2:2:height (height+1):2:(2*height-1)]);
    
    switch (option)
        case 'lowgain'
            M = double(low_gain)
        case 'combine'
            % Should be multiplication, which is equivalent to addition of the
            % attenuation
            M = double(high_gain) .* double(low_gain);
            %M = double(high_gain) + double(low_gain);
        otherwise
            M = double(high_gain);
    end
    
    % Fixing the start and end row
    M(:,1) = M(:,2);
    M(:,end) = M(:,end-1);
    
    % Fixing dead columns
    
    if nargin >= 4
        [Ncols tmp] = size(deadcols);
        
        % Check if dead column information is given correctly
        if tmp ~= 3
            error('ERROR: Incorrect dead columns information format.');
            return
        end
        
        % Fix the dead rows using neighboring average
        for kcol = 1:Ncols
            M(deadcols(kcol,1),deadcols(kcol,2):deadcols(kcol,3)) = ...
                0.5 * M(deadcols(kcol,1)-1,deadcols(kcol,2):deadcols(kcol,3)) + ...
                0.5 * M(deadcols(kcol,1)+1,deadcols(kcol,2):deadcols(kcol,3));
        end
    end
    
    % Geoffrey suggests using a median filter
    M = medfilt2(M,[3 3]);
    
    
    % Writing into a new Hnc/Hnd
    
    info.('uiSizeY') = height;
    
    switch pixeltype
        case 'uint8'
            info.('uiFileLength') = info.('uiFileLength') - length(M(:));
        case 'uint16'
            info.('uiFileLength') = info.('uiFileLength') - 2*length(M(:));
        case 'uint32'
            info.('uiFileLength') = info.('uiFileLength') - 4*length(M(:));
    end
    
    fid = fopen([outputdir,fsep,inputfl(k,:)],'w');
    if fid == -1
        error('ERROR: Error writing to the specified path.');
        return
    end
    
    % Writing header file
    fwrite(fid,info.bFileType,'uint8');
    fwrite(fid,info.uiFileLength,'uint32');
    fwrite(fid,info.bChecksumSpec,'uint8');
    fwrite(fid,info.uiCheckSum,'uint32');
    fwrite(fid,info.bCreationDate,'uint8');
    fwrite(fid,info.bCreationTime,'uint8');
    fwrite(fid,info.bPatientID,'uint8');
    fwrite(fid,info.uiPatientSer,'uint32');
    fwrite(fid,info.bSeriesID,'uint8');
    fwrite(fid,info.uiSeriesSer,'uint32');
    fwrite(fid,info.bSliceID,'uint8');
    fwrite(fid,info.uiSliceSer,'uint32');
    fwrite(fid,info.uiSizeX,'uint32');
    fwrite(fid,info.uiSizeY,'uint32');
    fwrite(fid,info.dSliceZPos,'double');
    fwrite(fid,info.bModality,'uint8');
    fwrite(fid,info.uiWindow,'uint32');
    fwrite(fid,info.uiLevel,'uint32');
    fwrite(fid,info.uiPixelOffset,'uint32');
    fwrite(fid,info.bImageType,'uint8');
    fwrite(fid,info.dGantryRtn,'double');
    fwrite(fid,info.dSAD,'double');
    fwrite(fid,info.dSFD,'double');
    fwrite(fid,info.dCollX1,'double');
    fwrite(fid,info.dCollX2,'double');
    fwrite(fid,info.dCollY1,'double');
    fwrite(fid,info.dCollY2,'double');
    fwrite(fid,info.dCollRtn,'double');
    fwrite(fid,info.dFieldX,'double');
    fwrite(fid,info.dFieldY,'double');
    fwrite(fid,info.dBladeX1,'double');
    fwrite(fid,info.dBladeX2,'double');
    fwrite(fid,info.dBladeY1,'double');
    fwrite(fid,info.dBladeY2,'double');
    fwrite(fid,info.dIDUPosLng,'double');
    fwrite(fid,info.dIDUPosLat,'double');
    fwrite(fid,info.dIDUPosVrt,'double');
    fwrite(fid,info.dIDUPosRtn,'double');
    fwrite(fid,info.dPatientSupportAngle,'double');
    fwrite(fid,info.dTableTopEccentricAngle,'double');
    fwrite(fid,info.dCouchVrt,'double');
    fwrite(fid,info.dCouchLng,'double');
    fwrite(fid,info.dCouchLat,'double');
    fwrite(fid,info.dIDUResolutionX,'double');
    fwrite(fid,info.dIDUResolutionY,'double');
    fwrite(fid,info.dImageResolutionX,'double');
    fwrite(fid,info.dImageResolutionY,'double');
    fwrite(fid,info.dEnergy,'double');
    fwrite(fid,info.dDoseRate,'double');
    fwrite(fid,info.dXRayKV,'double');
    fwrite(fid,info.dXRayMA,'double');
    fwrite(fid,info.dMetersetExposure,'double');
    fwrite(fid,info.dAcqAdjustment,'double');
    fwrite(fid,info.dCTProjectionAngle,'double');
    fwrite(fid,info.dCTNormChamber,'double');
    fwrite(fid,info.dGatingTimeTag,'double');
    fwrite(fid,info.dGating4DInfoX,'double');
    fwrite(fid,info.dGating4DInfoY,'double');
    fwrite(fid,info.dGating4DInfoZ,'double');
    fwrite(fid,info.dGating4DInfoTime,'double');
    % Not sure what the rest of the header corresponds to yet
    fwrite(fid,info.dUnknownField1,'double');
    fwrite(fid,info.dUnknownField2,'double');
    fwrite(fid,info.dUnknownField3,'double');
    
    % Write the image body

    % Normalize to prevent exceeding limit
    maxvalue = max(M(:));
    switch pixeltype
        case 'uint8'
            if (maxvalue > 2^8-1)
                M = M * (2^8-1) / maxvalue;
            end
            fwrite(fid,M(:),'uint8');
        case 'uint16'
            if (maxvalue > 2^16-1)
                M = M * (2^16-1) / maxvalue;
            end
            fwrite(fid,M(:),'uint16');
        case 'uint32'
            if (maxvalue > 2^32-1)
                M = M * (2^32-1) / maxvalue;
            end
            fwrite(fid,M(:),'uint32');
    end
    
    fclose(fid);
    
end

return
