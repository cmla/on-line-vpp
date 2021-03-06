// This program is free software: you can use, modify and/or redistribute it
// under the terms of the simplified BSD License. You should have received a
// copy of this license along this program. If not, see
// <http://www.opensource.org/licenses/bsd-license.html>.
//
// Copyright (C) 2015, Javier Sánchez Pérez <jsanchez@ulpgc.es>
// Copyright (C) 2014, Nelson Monzón López  <nmonzon@ctim.es>
// All rights reserved.

#include "mask.h"

#include <math.h>
#include <stdio.h>

/**
  *
  * Function to compute the gradient with centered differences
  *
**/
void gradient(
    float *input,  //input image
    float *dx,           //computed x derivative
    float *dy,           //computed y derivative
    const int nx,        //image width
    const int ny         //image height
)
{
  //apply the gradient to the center body of the image
  #pragma omp parallel for
  for(int i = 1; i < ny-1; i++)
  {
     for(int j = 1; j < nx-1; j++)
     {
         const int k = i * nx + j;
         dx[k] = 0.5*(input[k+1] - input[k-1]);
         dy[k] = 0.5*(input[k+nx] - input[k-nx]);
     }
  }

  //apply the gradient to the first and last rows
  #pragma omp parallel for
  for(int j = 1; j < nx-1; j++)
  {
     dx[j] = 0.5*(input[j+1] - input[j-1]);
     dy[j] = 0.5*(input[j+nx] - input[j]);

     const int k = (ny - 1) * nx + j;

     dx[k] = 0.5*(input[k+1] - input[k-1]);
     dy[k] = 0.5*(input[k] - input[k-nx]);
  }

  //apply the gradient to the first and last columns
  #pragma omp for schedule(dynamic) nowait
  for(int i = 1; i < ny-1; i++)
  {
     const int p = i * nx;
     dx[p] = 0.5*(input[p+1] - input[p]);
     dy[p] = 0.5*(input[p+nx] - input[p-nx]);

     const int k = (i+1) * nx - 1;

     dx[k] = 0.5*(input[k] - input[k-1]);
     dy[k] = 0.5*(input[k+nx] - input[k-nx]);
  }

  //apply the gradient to the four corners
  dx[0] = 0.5*(input[1] - input[0]);
  dy[0] = 0.5*(input[nx] - input[0]);

  dx[nx-1] = 0.5*(input[nx-1] - input[nx-2]);
  dy[nx-1] = 0.5*(input[2*nx-1] - input[nx-1]);

  dx[(ny-1)*nx] = 0.5*(input[(ny-1)*nx + 1] - input[(ny-1)*nx]);
  dy[(ny-1)*nx] = 0.5*(input[(ny-1)*nx] - input[(ny-2)*nx]);

  dx[ny*nx-1] = 0.5*(input[ny*nx-1] - input[ny*nx-1-1]);
  dy[ny*nx-1] = 0.5*(input[ny*nx-1] - input[(ny-1)*nx-1]);
}


/**
  *
  * Function to compute the gradient at given points
  * It does not treat border pixels
  *
**/
void gradient(
    float *input, //input image
    float *dx,    //computed x derivative
    float *dy,    //computed y derivative
    std::vector<int> &x,  //positions to compute the gradient
    const int nx  //image width
)
{
  //apply the gradient to the center body of the image
  #pragma omp parallel for
  for(unsigned int i = 0; i < x.size(); i++)
  {
     const int k = x[i];
     dx[i] = 0.5*(input[k+1] - input[k-1]);
     dy[i] = 0.5*(input[k+nx] - input[k-nx]);
  }
}

/**
 *
 * Convolution with a Gaussian
 *
 */
void gaussian (
  float *I,     //input image
  float *Is,    //output image
  int xdim,     //image width
  int ydim,     //image height
  float sigma,  //Gaussian sigma
  int precision //defines the size of the window
)
{
  int i, j, k;

  if(sigma<=0 || precision<=0){
    #pragma omp parallel for
    for(int i=0; i<xdim*ydim; i++) Is[i] = I[i];
    return;
  }
  
  double den  = 2*sigma*sigma;
  int    size = (int) (precision*sigma)+1;
  int    bdx  = xdim+size;
  int    bdy  = ydim+size;

  if(size>xdim) return;
        
  //compute the coefficients of the 1D convolution kernel
  double *B = new double[size];
  for (int i=0; i<size; i++)
    B[i] = 1/(sigma*sqrt(2.0*3.1415926))*exp(-i*i/den);

  double norm=0;

  //normalize the 1D convolution kernel
  for (int i=0; i<size; i++)
    norm += B[i];

  norm *= 2;

  norm -= B[0];

  for (int i=0; i<size; i++)
    B[i] /= norm;

#pragma omp parallel for
  //convolution of each line of the input image
  for (k=0; k<ydim; k++)
  {
    double *R = new double[size+xdim+size];
    for (i=size; i<bdx; i++)
      R[i] = I[k*xdim+i-size];

    //reflecting boundary conditions
    for (i=0, j=bdx; i<size; i++, j++)
    {
      R[i] = I[k*xdim+size-i];
      R[j] = I[k*xdim+xdim-i-1];
    }

    for (i=size; i<bdx; i++)
    {
      double sum = B[0]*R[i];

      for (int j = 1; j < size; j++)
        sum += B[j]*(R[i-j]+R[i+j]);

      Is[k*xdim+i-size] = sum;
    }
    delete []R;
  }

#pragma omp parallel for
  //convolution of each column of the input image
  for (k=0; k<xdim; k++)
  {
    double *T = new double[size+ydim+size];
    for (i=size; i<bdy; i++)
      T[i] = Is[(i-size)*xdim+k];
           
    //reflecting boundary conditions
    for (i=0, j=bdy; i<size; i++, j++)
    {
      T[i] = Is[(size-i)*xdim+k];
      T[j] = Is[(ydim-i-1)*xdim+k];
    }
      
    for (i=size; i<bdy; i++)
    {
      double sum = B[0]*T[i];

      for (j=1; j<size; j++)
        sum += B[j]*(T[i-j]+T[i+j]);

      Is[(i-size)*xdim+k] = sum;
    }
    delete[]T;
  }

  delete[]B;
}

