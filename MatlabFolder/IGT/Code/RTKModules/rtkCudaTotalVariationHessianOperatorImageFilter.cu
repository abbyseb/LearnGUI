/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

// rtk includes
#include "rtkCudaTotalVariationHessianOperatorImageFilter.hcu"
#include "rtkCudaUtilities.hcu"

#include <itkMacro.h>

// cuda includes
#include <cuda.h>

// TEXTURES AND CONSTANTS //

__constant__ int3 c_Size;

//inline int iDivUp(int a, int b){
//    return (a % b != 0) ? (a / b + 1) : (a / b);
//}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_( S T A R T )_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

__global__
void
tvhessian3_kernel(float *in, float *v, float* out, float a2, float b2, float c2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  long int id_x1 = (k * c_Size.y + j) * c_Size.x + i1;
  long int id_y1 = (k * c_Size.y + j1) * c_Size.x + i;
  long int id_z1 = (k1 * c_Size.y + j) * c_Size.x + i;
  long int id_x2 = (k * c_Size.y + j) * c_Size.x + i2;
  long int id_x2y1 = (k * c_Size.y + j1) * c_Size.x + i2;
  long int id_x2z1 = (k1 * c_Size.y + j) * c_Size.x + i2;
  long int id_y2 = (k * c_Size.y + j2) * c_Size.x + i;
  long int id_y2x1 = (k * c_Size.y + j2) * c_Size.x + i1;
  long int id_y2z1 = (k1 * c_Size.y + j2) * c_Size.x + i;
  long int id_z2 = (k2 * c_Size.y + j) * c_Size.x + i;
  long int id_z2x1 = (k2 * c_Size.y + j) * c_Size.x + i1;
  long int id_z2y1 = (k2 * c_Size.y + j1) * c_Size.x + i;
  
  float Dx = in[id] - in[id_x1];
  float Dy = in[id] - in[id_y1];
  float Dz = in[id] - in[id_z1];
  float Dxx = in[id_x2] - in[id];
  float Dxy = in[id_x2] - in[id_x2y1];
  float Dxz = in[id_x2] - in[id_x2z1];
  float Dyx = in[id_y2] - in[id_y2x1];
  float Dyy = in[id_y2] - in[id];
  float Dyz = in[id_y2] - in[id_y2z1];
  float Dzx = in[id_z2] - in[id_z2x1];
  float Dzy = in[id_z2] - in[id_z2y1];
  float Dzz = in[id_z2] - in[id];
  
  float sv = sqrtf( a2*Dx*Dx + b2*Dy*Dy + c2*Dz*Dz + 1.19e-07 );
  sv *= sv*sv;
  float svx = sqrtf( a2*Dxx*Dxx + b2*Dxy*Dxy + c2*Dxz*Dxz + 1.19e-07 );
  svx *= svx*svx;
  float svy = sqrtf( a2*Dyx*Dyx + b2*Dyy*Dyy + c2*Dyz*Dyz + 1.19e-07 );
  svy *= svy*svy;
  float svz = sqrtf( a2*Dzx*Dzx + b2*Dzy*Dzy + c2*Dzz*Dzz + 1.19e-07 );
  svz *= svz*svz;
  
  out[id] = v[id] * ( (a2*b2*(Dx-Dy)*(Dx-Dy) + a2*c2*(Dx-Dz)*(Dx-Dz) + b2*c2*(Dy-Dz)*(Dy-Dz))/sv + a2*(b2*Dxy*Dxy + c2*Dxz*Dxz)/svx + b2*(a2*Dyx*Dyx + c2*Dyz*Dyz)/svy + c2*(a2*Dzx*Dzx + b2*Dzy*Dzy)/svz )
	   + v[id_x1] * a2 * ( b2*Dy*(Dx-Dy) + c2*Dz*(Dx-Dz) ) / sv
	   + v[id_y1] * b2 * ( a2*Dx*(Dy-Dx) + c2*Dz*(Dy-Dz) ) / sv
	   + v[id_z1] * c2 * ( a2*Dx*(Dz-Dx) + b2*Dy*(Dz-Dy) ) / sv
	   + v[id_x2] * a2 * ( b2*Dxy*(Dxx-Dxy) + c2*Dxz*(Dxx-Dxz) ) / svx
	   + v[id_y2] * b2 * ( a2*Dyx*(Dyy-Dyx) + c2*Dyz*(Dyy-Dyz) ) / svy
	   + v[id_z2] * c2 * ( a2*Dzx*(Dzz-Dzx) + b2*Dzy*(Dzz-Dzy) ) / svz
	 - v[id_x2y1] * a2 * b2 * Dxx * Dxy / svx
	 - v[id_x2z1] * a2 * c2 * Dxx * Dxz / svx
	 - v[id_y2x1] * a2 * b2 * Dyx * Dyy / svy
	 - v[id_y2z1] * b2 * c2 * Dyy * Dyz / svy
	 - v[id_z2x1] * a2 * c2 * Dzx * Dzz / svz
	 - v[id_z2y1] * b2 * c2 * Dzy * Dzz / svz;
}

