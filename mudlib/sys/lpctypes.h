#ifndef LPC_LPCTYPES_H
#define LPC_LPCTYPES_H

/* compile time types, from functionlist() */

#define TYPE_UNKNOWN    0       /* This type must be casted */
#define TYPE_NUMBER     1
#define TYPE_STRING     2
#define TYPE_VOID       3
#define TYPE_OBJECT     4
#define TYPE_MAPPING    5
#define TYPE_FLOAT      6
#define TYPE_ANY        7       /* Will match any type */
#define TYPE_CLOSURE    8
#define TYPE_SYMBOL     9 
#define TYPE_QUOTED_ARRAY 10
#if defined(USE_STRUCTS) || defined(__LPC_STRUCTS__)
#define TYPE_STRUCT     11

#define TYPE_MOD_POINTER        0x0010  /* Pointer to a basic type        */
#else
#define TYPE_MOD_POINTER        0x0040  /* Pointer to a basic type        */
#endif

/* runtime types, from typeof() */

#define T_INVALID       0x0
#define T_LVALUE        0x1
#define T_NUMBER        0x2
#define T_STRING        0x3
#define T_POINTER       0x4
#define T_OBJECT        0x5
#define T_MAPPING       0x6
#define T_FLOAT         0x7
#define T_CLOSURE       0x8
#define T_SYMBOL        0x9
#define T_QUOTED_ARRAY  0xa

#ifndef __DRIVER_SOURCE__

#define CLOSURE_IS_LFUN(x)		(((x)&~1) == 0)
#define CLOSURE_IS_IDENTIFIER(x)	((x) == 2)
#define CLOSURE_IS_BOUND_LAMBDA(x)	((x) == 4)
#define CLOSURE_IS_LAMBDA(x)		((x) == 5)
#define CLOSURE_IS_UNBOUND_LAMBDA(x)	((x) == 6)
#define CLOSURE_IS_SIMUL_EFUN(x) (((x) & 0xf800) == 0xf800)
#define CLOSURE_IS_EFUN(x)	 (((x) & 0xf800) == 0xf000)
#define CLOSURE_IS_OPERATOR(x)	 (((x) & 0xf800) == 0xe800)

#endif /* __DRIVER_SOURCE__ */

#endif /* LPC_LPCTYPES_H */
