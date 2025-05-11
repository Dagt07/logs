#ifndef MERGESORT_HPP
#define MERGESORT_HPP

#include <string>

// Interface function to run external merge sort
long long run_mergesort(const std::string& inputFile, 
                        long N_SIZE, 
                        int a, 
                        long B_SIZE, 
                        long M_SIZE);

#endif