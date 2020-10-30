
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */

struct buf_stack;
struct data;

int sizeof_shm_stack(int size);
int init_shm_stack(struct buf_stack *p, int stack_size);
struct buf_stack *create_stack(int size);
void destroy_stack(struct buf_stack *p);
int is_full(struct buf_stack *p);
int is_empty(struct buf_stack *p);
int push(struct buf_stack *p, int seq, char* buf, size_t buf_size);
int pop(struct buf_stack *p, struct data* d);