__global__
void
tvhessian3_scaled_kernel(float *in, float *v, float* out, float scalingFactor, float a2, float b2, float c2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  long int id_x1 = (k * c_Size.y + j) * c_Size.x + i1;
  long int id_y1 = (k * c_Size.y + j1) * c_Size.x + i;
  long int id_z1 = (k1 * c_Size.y + j) * c_Size.x + i;
  long int id_x2 = (k * c_Size.y + j) * c_Size.x + i2;
  long int id_x2y1 = (k * c_Size.y + j1) * c_Size.x + i2;
  long int id_x2z1 = (k1 * c_Size.y + j) * c_Size.x + i2;
  long int id_y2 = (k * c_Size.y + j2) * c_Size.x + i;
  long int id_y2x1 = (k * c_Size.y + j2) * c_Size.x + i1;
  long int id_y2z1 = (k1 * c_Size.y + j2) * c_Size.x + i;
  long int id_z2 = (k2 * c_Size.y + j) * c_Size.x + i;
  long int id_z2x1 = (k2 * c_Size.y + j) * c_Size.x + i1;
  long int id_z2y1 = (k2 * c_Size.y + j1) * c_Size.x + i;
  
  float Dx = scalingFactor * ( in[id] - in[id_x1] );
  float Dy = scalingFactor * ( in[id] - in[id_y1] );
  float Dz = scalingFactor * ( in[id] - in[id_z1] );
  float Dxx = scalingFactor * ( in[id_x2] - in[id] );
  float Dxy = scalingFactor * ( in[id_x2] - in[id_x2y1] );
  float Dxz = scalingFactor * ( in[id_x2] - in[id_x2z1] );
  float Dyx = scalingFactor * ( in[id_y2] - in[id_y2x1] );
  float Dyy = scalingFactor * ( in[id_y2] - in[id] );
  float Dyz = scalingFactor * ( in[id_y2] - in[id_y2z1] );
  float Dzx = scalingFactor * ( in[id_z2] - in[id_z2x1] );
  float Dzy = scalingFactor * ( in[id_z2] - in[id_z2y1] );
  float Dzz = scalingFactor * ( in[id_z2] - in[id] );
  
  float sv = sqrtf( a2*Dx*Dx + b2*Dy*Dy + c2*Dz*Dz + 1.19e-07 );
  sv *= sv*sv;
  float svx = sqrtf( a2*Dxx*Dxx + b2*Dxy*Dxy + c2*Dxz*Dxz + 1.19e-07 );
  svx *= svx*svx;
  float svy = sqrtf( a2*Dyx*Dyx + b2*Dyy*Dyy + c2*Dyz*Dyz + 1.19e-07 );
  svy *= svy*svy;
  float svz = sqrtf( a2*Dzx*Dzx + b2*Dzy*Dzy + c2*Dzz*Dzz + 1.19e-07 );
  svz *= svz*svz;
  
  out[id] = v[id] * ( (a2*b2*(Dx-Dy)*(Dx-Dy) + a2*c2*(Dx-Dz)*(Dx-Dz) + b2*c2*(Dy-Dz)*(Dy-Dz))/sv + a2*(b2*Dxy*Dxy + c2*Dxz*Dxz)/svx + b2*(a2*Dyx*Dyx + c2*Dyz*Dyz)/svy + c2*(a2*Dzx*Dzx + b2*Dzy*Dzy)/svz )
	   + v[id_x1] * a2 * ( b2*Dy*(Dx-Dy) + c2*Dz*(Dx-Dz) ) / sv
	   + v[id_y1] * b2 * ( a2*Dx*(Dy-Dx) + c2*Dz*(Dy-Dz) ) / sv
	   + v[id_z1] * c2 * ( a2*Dx*(Dz-Dx) + b2*Dy*(Dz-Dy) ) / sv
	   + v[id_x2] * a2 * ( b2*Dxy*(Dxx-Dxy) + c2*Dxz*(Dxx-Dxz) ) / svx
	   + v[id_y2] * b2 * ( a2*Dyx*(Dyy-Dyx) + c2*Dyz*(Dyy-Dyz) ) / svy
	   + v[id_z2] * c2 * ( a2*Dzx*(Dzz-Dzx) + b2*Dzy*(Dzz-Dzy) ) / svz
	 - v[id_x2y1] * a2 * b2 * Dxx * Dxy / svx
	 - v[id_x2z1] * a2 * c2 * Dxx * Dxz / svx
	 - v[id_y2x1] * a2 * b2 * Dyx * Dyy / svy
	 - v[id_y2z1] * b2 * c2 * Dyy * Dyz / svy
	 - v[id_z2x1] * a2 * c2 * Dzx * Dzz / svz
	 - v[id_z2y1] * b2 * c2 * Dzy * Dzz / svz;
  out[id] *= scalingFactor;
}

