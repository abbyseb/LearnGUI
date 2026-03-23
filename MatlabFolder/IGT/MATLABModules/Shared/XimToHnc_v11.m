%% CONVERTS XIM TO HNC FILES. 

% E.g: XimToHnc_v11('./test/',1) OR XimToHnc_v11('./test/',0)
% For 800 XIM files, conversion takes about 13mn. 
% 1. Conversion from XIM to HNC
%    --> Attention: folder name should be in this format absolutely : ./FOLDER/
% 2. End of FOR loop, choice of reading HNC images while saving as BMP
%    If there are files HNC already in it, it will read them all.
% If Header needs to be read use XimReadInfo. 
% If Header for HNC needs to be read: use [header] =
% HncRead('./forldername/')

% ATTENTION WITH THE HNC WRITE: a lot of information are filled with
% dummies values like 'AAAAAA' or '1.11111' values. Those informations couldn't be
% found in the XIM files. If need to be completed: find the name of the
% variable in the XIM files and match it with info.name in the HncWrite
% function.
% 
% LINE 44: The images from TRUEBEAM have some kind of saturation related to the
% gantry angle. Still no explanation why. I apply a sin^2 threshold to it
% that work just fine. 
% 

function XimToHnc_v11(input, Read)
tic 
% Counting the number of file with extension XIM
dirListing = dir(strcat(input,'*.xim')); 
h = waitbar(0,'Converting from XIM to HNC files...'); %Create WAITING bar
output = strcat(input,'HNC/'); %create an output to store HNC
exist(output) == 0 && mkdir(output);

disp(strcat('HNC files store in  in ', output));

  for k=1:length(dirListing)-1 %Sometimes the last image is a few kb and has no data in it
        filename = strcat(input,dirListing(k).name);
        waitbar(k / length(dirListing))

        %Read the raw Data with ReadXim2
        M = ReadXim2(filename);
        M= transpose(M);
        
        % Need to change the threshold with Gtry Angle
        % sin^2 created to vary this threshold
        
        
        %thres= 2^32-1 %%32 bits: 
        % thres = 2^16-1 %%16 bits: 
        
        thres = sin(k*0.4*6.28/360)^2*2800+200;
        M(M>th res)=thres;

        %Read the info
        Info = ximinfo(filename);
     
        %Concatenate 'M' and 'info' together
        % Write the HNCs
        output1 = strcat(output,'/FrameHNC01',num2str(k-1));
        output2 = strcat(output1,'.hnc');
        HncWrite_v4_vc(Info, M, output2);
    
        fclose('all');
  end %End of FOR loop. All HNCs created
  disp('HNC files all created');
  close(h)  %Not closing h will error message out of memory after 400ish images
  
  
%   Option '1' or '0'. 
%   Read the data for confirmation. Read the headers.
  if Read == true
        disp(strcat('HNC files stored as BMP in ', output,'./BMP/'));
        ReadHnc(output)
  end
    
toc
  
end

function ximage = ReadXim2(varargin)

dataType      = {'uint8','uint16','error','uint32'};

if nargin==0
    [fname, pname]= uigetfile('*.xim');
    xfile=fullfile(pname,fname);
else
    xfile=varargin{1};
end

fid=fopen(xfile,'r');

% section 1 - header
fmtID     = fread(fid,8,'*char')'; %row vector
version   = fread(fid,1,'*uint32');
sizeX     = fread(fid,1,'uint32'); 
sizeY     = fread(fid,1,'uint32');
bits      = fread(fid,1,'*uint32');
bytes     = fread(fid,1,'*uint32');
compress  = fread(fid,1,'*uint32');  %0-uncompressed, 1-HND compression

% section 2 - data
pix           = zeros(sizeX*sizeY, 1);

if compress==0
    ucBufSizee  = fread(fid,1,'*uint32');  
    pix = fread(fid, sizeX*sizeY, dataType{bytes}); 
    % should be here...
    CP_end  = ucBufSizee + 36;
