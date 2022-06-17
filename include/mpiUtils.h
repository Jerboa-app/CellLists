#ifndef MPIUTILS_H
#define MPIUTILS_H

void distributeComputeElements(
  int nElements,
  int nProcesses,
  std::vector<std::vector<int>> & elementsOnProcess
){
  int n = std::floor(float(nElements) / float(nProcesses-1));
  if (n == 0) n = 1;
  // excess to be distributed fairly
  int k = nElements - (nProcesses-1)*n;
  int c = 0;
  int j = 0;
  int i = 0;
  while (i < nElements){
    elementsOnProcess[j+1].push_back(i);
    c += 1;
    if (c % n == 0){
      if ( (nProcesses-1-j) == k){
        // distribute one of the excess
        elementsOnProcess[j+1].push_back(i+1);
        i += 1;
        k -= 1;
      }
    }
    if (c >= n){
      c = 0;
      j += 1;
    }
    i += 1;
  }
}

void setupGatherIndices(
  int * disp,
  int * rec,
  std::vector< std::vector<int> > & elementsOnProcess,
  int N,
  int dataSize
){
  for (int i = 0; i < elementsOnProcess.size(); i++){
    if (i < N && i > 0){
      rec[i] = elementsOnProcess[i].size()*dataSize;
      disp[i] = rec[i-1]+disp[i-1];
    }
    else{
      rec[i] = 0;
      disp[i] = 0;
    }
  }
}

#endif