__global__
void
tvhessian3_adaptive_kernel(float *in, float *v, float* out, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  long int id_x1 = (k * c_Size.y + j) * c_Size.x + i1;
  long int id_y1 = (k * c_Size.y + j1) * c_Size.x + i;
  long int id_z1 = (k1 * c_Size.y + j) * c_Size.x + i;
  long int id_x2 = (k * c_Size.y + j) * c_Size.x + i2;
  long int id_x2y1 = (k * c_Size.y + j1) * c_Size.x + i2;
  long int id_x2z1 = (k1 * c_Size.y + j) * c_Size.x + i2;
  long int id_y2 = (k * c_Size.y + j2) * c_Size.x + i;
  long int id_y2x1 = (k * c_Size.y + j2) * c_Size.x + i1;
  long int id_y2z1 = (k1 * c_Size.y + j2) * c_Size.x + i;
  long int id_z2 = (k2 * c_Size.y + j) * c_Size.x + i;
  long int id_z2x1 = (k2 * c_Size.y + j) * c_Size.x + i1;
  long int id_z2y1 = (k2 * c_Size.y + j1) * c_Size.x + i;
  
  float Dx = in[id] - in[id_x1];
  float Dy = in[id] - in[id_y1];
  float Dz = in[id] - in[id_z1];
  float Dxx = in[id_x2] - in[id];
  float Dxy = in[id_x2] - in[id_x2y1];
  float Dxz = in[id_x2] - in[id_x2z1];
  float Dyx = in[id_y2] - in[id_y2x1];
  float Dyy = in[id_y2] - in[id];
  float Dyz = in[id_y2] - in[id_y2z1];
  float Dzx = in[id_z2] - in[id_z2x1];
  float Dzy = in[id_z2] - in[id_z2y1];
  float Dzz = in[id_z2] - in[id];
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;
  float wxx = wx[(k * c_Size.y + j) * c_Size.x + i2]; wxx *= wxx;
  float wyx = wy[(k * c_Size.y + j) * c_Size.x + i2]; wyx *= wyx;
  float wzx = wz[(k * c_Size.y + j) * c_Size.x + i2]; wzx *= wzx;
  float wxy = wx[(k * c_Size.y + j2) * c_Size.x + i]; wxy *= wxy;
  float wyy = wy[(k * c_Size.y + j2) * c_Size.x + i]; wyy *= wyy;
  float wzy = wz[(k * c_Size.y + j2) * c_Size.x + i]; wzy *= wzy;
  float wxz = wx[(k2 * c_Size.y + j) * c_Size.x + i]; wxz *= wxz;
  float wyz = wy[(k2 * c_Size.y + j) * c_Size.x + i]; wyz *= wyz;
  float wzz = wz[(k2 * c_Size.y + j) * c_Size.x + i]; wzz *= wzz;
  
  float sv = sqrtf( wx0*Dx*Dx + wy0*Dy*Dy + wz0*Dz*Dz + 1.19e-07 );
  sv *= sv*sv;
  float svx = sqrtf( wxx*Dxx*Dxx + wyx*Dxy*Dxy + wzx*Dxz*Dxz + 1.19e-07 );
  svx *= svx*svx;
  float svy = sqrtf( wxy*Dyx*Dyx + wyy*Dyy*Dyy + wzy*Dyz*Dyz + 1.19e-07 );
  svy *= svy*svy;
  float svz = sqrtf( wxz*Dzx*Dzx + wyz*Dzy*Dzy + wzz*Dzz*Dzz + 1.19e-07 );
  svz *= svz*svz;
  
  out[id] = v[id] * ( (wx0*wy0*(Dx-Dy)*(Dx-Dy) + wx0*wz0*(Dx-Dz)*(Dx-Dz) + wy0*wz0*(Dy-Dz)*(Dy-Dz))/sv + wxx*(wyx*Dxy*Dxy + wzx*Dxz*Dxz)/svx + wyy*(wxy*Dyx*Dyx + wzy*Dyz*Dyz)/svy + wzz*(wxz*Dzx*Dzx + wyz*Dzy*Dzy)/svz )
	   + v[id_x1] * wx0 * ( wy0*Dy*(Dx-Dy) + wz0*Dz*(Dx-Dz) ) / sv
	   + v[id_y1] * wy0 * ( wx0*Dx*(Dy-Dx) + wz0*Dz*(Dy-Dz) ) / sv
	   + v[id_z1] * wz0 * ( wx0*Dx*(Dz-Dx) + wy0*Dy*(Dz-Dy) ) / sv
	   + v[id_x2] * wxx * ( wyx*Dxy*(Dxx-Dxy) + wzx*Dxz*(Dxx-Dxz) ) / svx
	   + v[id_y2] * wyy * ( wxy*Dyx*(Dyy-Dyx) + wzy*Dyz*(Dyy-Dyz) ) / svy
	   + v[id_z2] * wzz * ( wxz*Dzx*(Dzz-Dzx) + wyz*Dzy*(Dzz-Dzy) ) / svz
	 - v[id_x2y1] * wxx * wyx * Dxx * Dxy / svx
	 - v[id_x2z1] * wxx * wzx * Dxx * Dxz / svx
	 - v[id_y2x1] * wxy * wyy * Dyx * Dyy / svy
	 - v[id_y2z1] * wyy * wzy * Dyy * Dyz / svy
	 - v[id_z2x1] * wxz * wzz * Dzx * Dzz / svz
	 - v[id_z2y1] * wyz * wzz * Dzy * Dzz / svz;
}