else

    %hnd compression (Thanos)
    lutSize   = fread(fid,1,'*uint32');  %1024x(768-1)/4 = 2bits per pixel

    %  0=1 byte diff, 1=2byte diff, 2=4byte diff.  lut is stored as uchar8
    lookupTable   = fread(fid,lutSize,'*uint8'); % 
    cpBufSiz  = fread(fid,1,'*uint32');  %compressed buffer size

    % Read the first row and the first pixel in the 2nd row
    % YES, even 16-bit images are stored in 32-bits here!
    pix(1:sizeX+1) = fread(fid, sizeX+1, 'uint32'); 
    
    remaining = cpBufSiz - 4*(sizeX+1);

    % Decompress the rest by reading 1,2 or 4 bytes from
 %   byteDiff = transpose(fread(fid, remaining, '*uint8')); 
    byteDiff = fread(fid, remaining, '*uint8'); 

    b_index = 1;
    lut_idx = 0;
    lut_off = 3; %prepare to start by reading the first lookupTable byte
    % (the first code is NOT discarded as it corresponds to 2nd pixel in 2nd row)

    % define all variables before using in the loop
    dif0=0; dif1=0; dif2=0; flag=0;
    b4=uint8(0);

    for k = sizeX+2 : sizeX*sizeY

        if lut_off==3
            lut_off=0;
            lut_idx = lut_idx + 1;
            b4 = lookupTable(lut_idx);
        else        
            lut_off = lut_off+1;
            b4 = bitshift(b4, -2); 
        end

        %              "double" reduced the time by factor of 10!!!
        flag = double(bitand(b4, 3));  %flag should be 0, 1, or 2
        
        switch flag
            % this decoding is OK now and is faster than "typecast"
            case 0
                % just convert 
                if byteDiff(b_index) < 128
                    dif0 = double(byteDiff(b_index));
                else
                    dif0 = double(byteDiff(b_index)) - 256;
                end
                b_index = b_index + 1;
            case 1
                % convert 2 bytes to uint16
                if byteDiff(b_index+1) < 128
                    dif0 = double(byteDiff(b_index))+256*double(byteDiff(b_index+1));
                else
                    dif0 = double(byteDiff(b_index))+256*double(byteDiff(b_index+1))- 2^16;
                end
            %    diff = double(typecast(byteDiff(b_index:b_index+1),'int16'));
                b_index = b_index + 2;
            case 2
                % convert 4 bytes to uint32
                dif1 = double(byteDiff(b_index))+256*double(byteDiff(b_index+1));
                dif2 = double(byteDiff(b_index+2))+256*double(byteDiff(b_index+3));
                if byteDiff(b_index+3) < 128
                    dif0 = dif1 + 65536*dif2;
                else
                    dif0 = dif1 + 65536*dif2 - 2^32;
                end
      %          diff = double(typecast(byteDiff(b_index:b_index+3),'int32'));
                b_index = b_index + 4;
            case 3
                fprintf('\nIllegal compression code 3 at element %d', k);
        end
        
    % r11 = pix(k-sizeX-1), r12 = pix(k-sizeX), r21 = pix(k-1), r22=pix(k)
      
        pix(k) = pix(k-1) + pix(k-sizeX) - pix(k-sizeX-1)+ dif0;
    end
    
    ucBufSizee  = fread(fid,1,'*uint32');   %this is the next element
    % should be here...
    CP_end  = lutSize + cpBufSiz + 44;

end % of the compressed read routine
   
% image pix stored columnwise so transpose
ximage = transpose(reshape(uint32(pix), sizeX  ,sizeY ));        
% ximage = transpose(ximage);
fclose(fid);
% the end
end

function hnd = ximinfo(varargin)


%Version 3 does not read alphabetically

if nargin==0
    [fname, pname]= uigetfile('*.xim');
    xfile=fullfile(pname,fname);
else
    xfile=varargin{1};
end

%xfile = './XIM_images/Proj_00000.xim'
fid=fopen(xfile,'r');

% section 1 - header
hnd.bFileType = 'VARIAN_VA_INTERNAL_HNC          '; %Added this row so that it can be read by HNC_reader
hnd.filename  = xfile;
hnd.fmtID     = fread(fid,8,'*char')'; %row vector
hnd.version   = fread(fid,1,'*uint32');
hnd.sizeX     = fread(fid,1,'uint32'); 
hnd.sizeY     = fread(fid,1,'uint32');
hnd.bits      = fread(fid,1,'*uint32');
hnd.bytes     = fread(fid,1,'*uint32');
hnd.compress  = fread(fid,1,'*uint32');  %0-uncompressed, 1-HND compression
hnd.Height    = hnd.sizeY     ;          % as for Varian's XIMReader
hnd.Width     = hnd.sizeX    ;         %

% section 2 - data
% pix           = zeros(hnd.sizeX*hnd.sizeY, 1);

if hnd.compress==0
    hnd.ucBufSizee  = fread(fid,1,'*uint32');  
