#include <iostream>
#include <sys/time.h>
#include <mpi.h>
using namespace std;

#define BLOCKLENGTH 10000

void mpiSendData(int *data, int length, int dest) {
  if (length == 0) {
    MPI_Send(data, 0, MPI_INT, dest, 0, MPI_COMM_WORLD);
    return;
  }
  while (length > 0) {
    if (length >= BLOCKLENGTH) {
      MPI_Send(data, BLOCKLENGTH, MPI_INT, dest, 0, MPI_COMM_WORLD);
      data += BLOCKLENGTH;
      length -= BLOCKLENGTH;
      if (length == 0)
        MPI_Send(data, 0, MPI_INT, dest, 0, MPI_COMM_WORLD);
    }
    else {
      MPI_Send(data, length, MPI_INT, dest, 0, MPI_COMM_WORLD);
      length = 0;
    }
  }
}

int mpiRecvData(int *data, int maxLength, int src) {
  MPI_Status status;
  int recvSize;
  int totalRecv = 0;
  do {
    MPI_Recv(data, maxLength, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &recvSize);
    data += recvSize;
    totalRecv += recvSize;
  } while (recvSize == BLOCKLENGTH);
  return totalRecv;
}

void swap(int *data, int i, int j) {
  int temp = data[i];
  data[i] = data[j];
  data[j] = temp;
}

int partition(int *data, int start, int end) {
  if (start >= end) return 0;

  int pivotValue = data[start];
  int low = start;
  int high = end - 1;
  while (low < high) {
    while (data[low] <= pivotValue && low < end) low++;
    while (data[high] > pivotValue && high > start) high--;
    if (low < high) swap(data, low, high);
  }
  swap(data, start, high);

  return high;
}

void quicksort(int *data, int start, int end) {
  if  (end-start+1 < 2) return;

  int pivot = partition(data, start, end);

  quicksort(data, start, pivot);
  quicksort(data, pivot+1, end);
}

void sort(int *data, int totalLength, int& localLength, int procId, int numProcessors) {
  // Send all of the data to processor 0
  int i;
  if (procId == 0) {
    for (i=1; i<numProcessors; i++) {
      mpiRecvData(data+i*localLength, localLength, i);
    }
    localLength = totalLength;
  }
  else {
    mpiSendData(data, localLength, 0);
    localLength = 0;
  }

  // Do recursive quicksort starting at processor 0 and spreading out recursive calls to other machines
  int s;
  int pivot;
  for (s=numProcessors; s > 1; s /= 2) {
    if (procId % s == 0) {
      pivot = partition(data, 0, localLength);

      // Send everything after the pivot to processor procId + s/2 and keep up to the pivot
      mpiSendData(data+pivot, localLength - pivot, procId + s/2);
      localLength = pivot;
    }
    else if (procId % s == s/2) {
      // Get data from processor procId - s/2
      localLength = mpiRecvData(data, totalLength, procId - s/2);
    }
  }

  // Perform local sort
  quicksort(data, 0, localLength);
}
