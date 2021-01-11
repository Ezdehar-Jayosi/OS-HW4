#include <cstdlib>
#include <unistd.h>
#include <cmath>

void* malloc(size_t size){
   if(size == 0)                  return NULL;
   if(size >  (pow(10,8)))        return NULL;
   void* result = sbrk(size);
   if (result == ((void*)(-1)))   return NULL;
   return result;
}