%   pix = fread(fid, hnd.sizeX*hnd.sizeY, dataType{hnd.bytes}); 
    % should be here...
    CP_end  = hnd.ucBufSizee + 36;
else

    %hnd compression (Thanos)
    hnd.lutSize   = fread(fid,1,'*uint32');  %1024x(768-1)/4 = 2bits per pixel

    %  0=1 byte diff, 1=2byte diff, 2=4byte diff.  lut is stored as uchar8
    % lookupTable   = fread(fid,hnd.lutSize,'*uint8'); % 
    
    fseek(fid, hnd.lutSize, 'cof');  % skip ahead this much
    hnd.cpBufSiz  = fread(fid,1,'*uint32');  %compressed buffer size
    fseek(fid, hnd.cpBufSiz, 'cof'); % skip ahead this much
   
    hnd.ucBufSizee  = fread(fid,1,'*uint32');   %this is the next element
    % should be here...
    CP_end  = hnd.lutSize + hnd.cpBufSiz + 44;

end % of the compressed read routine
   
% image pix stored columnwise so transpose
% hnd.image = transpose(reshape(uint32(pix), hnd.sizeX  ,hnd.sizeY ));        

if ftell(fid) <= CP_end
    fseek(fid,CP_end, 'bof');
else
    fprintf('\nFile read overrun at location %d', ftell(fid));
end    

% section 3 - histogram data
hnd.hist_size=fread(fid,1,'*uint32');
if hnd.hist_size>0
    hnd.histdata=fread(fid,hnd.hist_size,'*uint32');
end

% section 4 - property data - not entirely debugged?
hnd.nprops = fread(fid,1,'*uint32');
for k=1:hnd.nprops
%     k
    propLen  = fread(fid,1,'*uint32'); 
    propName = transpose(fread(fid,propLen,'*char'));
    %propName = fread(fid,propLen,'*char')
    propType = fread(fid,1,'*uint32'); %0-int, 1-dble, 2-str, 4-dbl arr, 5-int arr.
    switch propType
        case 0
          hnd.(propName) = fread(fid,1,'*int32'); 
        case 1
          hnd.(propName) = fread(fid,1,'*double'); 
        case 2
          propLen        = fread(fid,1,'*uint32'); 
          hnd.(propName) = transpose(fread(fid,propLen,'*char'));
        case 4
          propLen        = fread(fid,1,'*uint32');
          propLen = propLen/8;
          hnd.(propName) = transpose(fread(fid,propLen,'*double')); %untested
        case 5
          %propLen        = fread(fid,1,'*uint32') ;
          hnd.(propName) = transpose(fread(fid,5,'*uint32'));  %untested
    end
end



%hnd=orderfields(hnd); %alphabetically-ordered fields
fclose(fid);
% the end


end

function HncWrite_v4_vc(info,M,output)

%% HncWrite(info,M,outputfp)
% ------------------------------------------
% FILE   : HncWrite.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2012-11-26  Created.
% ------------------------------------------
% PURPOSE
%   Write the input header and image to a Varian HNC file.
% ------------------------------------------
% INPUT
%   info:       The header MATLAB struct.
%   M:          The image body, 2D (uint16).
%   outputfp:   The path of the output file.
% ------------------------------------------

%% Writing header

HNCHEADLENGTH = 512;

pixeltype = class(M);
%DEFINE THE FILES LENGTH 
switch pixeltype
    case 'uint8'
        info.('uiFileLength') = HNCHEADLENGTH + length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + length(M(:));
    case 'uint16'
        info.('uiFileLength') = HNCHEADLENGTH + 2*length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + 2*length(M(:));
    case 'uint32'
        info.('uiFileLength') = HNCHEADLENGTH + 4*length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + 2*length(M(:));
end

fid = fopen(output,'w');


% WRITING THE HEADERS WITH THE POSITIONS

%% ASSIGNING RANDOM VALUES WHEN VALUE IS NOT GIVEN IN XIM FILES
% IN CASE OF READBYTES = CHAR = uint8 : ASSIGNING LETTERS ABCDEG.. with the
% proper number of bits

% IF 'DOUBLE'= READDOUBLE. ASSIGNING VALUES 1.101 
DouBle =1.1111;

% IF LONG = UINT32 
LonG =uint32(111111);

