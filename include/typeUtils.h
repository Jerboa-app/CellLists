#ifndef TYPEUTILS_H
#define TYPEUTILS_H

// forces the number x to be rendered in exactly length characters as a string
std::string fixedLengthNumber(double x, unsigned length){
  std::string d = std::to_string(x);
  std::string dtrunc(length,' ');
  for (int c = 0; c < dtrunc.length(); c++){
    dtrunc[c] = d[c];
  }
  return dtrunc;
}

#endif
