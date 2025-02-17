#include <stdlib.h>
struct fptr
{
int (*p_fptr)(int, int);
};
struct fsptr
{
struct fptr * sptr;
};
struct wfsptr
{
  struct fsptr * wfptr;
};
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}
void make_simple_alias(struct wfsptr * a_fptr,struct fsptr * b_fptr)
{
  a_fptr->wfptr=b_fptr;
}
void make_alias(struct wfsptr* a_fptr,struct wfsptr * b_fptr)
{
  a_fptr->wfptr->sptr=b_fptr->wfptr->sptr;
}
void swap_w(struct wfsptr * a_fptr,struct wfsptr * b_fptr)
{
     struct wfsptr wftemp=*a_fptr;
     *a_fptr=*b_fptr;
     *b_fptr=wftemp;
}

struct fptr * foo(int a, int b, struct wfsptr * a_fptr, struct wfsptr * b_fptr) {
   if(a>0 && b<0)
   {
    swap_w(a_fptr,b_fptr);
    b_fptr->wfptr->sptr->p_fptr(a,b);
    return a_fptr->wfptr->sptr;
   }
   a_fptr->wfptr->sptr->p_fptr=minus;
   return b_fptr->wfptr->sptr;
} 

struct fptr * clever(int a, int b, struct fsptr * a_fptr, struct fsptr * b_fptr ) {
   struct wfsptr t1_fptr;
   make_simple_alias(&t1_fptr,a_fptr);
   struct wfsptr t2_fptr;
   make_simple_alias(&t2_fptr,b_fptr);
   return foo(a,b,&t1_fptr,&t2_fptr);
}


int moo(char x, int op1, int op2) {
    struct fptr a_fptr ;
    a_fptr.p_fptr=plus;
    struct fptr s_fptr ;
    s_fptr.p_fptr=minus;

    struct fsptr m_fptr;
    m_fptr.sptr=&a_fptr;
    struct fsptr n_fptr;
    n_fptr.sptr=&s_fptr;
    
    struct wfsptr w_fptr;
    make_simple_alias(&w_fptr,&m_fptr);
    struct wfsptr x_fptr;
    make_simple_alias(&x_fptr,&n_fptr);

    struct fsptr k_fptr;
    struct wfsptr y_fptr;
    y_fptr.wfptr= & k_fptr;
    make_alias(&y_fptr,&x_fptr);

    struct fptr* t_fptr = 0;

    clever(op1, op2, &m_fptr, y_fptr.wfptr);
    t_fptr = clever(op1,op2,y_fptr.wfptr,x_fptr.wfptr);
    t_fptr->p_fptr(op1,op2);
    
    return 0;
}

// 39 : swap_w
// 40 : plus, minus
// 49 : make_simple_alias
// 51 : make_simple_alias
// 52 : foo
// 68 : make_simple_alias
// 70 : make_simple_alias
// 75 : make_alias
// 79 : clever
// 80 : clever
// 81 : plus, minus
