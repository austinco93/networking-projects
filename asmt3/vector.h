// vector.h                                                                     
// Austin Corotan
#define VECTOR_INITIAL_CAPACITY 80

// Define a vector type                                                         
typedef struct {
  int len;      // slots used so far                                            
  int size;  // total available slots                                           
  char *data;     // array of integers we're storing                            
} Vector;

void vector_init(Vector *vector);

void vector_append(Vector *vector, char* user, char* msg);

void vector_double_capacity_if_full(Vector *vector);

void vector_free(Vector *vector);

void vector_pop_front(Vector *vector, int count);

