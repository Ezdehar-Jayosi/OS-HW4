#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include <cassert>
#include <stdio.h>
#include <cstring>

#define META_SIZE sizeof(class MallocMetadata)
#define MAX_BLOCK_SIZE (pow(10, 8))

// META-DATA CLASS//
class MallocMetadata{
public:
bool is_free;
size_t req_size;
MallocMetadata* next;
MallocMetadata* prev;

};
MallocMetadata *listptr = NULL; //global pointer to metadata class list

/************** Aux Functions *****************/
MallocMetadata* find_last_block();
MallocMetadata* find_free_block(size_t requested_size);
MallocMetadata* allocate_space(size_t size){
MallocMetadata* new_block=(MallocMetadata*)sbrk(0);
    //void *requested = sbrk(size + sizeof(class MallocMetadata));
    if ((void *) sbrk(size + sizeof(class MallocMetadata)) == (void *) -1) {
        return NULL; // sbrk failed.
    }
	new_block->next = NULL;
    new_block->prev = NULL;
    new_block->req_size = size;
    new_block->is_free = false;
    MallocMetadata* last_block = find_last_block();
    if (last_block) {
       last_block->next = new_block;
	   new_block->prev = last_block;
    }
    return new_block;
}
MallocMetadata* find_last_block(){
	if(!listptr) return NULL;
	MallocMetadata* ptr = listptr;
    while (ptr->next){
		ptr = ptr->next;
    }
    return ptr;

}
MallocMetadata* find_free_block(size_t requested_size) {
    if(!listptr)return NULL;
    MallocMetadata* currPtr = listptr;
    while (currPtr &&
           (!(currPtr->is_free) || currPtr->req_size < requested_size)) {
        currPtr = currPtr->next;
    }
    return currPtr;
}

/************** Aux Functions For Testing *****************/
size_t _num_free_blocks(){
	MallocMetadata* ptr = listptr;
	size_t counter=0;
    while(ptr){
	  if(ptr->is_free) counter++;
	  ptr=ptr->next;
	}		
	return counter;
}

size_t _num_free_bytes(){
	MallocMetadata* ptr = listptr;
	size_t counter=0;
    while(ptr){
	  if(ptr->is_free) counter+=ptr->req_size;
	  ptr=ptr->next;
	}		
	return counter;
}

size_t _num_allocated_blocks(){
	MallocMetadata* ptr = listptr;
	size_t counter=0;
    while(ptr){
	  counter++;
	  ptr=ptr->next;
	}		
	return counter;
}

size_t _num_allocated_bytes(){
	MallocMetadata* ptr = listptr;
	size_t counter=0;
    while(ptr){
	  counter+=ptr->req_size;
	  ptr=ptr->next;
	}		
	return counter;

}

size_t _num_meta_data_bytes(){
    return ((_num_allocated_blocks())*(sizeof(class MallocMetadata)));

}

size_t _size_meta_data(){
	return sizeof(class MallocMetadata);
}

/************** Main Functions *****************/
void* smalloc(size_t size){
    if(size==0 || size > pow(10,8)) return NULL;
	//check if there exists a free block with the requested size
	MallocMetadata* result = find_free_block(size);
    if (result) {
        result->is_free = false;
        return result + 1;
    }
    //allocate a new block at the end of the global list since 
	//no free block was found with the requested size
    result = allocate_space(size);
    if (!result) {
        return NULL;  // sbrk failed
    }
	if(!listptr){  //if listptr is NULL then the list is empty 
				   //and we should allocate one at the start of the list
        listptr = result;
	}
    return result + 1;
}

void* scalloc(size_t num, size_t size){
    void *result = smalloc(num * size);
    //if malloc success
    if (result == (void *) NULL) {
        return result;
    }
    std::memset(result, 0, num * size);
    return result;
}

void sfree(void* p){
    if (!p) return;
    //MallocMetadata* to_free = (meta_data *) p - 1;
    if (((MallocMetadata*) p - 1)->is_free) return;
    ((MallocMetadata*) p - 1)->is_free = true;
}

void* srealloc(void* oldp, size_t size){
	if (size == 0 || (size > MAX_BLOCK_SIZE)) return NULL;

    if (!oldp) {
        return smalloc(size);
    }
    
    if (((MallocMetadata*) oldp - 1)->req_size >= size) {
        return oldp;
    }
    void *result = smalloc(size);
    if (result == (void *) NULL) {
        return result;
    }
    sfree(oldp);
    return std::memcpy(result, oldp, size);

}

