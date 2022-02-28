#ifndef HOWARD_H
#  define HOWARD_H

#  ifdef __cplusplus
extern "C" {
#  endif

int Howard(int *IJ, double *A,int NNODES,int NARCS,double *CHI,double *V,int *POLICY,int *NITERATIONS,int *NCOMPONENTS,int VERBOSEMODE);
int Semi_Howard(int *IJ, double *A,double *T,int NNODES,int NARCS,double *CHI,double *V,int *POLICY,int *NITERATIONS,int *NCOMPONENTS,int VERBOSEMODE);

#  ifdef __cplusplus
} // extern C
#  endif

#endif // HOWARD_H
