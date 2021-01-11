#include <sys/mman.h>
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
MallocMetadata* listptr = NULL; //global pointer to metadata class list
MallocMetadata* listptrMM = NULL; //global pointer to metadata class list
void* scalloc(size_t num, size_t size);
void* smalloc(size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
/************** Aux Functions For Testing *****************/
size_t _num_free_blocks(){
	MallocMetadata* ptr = listptr;
	MallocMetadata* ptrMM = listptrMM;
	size_t counter=0;
    while(ptr){
	  if(ptr->is_free) counter++;
	  ptr=ptr->next;
	}		
	return counter;
}

size_t _num_free_bytes(){
	MallocMetadata* ptr = listptr;
	MallocMetadata* ptrMM = listptrMM;
	size_t counter=0;
    while(ptr){
	  if(ptr->is_free) counter+=ptr->req_size;
	  ptr=ptr->next;
	} 
	while(ptrMM){
	  if(ptrMM->is_free) counter+=ptrMM->req_size;
	  ptrMM=ptrMM->next;
	}		
	return counter;
}

size_t _num_allocated_blocks(){
	MallocMetadata* ptr = listptr;
	MallocMetadata* ptrMM = listptrMM;
	size_t counter=0;
    while(ptr){
	  counter++;
	  ptr=ptr->next;
	}
	while(ptrMM){
	  counter++;
	  ptrMM=ptrMM->next;
	}		
	return counter;
}

size_t _num_allocated_bytes(){
	MallocMetadata* ptr = listptr;
	MallocMetadata* ptrMM = listptrMM;
	size_t counter=0;
    while(ptr){
	  counter+=ptr->req_size;
	  ptr=ptr->next;
	}
	
	while(ptrMM){
	  counter+=ptrMM->req_size;
	  ptrMM=ptrMM->next;
	}		
	return counter;

}

size_t _num_meta_data_bytes(){
	MallocMetadata* ptr = listptr;
	size_t counter=0;
    while(ptr){
	  counter++;
	  ptr=ptr->next;
	}
	
    return (counter*(sizeof(class MallocMetadata)));

}

size_t _size_meta_data(){
	return sizeof(class MallocMetadata);
}

/************** Aux Functions *****************/
void* reallocHelper2(void* oldp ,MallocMetadata* block_ptr,size_t size,size_t remainder);
MallocMetadata* find_last_block();
MallocMetadata* find_last_blockMM();
MallocMetadata* find_free_block(size_t requested_size);
void* largeEnough(void* ptr,size_t size){
	MallocMetadata* block_ptr = (MallocMetadata*) ptr - 1;
	size_t remainder = block_ptr->req_size - size;
	if(remainder <= META_SIZE) return ptr;
    remainder-=META_SIZE;
    if ((block_ptr->req_size) > size   && (remainder >= 128)) {
	    return reallocHelper2(ptr,block_ptr,size,remainder);
	}
	return ptr;
}
MallocMetadata* allocate_space(size_t size){
	MallocMetadata* new_block;
	if(size >= 128*1024){
		new_block = (MallocMetadata*) mmap(NULL, size+ sizeof(class MallocMetadata),
					PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
			//if (MAP_FAILED == mmap(NULL, size+ sizeof(class MallocMetadata),
			//		PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0))
	    if((void*) new_block == (void *) -1) return (NULL);
	new_block->req_size = size;
    new_block->next = NULL;
    new_block->prev = NULL;
    new_block->is_free = false;
    MallocMetadata* last_block = find_last_blockMM();
    if (last_block) {
       last_block->next = new_block;
	   new_block->prev = last_block;
	 }
    return new_block;
	}else{
	new_block=(MallocMetadata*)sbrk(0);
    //void *requested = sbrk(size + sizeof(class MallocMetadata));
    if ((void *) sbrk(size + sizeof(class MallocMetadata)) == (void *) -1) {
        return NULL; // sbrk failed.
    }
	}
    new_block->req_size = size;
    new_block->next = NULL;
    new_block->prev = NULL;
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
MallocMetadata* find_last_blockMM(){
if(!listptrMM) return NULL;
MallocMetadata* ptr = listptrMM;
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
MallocMetadata* challenge1(MallocMetadata* block,size_t size){
	int diff = (size + META_SIZE > block->req_size) ? (size + META_SIZE - block->req_size) : (block->req_size - size - META_SIZE);
    if (diff < 128) {
        return block + 1;
    }
    block->is_free = false;
    MallocMetadata* new_block = (MallocMetadata*) (((char *) block) + size + META_SIZE);
    new_block->req_size = block->req_size - size - _size_meta_data();
    new_block->is_free = true;
    block->req_size = size ;
    MallocMetadata* next_ptr = block->next;
    block->next = new_block;
    new_block->next = next_ptr;
    new_block->prev = block;
    return block + 1;

}
MallocMetadata* challenge3(size_t size){
 MallocMetadata* block = find_last_block();
    if (block->next == NULL) {
        if (block->is_free) {
		    void *request = sbrk(size+META_SIZE);
			if (request == (void *) -1) return NULL; //sbrk failed
			
            block->req_size = size ;
            block->is_free = false;
            return block + 1;
        }
    }
}
void* reallocWilderness(void* oldp,MallocMetadata* block ,size_t size){
	if (block->next == NULL  ) {
		void *request = sbrk(size - (block->req_size));
        if (request == (void *) -1) return NULL; // sbrk failed.
        block->req_size = size;
        return oldp;
    }
    if(block->next && block->next->is_free  && (block->req_size + (block->next)->req_size + META_SIZE) >= size){
	    size_t res = block->req_size + (block->next)->req_size + META_SIZE;
		block->req_size += (block->next)->req_size + META_SIZE;
		block->next = block->next->next;
		if(block->next!=NULL) block->next->prev=block;
		if(res==size) return oldp;
		return largeEnough(oldp,size);
    }

	 if(block->prev && block->prev->is_free  && (block->req_size + (block->prev)->req_size + META_SIZE) >= size){
	    size_t res = block->req_size + (block->prev)->req_size + META_SIZE;
		block->prev->req_size += block->req_size + META_SIZE;
		block->prev->next = block->next;
		if(block->next) block->next->prev = block->prev;
		block->prev->is_free=false;
		if(res==size) return block->prev;
		//char *tmp_block = ((char *) oldp) + new_size;
		//meta_data *block = (meta_data *) tmp_block;
		void* newOldp = (char*) ((block->prev) +1);
		return largeEnough(newOldp,size);
    }
	 if(block->prev && block->next && block->prev->is_free  && block->next->is_free && (block->req_size + (block->prev)->req_size + block->next->req_size+ 2*META_SIZE) >= size){
		size_t res = block->req_size + (block->prev)->req_size +  block->next->req_size + 2*META_SIZE;
		block->prev->req_size += block->req_size +  block->next->req_size +2 *META_SIZE;
		block->prev->next = block->next->next;
		if(block->next->next) block->next->next->prev = block->prev;
		block->prev->is_free=false;
		if(res==size) return block->prev;
		//char *tmp_block = ((char *) oldp) + new_size;
		//meta_data *block = (meta_data *) tmp_block;
		void* newOldp = (char*) ((block->prev) + 1);
		return largeEnough(newOldp,size);
    }
	
    void *result = smalloc(size);
    if (result == (void *) NULL) return result;
	sfree(oldp);
    return std::memcpy(result, oldp, size);
}
void* reallocHelper2(void* oldp ,MallocMetadata* block_ptr,size_t size,size_t remainder){
	char *tmp_block = ((char *) oldp) + size;
    MallocMetadata* block = (MallocMetadata*) tmp_block;
    block->req_size = remainder;
    block->is_free = false;
    block_ptr->req_size = size;
    MallocMetadata* next_ptr = block_ptr->next;
    block_ptr->next = block;
    block->next = next_ptr;
    sfree(block+1);
    return oldp;
}


/************** Main Functions *****************/
void* smalloc(size_t size){
    if(size==0 || size > pow(10,8)) return NULL;
	 size_t req_size = META_SIZE + size;
    // round requested size to multiplication of 8
    //size_t arch_bytes = sizeof(size_t)/8;
    if (req_size % 8 != 0) {
        req_size = req_size + (8 - req_size % 8);
    }
	if(size >= 128*1024){
		MallocMetadata* result = allocate_space(req_size-META_SIZE);
		if(!listptrMM){  //if listptr is NULL then the list is empty 
				   //and we should allocate one at the start of the list
        listptrMM = result;
	   }
        return (result + 1);
	}
	if(!listptr){  //if listptr is NULL then the list is empty 
				   //and we should allocate one at the start of the list
		MallocMetadata* result = allocate_space(req_size-META_SIZE);
        listptr = result;
        return (result + 1);
	}
	//check if there exists a free block with the requested size
	MallocMetadata* result = find_free_block(req_size-META_SIZE);
    if (result) {
        result->is_free = false;
        return challenge1(result,req_size-META_SIZE);
    }
  //check for the wilderness block
	result = challenge3(req_size-META_SIZE);
	if(result) return result ;
	  //allocate a new block at the end of the global list since 
	//no free block was found with the requested size
    result = allocate_space(req_size-META_SIZE);
    if (!result) {
        return NULL;  // sbrk failed
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
	//printf("freeing block with size %d and isFree= %d\n",((MallocMetadata*) p - 1)->req_size,((MallocMetadata*) p - 1)->is_free);
	if (((MallocMetadata*) p - 1)->is_free) return;
    ((MallocMetadata*) p - 1)->is_free = true;
	MallocMetadata* freedblock = ((MallocMetadata*) p - 1);
	if(freedblock->req_size >= 128*1024){
		if(freedblock->prev) freedblock->prev->next=freedblock->next;
		if(freedblock->next) freedblock->next->prev=freedblock->prev;
		if(freedblock->prev==NULL && freedblock->next ==NULL) listptrMM=NULL;
	    munmap(p, freedblock->req_size + META_SIZE );
	    return;
	}
    if ((freedblock->next) && ((freedblock->next)->is_free)) {
        freedblock->req_size += (freedblock->next)->req_size + META_SIZE;
        freedblock->next = freedblock->next->next;
		if(freedblock->next) freedblock->next->prev=freedblock;
    }
    //we have freed the first meta data
    if(!(freedblock->prev)) return;// the freed one is the first one
	if(((freedblock)->prev)->is_free){
		((freedblock)->prev)->req_size += freedblock->req_size + META_SIZE;
		((freedblock)->prev)->next = freedblock->next;
		if(freedblock->next) (freedblock->next)->prev=((freedblock)->prev);
	}
}

void* srealloc(void* oldp, size_t size){
	if (size == 0 || (size > MAX_BLOCK_SIZE)) return NULL;
	 size_t req_size = META_SIZE + size;
    // round requested size to multiplication of 8
    //size_t arch_bytes = sizeof(size_t)/8;
    if (req_size % 8 != 0) {
        req_size = req_size + (8 - req_size % 8);
    }
    if (!oldp) {
        return smalloc(req_size-META_SIZE);
    }
	if(req_size-META_SIZE >= 128*1024 && ((MallocMetadata*) oldp - 1)->req_size >= 128*1024 ){
		MallocMetadata* freedblock = ((MallocMetadata*) oldp - 1);
		MallocMetadata* new_block = (MallocMetadata*) mmap(NULL, req_size,
					PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	    if((void*) new_block == (void *) -1) return (NULL);
		//std::memcpy(new_block, oldp, size);
		new_block->req_size=req_size-META_SIZE;
		 size_t oldsize=freedblock->req_size+META_SIZE;
		 freedblock->req_size=req_size-META_SIZE;
		new_block->next=NULL;
		new_block->prev=NULL;
		if(freedblock->prev) freedblock->prev->next=new_block;
		if(freedblock->next) freedblock->next->prev=new_block;
		if(!freedblock->next && !freedblock->prev) listptrMM = new_block;
		 new_block->next = freedblock->next;
		 new_block->prev = freedblock->prev;
	     munmap(oldp, oldsize );
		 void* result = new_block+1;
		 return std::memcpy(result, oldp,req_size-META_SIZE);
	}
    //MallocMetadata* meta_ptr = (MallocMetadata*) oldp - 1;
    if (((MallocMetadata*) oldp - 1)->req_size >= req_size-META_SIZE) {
        return oldp;
    }
	
    MallocMetadata* block_ptr = ((MallocMetadata*) oldp) - 1;
	if((block_ptr->req_size) >= req_size-META_SIZE) return oldp;
	if((req_size-META_SIZE > (block_ptr->req_size))) return reallocWilderness(oldp,block_ptr,req_size-META_SIZE);// && (!(block_ptr->next) || (block_ptr->next && block_ptr->next->is_free ))
	
	sfree(oldp);
    void *result = smalloc(req_size-META_SIZE);
    if (result == (void *) NULL) {
        return result;
    }
    return std::memcpy(result, oldp, req_size-META_SIZE);
}