__global__
void
tvhessian3_adaptive_scaled_kernel(float *in, float *v, float* out, float scalingFactor, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  long int id_x1 = (k * c_Size.y + j) * c_Size.x + i1;
  long int id_y1 = (k * c_Size.y + j1) * c_Size.x + i;
  long int id_z1 = (k1 * c_Size.y + j) * c_Size.x + i;
  long int id_x2 = (k * c_Size.y + j) * c_Size.x + i2;
  long int id_x2y1 = (k * c_Size.y + j1) * c_Size.x + i2;
  long int id_x2z1 = (k1 * c_Size.y + j) * c_Size.x + i2;
  long int id_y2 = (k * c_Size.y + j2) * c_Size.x + i;
  long int id_y2x1 = (k * c_Size.y + j2) * c_Size.x + i1;
  long int id_y2z1 = (k1 * c_Size.y + j2) * c_Size.x + i;
  long int id_z2 = (k2 * c_Size.y + j) * c_Size.x + i;
  long int id_z2x1 = (k2 * c_Size.y + j) * c_Size.x + i1;
  long int id_z2y1 = (k2 * c_Size.y + j1) * c_Size.x + i;
  
  float Dx = scalingFactor * ( in[id] - in[id_x1] );
  float Dy = scalingFactor * ( in[id] - in[id_y1] );
  float Dz = scalingFactor * ( in[id] - in[id_z1] );
  float Dxx = scalingFactor * ( in[id_x2] - in[id] );
  float Dxy = scalingFactor * ( in[id_x2] - in[id_x2y1] );
  float Dxz = scalingFactor * ( in[id_x2] - in[id_x2z1] );
  float Dyx = scalingFactor * ( in[id_y2] - in[id_y2x1] );
  float Dyy = scalingFactor * ( in[id_y2] - in[id] );
  float Dyz = scalingFactor * ( in[id_y2] - in[id_y2z1] );
  float Dzx = scalingFactor * ( in[id_z2] - in[id_z2x1] );
  float Dzy = scalingFactor * ( in[id_z2] - in[id_z2y1] );
  float Dzz = scalingFactor * ( in[id_z2] - in[id] );
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;
  float wxx = wx[(k * c_Size.y + j) * c_Size.x + i2]; wxx *= wxx;
  float wyx = wy[(k * c_Size.y + j) * c_Size.x + i2]; wyx *= wyx;
  float wzx = wz[(k * c_Size.y + j) * c_Size.x + i2]; wzx *= wzx;
  float wxy = wx[(k * c_Size.y + j2) * c_Size.x + i]; wxy *= wxy;
  float wyy = wy[(k * c_Size.y + j2) * c_Size.x + i]; wyy *= wyy;
  float wzy = wz[(k * c_Size.y + j2) * c_Size.x + i]; wzy *= wzy;
  float wxz = wx[(k2 * c_Size.y + j) * c_Size.x + i]; wxz *= wxz;
  float wyz = wy[(k2 * c_Size.y + j) * c_Size.x + i]; wyz *= wyz;
  float wzz = wz[(k2 * c_Size.y + j) * c_Size.x + i]; wzz *= wzz;
  
  float sv = sqrtf( wx0*Dx*Dx + wy0*Dy*Dy + wz0*Dz*Dz + 1.19e-07 );
  sv *= sv*sv;
  float svx = sqrtf( wxx*Dxx*Dxx + wyx*Dxy*Dxy + wzx*Dxz*Dxz + 1.19e-07 );
  svx *= svx*svx;
  float svy = sqrtf( wxy*Dyx*Dyx + wyy*Dyy*Dyy + wzy*Dyz*Dyz + 1.19e-07 );
  svy *= svy*svy;
  float svz = sqrtf( wxz*Dzx*Dzx + wyz*Dzy*Dzy + wzz*Dzz*Dzz + 1.19e-07 );
  svz *= svz*svz;

  out[id] = v[id] * ( (wx0*wy0*(Dx-Dy)*(Dx-Dy) + wx0*wz0*(Dx-Dz)*(Dx-Dz) + wy0*wz0*(Dy-Dz)*(Dy-Dz))/sv + wxx*(wyx*Dxy*Dxy + wzx*Dxz*Dxz)/svx + wyy*(wxy*Dyx*Dyx + wzy*Dyz*Dyz)/svy + wzz*(wxz*Dzx*Dzx + wyz*Dzy*Dzy)/svz )
	   + v[id_x1] * wx0 * ( wy0*Dy*(Dx-Dy) + wz0*Dz*(Dx-Dz) ) / sv
	   + v[id_y1] * wy0 * ( wx0*Dx*(Dy-Dx) + wz0*Dz*(Dy-Dz) ) / sv
	   + v[id_z1] * wz0 * ( wx0*Dx*(Dz-Dx) + wy0*Dy*(Dz-Dy) ) / sv
	   + v[id_x2] * wxx * ( wyx*Dxy*(Dxx-Dxy) + wzx*Dxz*(Dxx-Dxz) ) / svx
	   + v[id_y2] * wyy * ( wxy*Dyx*(Dyy-Dyx) + wzy*Dyz*(Dyy-Dyz) ) / svy
	   + v[id_z2] * wzz * ( wxz*Dzx*(Dzz-Dzx) + wyz*Dzy*(Dzz-Dzy) ) / svz
	 - v[id_x2y1] * wxx * wyx * Dxx * Dxy / svx
	 - v[id_x2z1] * wxx * wzx * Dxx * Dxz / svx
	 - v[id_y2x1] * wxy * wyy * Dyx * Dyy / svy
	 - v[id_y2z1] * wyy * wzy * Dyy * Dyz / svy
	 - v[id_z2x1] * wxz * wzz * Dzx * Dzz / svz
	 - v[id_z2y1] * wyz * wzz * Dzy * Dzz / svz;
  out[id] *= scalingFactor;
}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-( E N D )-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

