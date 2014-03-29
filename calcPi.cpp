#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <mpi.h>

using namespace std;

void calcPi(long firstTerm, long lastTerm, double& result) {
  for (long i = firstTerm; i < lastTerm; i++)
    if (i%2 == 0)
      result += 4./(2*i+1);
    else
      result -= 4./(2*i+1);
}

int main(int argc, char **argv) {
  MPI_Init( &argc, &argv);
  long numTerms = atol(argv[1]);

  int rank, size;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  double pi;
  calcPi((numTerms*rank/size), (numTerms*(rank+1)/size), pi);

  // Send all of the partial results to processor 0
  if (rank == 0) {
    double data;
    MPI_Status status;

    for (int i=1; i<size; i++) {
      MPI_Recv(&data, 1, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      pi += data;
    }
    cout << "Pi is approximately " << setprecision(18) << pi << endl;
  }
  else {
    MPI_Send(&pi, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
  
  MPI_Finalize();
 return 0;
}