%Position 0 to 32
fwrite(fid,info.bFileType,'uint8'); %'VARIAN_HNC_...'
%Position 32
fwrite(fid,info.uiFileLength,'uint32');
%Position 36
info.bCheckSumSpec = 'ABCD'; % 4 CHAR read by READBYTES(4) in KIM
fwrite(fid,info.bCheckSumSpec,'uint8');
%Position 40
info.uiCheckSum = LonG; % SME AS BEFORE
fwrite(fid,info.uiCheckSum,'uint32');
%Position 44
info.bCreationDate = 'ABCDEFGH'; %READBYTES(8)
fwrite(fid,info.bCreationDate,'uint8');
%Position 52
info.bCreationTime = 'ABCDEFGH';
fwrite(fid,info.bCreationTime,'uint8');
%Position 60
info.bPatientID = 'ABCDEFGHIJKLMNOP';
fwrite(fid,info.bPatientID,'uint8');
%Position 76 
info.uiPatientSer = LonG;
fwrite(fid,info.uiPatientSer,'uint32');
%Position 80
info.bSeriesID = 'ABCDEFGHIJKLMNOP';
fwrite(fid,info.bSeriesID,'uint8');
%Position 96
info.uiSeriesSer = LonG;
fwrite(fid,info.uiSeriesSer,'uint32');
%Position 100
info.bSliceID = 'ABCDEFGHIJKLMNOP';
fwrite(fid,info.bSliceID,'uint8');
%Position 116
info.uiSliceSer = LonG;
fwrite(fid,info.uiSliceSer,'uint32');
%Position 120
info.uiSizeX = info.sizeX; %Position = 121 122 123 124
fwrite(fid,info.uiSizeX,'uint32'); % Position 125 126 127 128
%Position 124
info.uiSizeY = info.sizeY ; %THE WIDTH AND LENGTH NEED TO BE SWITCHED FOR WRITING
fwrite(fid,info.uiSizeY,'uint32');
%Position 128
info.dSliceZPos = DouBle;
fwrite(fid,info.dSliceZPos,'double');
%Position 136
info.bModality = 'ABCDEFGHIJKLMNOP';
fwrite(fid,info.bModality,'uint8');
%Position 152
info.uiWindow =LonG;
fwrite(fid,info.uiWindow,'uint32');
%Position 156
info.uiLevel=LonG;
fwrite(fid,info.uiLevel,'uint32');
%Position 160
info.uiPixelOffset =LonG;
fwrite(fid,info.uiPixelOffset,'uint32');
%Position 164
info.bImageType = 'CHAR';
fwrite(fid,info.bImageType,'uint8');
%Position 168
info.dGantryRtn = info.GantryRtn;
fwrite(fid,info.dGantryRtn,'double');
%Position 186
info.dSAD = DouBle;
fwrite(fid,info.dSAD,'double');
%Position 194
info.dSFD = DouBle;
fwrite(fid,info.dSFD,'double');
%Position 202
info.dCollX1= info.KVCollimatorX1;
fwrite(fid,info.dCollX1,'double');
%Position 210
info.dCollX2= info.KVCollimatorX2;
fwrite(fid,info.dCollX2,'double');
%Position 218
info.dCollY1 =info.KVCollimatorY1;
fwrite(fid,info.dCollY1,'double');
%Position 226
info.dCollY2 = info.KVCollimatorY2;
fwrite(fid,info.dCollY2,'double');
%Position 234
info.dCollRtn = DouBle;
fwrite(fid,info.dCollRtn,'double');
%Position 242
info.dFieldX = DouBle;
fwrite(fid,info.dFieldX,'double');
%Position 250
info.dFieldY = DouBle;
fwrite(fid,info.dFieldY,'double');
%Position 258
info.dBladeX1 = DouBle;
fwrite(fid,info.dBladeX1,'double');
%Position 266
info.dBladeX2 = DouBle;
fwrite(fid,info.dBladeX2,'double');
%Position 272
info.dBladeY1 = DouBle;
fwrite(fid,info.dBladeY1,'double');
%Position 280
info.dBladeY2 = DouBle;
fwrite(fid,info.dBladeY2,'double');
%Position 288
info.dIDUPosLng = DouBle;
fwrite(fid,info.dIDUPosLng,'double');
%Position 296
info.dIDUPosLat = DouBle;
fwrite(fid,info.dIDUPosLat,'double');
%Position 302
info.dIDUPosVrt = DouBle;
fwrite(fid,info.dIDUPosVrt,'double');
%Position 310
info.dIDUPosRtn = DouBle;
fwrite(fid,info.dIDUPosRtn,'double');
%Position 318
info.dPatientSupportAngle = DouBle;
fwrite(fid,info.dPatientSupportAngle,'double');
%Position 326
info.dTableTopEccentricAngle = DouBle;
fwrite(fid,info.dTableTopEccentricAngle,'double');
%Position 332
info.dCouchVrt = info.CouchVrt;
fwrite(fid,info.dCouchVrt,'double');
%Position 340
info.dCouchLng =info.CouchLng;
fwrite(fid,info.dCouchLng,'double');
%Position 348
info.dCouchLat= info.CouchLat;
fwrite(fid,info.dCouchLat,'double');
%Position 356
info.dIDUResolutionX = DouBle;
fwrite(fid,info.dIDUResolutionX,'double');
%Position 362
info.dIDUResolutionY = DouBle;
fwrite(fid,info.dIDUResolutionY,'double');
%Position 370
info.dImageResolutionX = info.PixelWidth;
fwrite(fid,info.dImageResolutionX,'double');
%Position 378
info.dImageResolutionY = info.PixelHeight;
fwrite(fid,info.dImageResolutionY,'double');
%Position 386
info.dEnergy = DouBle;
fwrite(fid,info.dEnergy,'double');
%Position 392
info.dDoseRate= DouBle;
fwrite(fid,info.dDoseRate,'double');
%Position 400
info.dXRayKV=info.KVKiloVolts;
fwrite(fid,info.dXRayKV,'double');
%Position 408
info.dXRayMA=  info.KVMilliAmperes;
fwrite(fid,info.dXRayMA,'double');
%Position 416
info.dMetersetExposure= DouBle;
fwrite(fid,info.dMetersetExposure,'double');
%Position 424
info.dAcqAdjustment= DouBle;
fwrite(fid,info.dAcqAdjustment,'double');
%Position 432
info.dCTProjectionAngle = info.GantryRtn-90;
fwrite(fid,info.dCTProjectionAngle,'double');
%Position 440
info.CBCTPositiveAngle = info.dCTProjectionAngle + 270.0;
fwrite(fid,info.CBCTPositiveAngle,'double');
%Position 448
info.dCTNormChamber=DouBle;
fwrite(fid,info.dCTNormChamber,'double');
%Position 456
info.dGatingTimeTag=DouBle;
fwrite(fid,info.dGatingTimeTag,'double');
%Position 464
info.dGating4DInfoX=DouBle;
fwrite(fid,info.dGating4DInfoX,'double');
%Position 472
info.dGating4DInfoY=DouBle;
fwrite(fid,info.dGating4DInfoY,'double');
%Position 480
info.dGating4DInfoZ=DouBle;
fwrite(fid,info.dGating4DInfoZ,'double');
%Position 488
info.dGating4DInfoTime=DouBle;
fwrite(fid,info.dGating4DInfoTime,'double');
%Position 496
info.dOffsetX=DouBle;
fwrite(fid,info.dOffsetX,'double');
%Position 504
info.dOffsetY=DouBle;
fwrite(fid,info.dOffsetY,'double');
% END AT 504+8 = 512!