void
CUDA_total_variation_hessian_operator(int size[3],
						float* dev_in0,
						float* dev_in1,
						float* dev_out,
						float scalingFactor,
						float spacingX,
						float spacingY,
						float spacingZ)
{
  int3 dev_Size = make_int3(size[0], size[1], size[2]);
  cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));

  // Reset output volume
  long int memorySizeOutput = size[0] * size[1] * size[2] * sizeof(float);
  cudaMemset((void *)dev_out, 0, memorySizeOutput );

  // Thread Block Dimensions
  dim3 dimBlock = dim3(16, 4, 4);

  int blocksInX = iDivUp(size[0], dimBlock.x);
  int blocksInY = iDivUp(size[1], dimBlock.y);
  int blocksInZ = iDivUp(size[2], dimBlock.z);

  dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
  
  float spacingX2 = spacingX * spacingX;
  float spacingY2 = spacingY * spacingY;
  float spacingZ2 = spacingZ * spacingZ;
  
  if (scalingFactor==1)
	tvhessian3_kernel <<< dimGrid, dimBlock >>> ( dev_in0, dev_in1, dev_out, spacingX2, spacingY2, spacingZ2);
  else
	tvhessian3_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in0, dev_in1, dev_out, scalingFactor, spacingX2, spacingY2, spacingZ2);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}

void
CUDA_total_variation_hessian_operator_adaptive(int size[3],
						float* dev_in0,
						float* dev_in1,
						float* dev_out,
						float scalingFactor,
						float* dev_wx,
						float* dev_wy,
						float* dev_wz)
{
  int3 dev_Size = make_int3(size[0], size[1], size[2]);
  cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));

  // Reset output volume
  long int memorySizeOutput = size[0] * size[1] * size[2] * sizeof(float);
  cudaMemset((void *)dev_out, 0, memorySizeOutput );

  // Thread Block Dimensions
  dim3 dimBlock = dim3(16, 4, 4);

  int blocksInX = iDivUp(size[0], dimBlock.x);
  int blocksInY = iDivUp(size[1], dimBlock.y);
  int blocksInZ = iDivUp(size[2], dimBlock.z);

  dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
  
  if (scalingFactor==1)
	tvhessian3_adaptive_kernel <<< dimGrid, dimBlock >>> ( dev_in0, dev_in1, dev_out, dev_wx, dev_wy, dev_wz);
  else
	tvhessian3_adaptive_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in0, dev_in1, dev_out, scalingFactor, dev_wx, dev_wy, dev_wz);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}
