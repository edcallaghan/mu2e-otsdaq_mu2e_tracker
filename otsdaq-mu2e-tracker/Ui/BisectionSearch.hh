// Ed Callaghan
// Bisection search over a discrete search space, templated to avoid sorting out functor types
// July 2024

#ifndef BisectionSearch_hh
#define BisectionSearch_hh

#include <math.h>
#include <string>
#include "cetlib_except/exception.h"

// search for a threshold-crossing of a monotonically increasing function
// of one discrete variable, using the classic bisection method
//
// T is type of discrete search space
// U is type of target space
// functor is type of whatever is callable, e.g. a class with operator()
template<typename T, typename U, typename functor>
T bisection_search(functor f, U target, U tolerance, T left, T right){
  // first, some short-circuits:
  // a crossing may or may not be found, but search windows on discrete spaces
  // may collapse to a single point, at which time the search must terminate
  if (left == right){
    return left;
  }
  // sanity check that the bounds are ordered by increasing function value
  else if (!(left < right)){
    std::string msg = "unsorted bisection search bounds";
    throw cet::exception("bisection_search") << msg << std::endl;
  }
  // effectively the same as the first case: an incompressible search window
  else if (right - left == 1){
    return left;
  }

  // evaluate function in the middle of the search window
  T middle = static_cast<T>((left + right) / 2);
  U current = f(middle);
  U error = fabs(current - target);

  // branch for next iteration
  T rv = 0;
  // if within tolerance, exit
  if (error < tolerance){
    rv = middle;
  }
  // if we are left-of/under threshold, search the right half
  else if (current < target){
    rv = bisection_search(f, target, tolerance, middle, right);
  }
  // if we are right-of/above threshold, search the left half
  else if (target < current){
    rv = bisection_search(f, target, tolerance, left, middle);
  }
  // should never reach here
  else{
    std::string msg = "reached impossible state";
    throw cet::exception("bisection_search") << msg << std::endl;
  }

  return rv;
}

#endif