%M(1:3,1:3) = uint32(5); Add this line
%to check that it does start writing at the location 512

%% Write the image body
% Truncate to prevent overflow

        M(M>2^32-1) = 2^32 -1;
        fwrite(fid,M(:),'uint32');

fclose(fid);

return

end

function ReadHnc(input)

h = waitbar(0,'BMP HNC files...');

headerlength=512;
dirListing = dir(strcat(input,'*.hnc')); 

list =dir(fullfile(input,'*.hnc'));
name={list.name};
str=sprintf('%s#',name{:});
num=sscanf(str,'OutputHNC_%d.hnc#');
[~,index]=sort(num);
name=name(index);
mkdir(strcat(input,'.\BMP/'));


%Taking their names out. Treating one by one the files
    for k = 1 :length(dirListing)
        filename=strcat(input,char(name(k)));
        fid = fopen(filename);
        waitbar(k / length(dirListing))

         if fid>0
         hnc_image = fopen(filename,'r');


 
    fseek(hnc_image,0 , 'bof'); %22-0 = 22
    
    header.FileType = char(fread(hnc_image,32,'*char')');
    header.FileLength = fread(hnc_image,1,'*long');
    header.CheckSumSpec = strtok(char(fread(hnc_image,4,'*char')'));
    header.CheckSum = fread(hnc_image,1,'*long');
    header.ImgDate = char(fread(hnc_image,8,'char')');
    header.ImgTime = char(fread(hnc_image,8,'char')');  
    header.PatientId = strtok(char(fread(hnc_image,16,'*char')'));
    header.PatientSerNr = fread(hnc_image,1,'*long');
    header.SeriesId = strtok(char(fread(hnc_image,16,'*char')'));
    header.SeriesSerNr = fread(hnc_image,1,'*long');
    header.SliceId = strtok(char(fread(hnc_image,16,'*char')'));
    header.SliceSerNr = fread(hnc_image,1,'*long');
    header.ImgWidth = double(fread(hnc_image,1,'*long'));
    header.ImgHeight = double(fread(hnc_image,1,'*long'));
    header.SliceZPos = fread(hnc_image,1,'*double');
    header.Modality = strtok(char(fread(hnc_image,16,'*char')'));
    header.Window = fread(hnc_image,1,'*long');
    header.Level = fread(hnc_image,1,'*long');
    header.PixelOffset = fread(hnc_image,1,'*long');
    header.ImageType = strtok(char(fread(hnc_image,4,'*char')'));
    header.GantryRtn = fread(hnc_image,1,'*double');
    header.SAD = fread(hnc_image,1,'*double');
    header.SFD = fread(hnc_image,1,'*double');
    header.CollX1 = fread(hnc_image,1,'*double');
    header.CollX2 = fread(hnc_image,1,'*double');
    header.CollY1 = fread(hnc_image,1,'*double');
    header.CollY2 = fread(hnc_image,1,'*double');
    header.CollRtn = fread(hnc_image,1,'*double');
    header.FieldX = fread(hnc_image,1,'*double');
    header.FieldY = fread(hnc_image,1,'*double');
    header.BladeX1 = fread(hnc_image,1,'*double');
    header.BladeX2 = fread(hnc_image,1,'*double');
    header.BladeY1 = fread(hnc_image,1,'*double');
    header.BladeY2 = fread(hnc_image,1,'*double');
    header.IDUPosLng = fread(hnc_image,1,'*double');
    header.IDUPosLat = fread(hnc_image,1,'*double');
    header.IDUPosVrt = fread(hnc_image,1,'*double');
    header.IDUPosRtn = fread(hnc_image,1,'*double');
    header.PatientSupportAngle = fread(hnc_image,1,'*double');
    header.TableTopEccentricAngle = fread(hnc_image,1,'*double');
    header.CouchVrt = fread(hnc_image,1,'*double');
    header.CouchLng = fread(hnc_image,1,'*double');
    header.CouchLat = fread(hnc_image,1,'*double');
    header.IDUResolutionX = fread(hnc_image,1,'*double');
    header.IDUResolutionY = fread(hnc_image,1,'*double');
    header.ImageResolutionX = fread(hnc_image,1,'*double');
    header.ImageResolutionY = fread(hnc_image,1,'*double');
    header.Energy = fread(hnc_image,1,'*double');
    header.DoseRate = fread(hnc_image,1,'*double');
    header.XRayKV = fread(hnc_image,1,'*double');
    header.XRayMA = fread(hnc_image,1,'*double');
    header.MetersetExposure = fread(hnc_image,1,'*double');
    header.AcqAdjustment = fread(hnc_image,1,'*double');
    header.CTProjectionAngle = fread(hnc_image,1,'*double');
    header.CBCTPositiveAngle=fread(hnc_image,1,'*double');
    header.CTNormChamber = fread(hnc_image,1,'*double');
    header.GatingTimeTag = fread(hnc_image,1,'*double');
    header.Gating4DInfoX = fread(hnc_image,1,'*double');
    header.Gating4DInfoY = fread(hnc_image,1,'*double');
    header.Gating4DInfoZ = fread(hnc_image,1,'*double');
    header.Gating4DInfoTime = fread(hnc_image,1,'*double');
    header.dOffsetX = fread(hnc_image,1,'*double');
    header.dOffsetY = fread(hnc_image,1,'*double');

    fseek(hnc_image,headerlength,-1);
    
    M = uint16((fread(hnc_image,[1024,768],'long'))');

    AngleGantry= header.GantryRtn;
    
    imagesc(M)
    title('HNC Image')
    colormap(gray)
    f=getframe;
    output=strcat(input,'.\BMP/',num2str(k-1),'_',num2str(AngleGantry),'.bmp');
    imwrite(f.cdata,output,'bmp');
    




    end
 fclose('all');
        

    end
close(h)
end







