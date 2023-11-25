#ifndef HOWARD_H
#  define HOWARD_H

#  ifdef __cplusplus
extern "C" {
#  endif

/* INPUT of Howard Algorithm =
   ij,A,nnodes,narcs : sparse description of a matrix
   verbosemode: normal value is zero
   higher values (1,2) yield more info for debugging purposes.
   Value -1 of verbosemode suppresses the checking of consistency
   of data.

   OUTPUT =
   chi cycle time vector
   v bias
   pi optimal policy
   NIterations: Number of iterations of the algorithm
   NComponents: Number of connected components of the optimal policy

   REQUIRES: O(nnodes) SPACE
   One iteration requires: O(narcs+nnodes) TIME

   Experimentally, the number of iterations N_H(nnodes,...) seems to grow
   slowy with the dimension, something like
   N_H(nnodes)=O(log(nnodes)) for full matrices.
   The matrix A must have at least one finite entry
   per row (this is checked when verbosemode is >=1)

*/


/*
  The following variables should be defined in the environment
  from which the Howard routine is called.

  INPUT VARIABLES
  int NNODES;
  number of nodes of the graph
  int NARCS;
  number of arcs of the graph
  int *IJ;
  array of integers of size 2*narcs

  for (0 <=k <narcs), the arc numbered k  goes from
  IJ[k][0] =(IJ[2k]) to IJ[k][1] (=IJ[2k+1])

  double *A;
  array of double of size narcs
  A[k]=weight of the arc numbered k

  OUTPUT VARIABLES

  double *V;
  array of double of size nnodes (the bias vector)
  double *CHI;
  array of double of size nnodes (the cycle time vector)
  int *POLICY;
  array of integer of size nnodes (an optimal policy)
  int NITERATIONS;
  integer: the number of iterations of the algorithm

  int NCOMPONENTS;
  integer: the number of connected components of the optimal
  policy which is returned.
*/
int Howard(int *IJ, double *A,int NNODES,int NARCS,double *CHI,double *V,int *POLICY,int *NITERATIONS,int *NCOMPONENTS,int VERBOSEMODE);

/*  Input of Semi_Howard is identical, plus
    double *T: array of double of size narcs (delays).
*/
int Semi_Howard(int *IJ, double *A,double *T,int NNODES,int NARCS,double *CHI,double *V,int *POLICY,int *NITERATIONS,int *NCOMPONENTS,int VERBOSEMODE);

#  ifdef __cplusplus
} // extern C
#  endif

#endif // HOWARD_H
