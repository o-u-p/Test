#ifndef LIBRARY_H
#define LIBRARY_H
#if defined(__GNUC__)
#define FORCEDINLINE __attribute__((always_inline))
#else
#define FORCEDINLINE
#endif

extern FORCEDINLINE int addNumbers(int a, int b);

#endif