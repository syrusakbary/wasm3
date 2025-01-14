//
//  m3_exec.h
//  M3: Massey Meta Machine
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#ifndef m3_exec_h
#define m3_exec_h

// TODO: all these functions could move over to the .c at some point. normally, i'd say screw it,
// but it might prove useful to be able to compile m3_exec alone w/ optimizations while the remaining
// code is at debug O0

#include "m3_exec_defs.h"
#include "m3_math_utils.h"

#include <math.h>
#include <limits.h>

# define rewrite(NAME)              * ((void **) (_pc-1)) = (void*)(NAME)

# define d_m3RetSig                 static inline m3ret_t vectorcall
# define d_m3Op(NAME)               d_m3RetSig op_##NAME (d_m3OpSig)

# define d_m3OpDef(NAME)            m3ret_t vectorcall op_##NAME (d_m3OpSig)
# define d_m3OpDecl(NAME)           d_m3OpDef (NAME);

# define immediate(TYPE)            * ((TYPE *) _pc++)
# define skip_immediate(TYPE)       (void)* ((TYPE *) _pc++)

# define slot(TYPE)                 * (TYPE *) (_sp + immediate (i32))

#define nextOpDirect()              ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs)
#define jumpOpDirect(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs)

# if d_m3EnableOpProfiling
#   define nextOp()                 profileOp (d_m3OpAllArgs, __PRETTY_FUNCTION__)
# elif d_m3TraceExec
#   define nextOp()                 debugOp (d_m3OpAllArgs, __PRETTY_FUNCTION__)
# else
#   define nextOp()                 nextOpDirect()
# endif

#define jumpOp(PC)                  jumpOpDirect((pc_t)PC)

d_m3RetSig  Call  (d_m3OpSig)
{
    m3Yield ();
    return nextOpDirect();
}


d_m3RetSig  debugOp  (d_m3OpSig, cstr_t i_opcode)
{
    char name [100];
    strcpy (name, strstr (i_opcode, "op_") + 3);
    char * bracket = strstr (name, "(");
    if (bracket) {
        *bracket  = 0;
    }

    puts (name);
    return nextOpDirect();
}

static const u32 c_m3ProfilerSlotMask = 0xFFFF;

typedef struct M3ProfilerSlot
{
    cstr_t      opName;
    u64         hitCount;
}
M3ProfilerSlot;

void  ProfileHit  (cstr_t i_operationName);

d_m3RetSig  profileOp  (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit (i_operationName);

    return nextOpDirect();
}

#if d_m3RuntimeStackDumps
d_m3OpDecl  (DumpStack)
#endif



// TODO: OK, this needs some explanation here ;0

#define d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE * stack = (TYPE *) (_sp + immediate (i32));    \
    OP((RES), (* stack), ((TYPE) REG), ##__VA_ARGS__);  \
    return nextOp ();                                   \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE * stackB = (TYPE *) (_sp + immediate (i32));   \
    TYPE * stackA = (TYPE *) (_sp + immediate (i32));   \
    OP((RES), (* stackA), (* stackB), ##__VA_ARGS__);   \
    return nextOp ();                                   \
}

#define d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE * stack = (TYPE *) (_sp + immediate (i32));    \
    OP((RES), ((TYPE) REG), (* stack), ##__VA_ARGS__);  \
    return nextOp ();                                   \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#define d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#define M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))    // Accept functions: res = OP(a,b)
#define M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))    // Accept operators: res = a OP b

#define d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#define d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#define d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// compare needs to be distinct for fp 'cause the result must be _r0
#define d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


//-----------------------

// signed
d_m3CommutativeOp_i (i32, Equal,            ==)     d_m3CommutativeOp_i (i64, Equal,            ==)
d_m3CommutativeOp_i (i32, NotEqual,         !=)     d_m3CommutativeOp_i (i64, NotEqual,         !=)

d_m3Op_i (i32, LessThan,                    < )     d_m3Op_i (i64, LessThan,                    < )
d_m3Op_i (i32, GreaterThan,                 > )     d_m3Op_i (i64, GreaterThan,                 > )
d_m3Op_i (i32, LessThanOrEqual,             <=)     d_m3Op_i (i64, LessThanOrEqual,             <=)
d_m3Op_i (i32, GreaterThanOrEqual,          >=)     d_m3Op_i (i64, GreaterThanOrEqual,          >=)

// unsigned
d_m3Op_i (u32, LessThan,                    < )     d_m3Op_i (u64, LessThan,                    < )
d_m3Op_i (u32, GreaterThan,                 > )     d_m3Op_i (u64, GreaterThan,                 > )
d_m3Op_i (u32, LessThanOrEqual,             <=)     d_m3Op_i (u64, LessThanOrEqual,             <=)
d_m3Op_i (u32, GreaterThanOrEqual,          >=)     d_m3Op_i (u64, GreaterThanOrEqual,          >=)

// float
d_m3CommutativeCmpOp_f (f32, Equal,         ==)     d_m3CommutativeCmpOp_f (f64, Equal,         ==)
d_m3CommutativeCmpOp_f (f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f (f64, NotEqual,      !=)
d_m3CompareOp_f (f32, LessThan,             < )     d_m3CompareOp_f (f64, LessThan,             < )
d_m3CompareOp_f (f32, GreaterThan,          > )     d_m3CompareOp_f (f64, GreaterThan,          > )
d_m3CompareOp_f (f32, LessThanOrEqual,      <=)     d_m3CompareOp_f (f64, LessThanOrEqual,      <=)
d_m3CompareOp_f (f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f (f64, GreaterThanOrEqual,   >=)


d_m3CommutativeOp_i (i32, Add,              +)      d_m3CommutativeOp_i (i64, Add,              +)
d_m3CommutativeOp_i (i32, Multiply,         *)      d_m3CommutativeOp_i (i64, Multiply,         *)

d_m3Op_i (i32, Subtract,                    -)      d_m3Op_i (i64, Subtract,                    -)

// Note: For some reason modulo is needed for Clang
#define OP_SHL_32(X,N) (X << (N % 32))
#define OP_SHL_64(X,N) (X << (N % 64))
#define OP_SHR_32(X,N) (X >> (N % 32))
#define OP_SHR_64(X,N) (X >> (N % 64))

d_m3OpFunc_i (i32, ShiftLeft,       OP_SHL_32)      d_m3OpFunc_i (i64, ShiftLeft,       OP_SHL_64)
d_m3OpFunc_i (i32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (i64, ShiftRight,      OP_SHR_64)
d_m3OpFunc_i (u32, ShiftRight,      OP_SHR_32)      d_m3OpFunc_i (u64, ShiftRight,      OP_SHR_64)

d_m3CommutativeOp_i (u64, And,              &)
d_m3CommutativeOp_i (u64, Or,               |)
d_m3CommutativeOp_i (u64, Xor,              ^)

d_m3CommutativeOp_f (f32, Add,              +)      d_m3CommutativeOp_f (f64, Add,              +)
d_m3CommutativeOp_f (f32, Multiply,         *)      d_m3CommutativeOp_f (f64, Multiply,         *)
d_m3Op_f (f32, Subtract,                    -)      d_m3Op_f (f64, Subtract,                    -)
d_m3Op_f (f32, Divide,                      /)      d_m3Op_f (f64, Divide,                      /)


d_m3OpFunc_i(u32, Rotl, rotl32)
d_m3OpFunc_i(u32, Rotr, rotr32)
d_m3OpFunc_i(u64, Rotl, rotl64)
d_m3OpFunc_i(u64, Rotr, rotr64)

d_m3OpMacro_i(u32, Divide, OP_DIV_U);
d_m3OpMacro_i(i32, Divide, OP_DIV_S, INT32_MIN);
d_m3OpMacro_i(u64, Divide, OP_DIV_U);
d_m3OpMacro_i(i64, Divide, OP_DIV_S, INT64_MIN);

d_m3OpMacro_i(u32, Remainder, OP_REM_U);
d_m3OpMacro_i(i32, Remainder, OP_REM_S, INT32_MIN);
d_m3OpMacro_i(u64, Remainder, OP_REM_U);
d_m3OpMacro_i(i64, Remainder, OP_REM_S, INT64_MIN);

d_m3OpFunc_f(f32, Min, min_f32);
d_m3OpFunc_f(f32, Max, max_f32);
d_m3OpFunc_f(f64, Min, min_f64);
d_m3OpFunc_f(f64, Max, max_f64);

d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#define d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    return nextOp ();                               \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE * stack = (TYPE *) (_sp + immediate (i32));\
    OP((RES), (* stack), ##__VA_ARGS__);            \
    return nextOp ();                               \
}

#define M3_UNARY(RES, X, OP) (RES) = OP(X)
#define d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#define d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)

d_m3UnaryOp_f (f32, Abs,        fabsf);         d_m3UnaryOp_f (f64, Abs,        fabs);
d_m3UnaryOp_f (f32, Ceil,       ceilf);         d_m3UnaryOp_f (f64, Ceil,       ceil);
d_m3UnaryOp_f (f32, Floor,      floorf);        d_m3UnaryOp_f (f64, Floor,      floor);
d_m3UnaryOp_f (f32, Trunc,      truncf);        d_m3UnaryOp_f (f64, Trunc,      trunc);
d_m3UnaryOp_f (f32, Sqrt,       sqrtf);         d_m3UnaryOp_f (f64, Sqrt,       sqrt);
d_m3UnaryOp_f (f32, Nearest,    rintf);         d_m3UnaryOp_f (f64, Nearest,    rint);
d_m3UnaryOp_f (f32, Negate,     -);             d_m3UnaryOp_f (f64, Negate,     -);


#define OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i (i32, EqualToZero, OP_EQZ)
d_m3UnaryOp_i (i64, EqualToZero, OP_EQZ)

// clz(0), ctz(0) results are undefined, fix it
#define OP_CLZ_32(x) (((x) == 0) ? 32 : __builtin_clz(x))
#define OP_CTZ_32(x) (((x) == 0) ? 32 : __builtin_ctz(x))
#define OP_CLZ_64(x) (((x) == 0) ? 64 : __builtin_clzll(x))
#define OP_CTZ_64(x) (((x) == 0) ? 64 : __builtin_ctzll(x))

d_m3UnaryOp_i (u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i (u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i (u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i (u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i (u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i (u64, Popcnt, __builtin_popcountll)

#define OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3UnaryOp_i (i32, Wrap_i64, OP_WRAP_I64)


#define d_m3TruncMacro(RES, REG, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r)                           \
{                                                   \
    OP((RES), (FROM) REG, ##__VA_ARGS__);           \
    return nextOp ();                               \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s)                  \
{                                                   \
	FROM * stack = (FROM *) (_sp + immediate (i32));\
    OP((RES), (* stack), ##__VA_ARGS__);            \
    return nextOp ();                               \
}

d_m3TruncMacro(_r0, _fp0, i32, Trunc, f32, OP_I32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f32, OP_U32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f64, OP_I32_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f64, OP_U32_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i64, Trunc, f32, OP_I64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f32, OP_U64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i64, Trunc, f64, OP_I64_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f64, OP_U64_TRUNC_F64)


#define d_m3TypeConvertOp(REG_TO, REG_FROM, TO, NAME, FROM) \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
	REG_TO = (TO) ((FROM) REG_FROM);                        \
    return nextOp ();                                       \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM * stack = (FROM *) (_sp + immediate (i32));        \
    REG_TO = (TO) (* stack);                                \
    return nextOp ();                                       \
}

// Int to int
d_m3TypeConvertOp(_r0, _r0, i64, Extend, i32);
d_m3TypeConvertOp(_r0, _r0, i64, Extend, u32);

// Int to float
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);

// Float to float
d_m3TypeConvertOp(_fp0, _fp0, f32, Demote, f64);
d_m3TypeConvertOp(_fp0, _fp0, f64, Promote, f32);

#define d_m3ReinterpretOp(REG, TO, SRC, FROM)               \
d_m3Op(TO##_Reinterpret_##FROM##_r)                         \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = (FROM)SRC;                                        \
    REG = u.t;                                              \
    return nextOp ();                                       \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s)                         \
{                                                           \
    union { FROM c; TO t; } u;                              \
    u.c = *(FROM *) (_sp + immediate (i32));                \
    REG = u.t;                                              \
    return nextOp ();                                       \
}

d_m3ReinterpretOp (_r0, i32, _fp0, f32)
d_m3ReinterpretOp (_r0, i64, _fp0, f64)
d_m3ReinterpretOp (_fp0, f32, _r0, i32)
d_m3ReinterpretOp (_fp0, f64, _r0, i64)

d_m3Op  (Nop)
{
    return nextOp ();
}


d_m3Op  (Block)
{
    return nextOp ();
}



d_m3OpDecl  (Loop)
d_m3OpDecl  (If_r)
d_m3OpDecl  (If_s)


d_m3Op  (Select_i_ssr)
{
    i32 condition = (i32) _r0;

    i64 operand2 = * (_sp + immediate (i32));
    i64 operand1 = * (_sp + immediate (i32));

    _r0 = (condition) ? operand1 : operand2;

    return nextOp ();
}

d_m3Op  (Select_i_srs)
{
    i32 condition = (i32) * (_sp + immediate (i32));

    i64 operand2 = _r0;
    i64 operand1 = * (_sp + immediate (i32));

    _r0 = (condition) ? operand1 : operand2;

    return nextOp ();
}


d_m3Op  (Select_i_rss)
{
    i32 condition = (i32) * (_sp + immediate (i32));

    i64 operand2 = * (_sp + immediate (i32));
    i64 operand1 = _r0;

    _r0 = (condition) ? operand1 : operand2;

    return nextOp ();
}


d_m3Op  (Select_i_sss)
{
    i32 condition = (i32) * (_sp + immediate (i32));

    i64 operand2 = * (_sp + immediate (i32));
    i64 operand1 = * (_sp + immediate (i32));

    _r0 = (condition) ? operand1 : operand2;

    return nextOp ();
}


d_m3Op  (Select_f)
{
    i32 condition = (i32) _r0;

    f64 operand2 = * (f64 *) (_sp + immediate (i32));
    f64 operand1 = * (f64 *) (_sp + immediate (i32));

    _fp0 = (condition) ? operand1 : operand2;

    return nextOp ();
}


d_m3Op  (Return)
{
    m3StackCheck();
    return 0;
}


d_m3Op  (Branch)
{
    return jumpOp (* _pc);
}


d_m3Op  (Bridge)
{
    return jumpOp (* _pc);
}


d_m3Op  (BranchIf)
{
    i32 condition   = (i32) _r0;
    pc_t branch     = immediate (pc_t);

    if (condition)
    {
        return jumpOp (branch);
    }
    else return nextOp ();
}


d_m3Op  (BranchTable)
{
    i32 index       = (i32) _r0;
    u32 numTargets  = immediate (u32);

    pc_t * branches = (pc_t *) _pc;

    if (index < 0 or index > numTargets)
        index = numTargets; // the default index

    return jumpOp (branches [index]);
}


d_m3Op  (ContinueLoop)
{
    // TODO: this is where execution can "escape" the M3 code and callback to the client / fiber switch
    // OR it can go in the Loop operation

    void * loopId = immediate (void *);
    return loopId;
}


d_m3Op  (ContinueLoopIf)
{
    i32 condition = (i32) _r0;
    void * loopId = immediate (void *);

    if (condition)
    {
        return loopId;
    }
    else return nextOp ();
}



d_m3OpDecl  (Compile)
d_m3OpDecl  (Call)
d_m3OpDecl  (CallIndirect)
d_m3OpDecl  (Entry)


d_m3Op  (Const)
{
    u64 constant = immediate (u64);
    i32 offset = immediate (i32);
    * (_sp + offset) = constant;

    return nextOp ();
}


d_m3Op  (Trap)
{                                                   m3log (exec, "*** trapping ***");
    m3StackCheck();
    return c_m3Err_runtimeTrap;
}


d_m3Op  (End)
{
    m3StackCheck();
    return 0;
}



d_m3Op  (GetGlobal)
{
    i64 * global = immediate (i64 *);

//  printf ("get global: %p %" PRIi64 "\n", global, *global);

    i32 offset  = immediate (i32);
    * (_sp + offset) = * global;

    return nextOp ();
}


d_m3Op  (SetGlobal_s)
{
    i64 * global = immediate (i64 *);
    i32 offset  = immediate (i32);
    * global = * (_sp + offset);

    return nextOp ();
}


d_m3Op  (SetGlobal_i)
{
    i64 * global = immediate (i64 *);
    * global = _r0;

//  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    return nextOp ();
}


d_m3Op  (SetGlobal_f)
{
    f64 * global = immediate (f64 *);
    * global = _fp0;

    return nextOp ();
}


d_m3Op (CopySlot)
{
    u64 * dst = _sp + immediate (i32);
    u64 * src = _sp + immediate (i32);

    * dst = * src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    return nextOp ();
}


d_m3Op (PreserveCopySlot)
{
    u64 * dest = _sp + immediate (i32);
    u64 * src = _sp + immediate (i32);
    u64 * preserve = _sp + immediate (i32);

    * preserve = * dest;
    * dest = * src;

    return nextOp ();
}


d_m3Op  (SetRegister_i)
{
    i32 offset = immediate (i32);

    u64 * stack = _sp + offset;
    _r0 = * stack;

    return nextOp ();
}


d_m3Op  (SwapRegister_i)
{
    slot (u64) = _r0;
    _r0 = slot (u64);

    return nextOp ();
}


d_m3Op  (SetRegister_f)
{
    i32 offset = immediate (i32);

    f64 * stack = (f64 *) _sp + offset;
    _fp0 = * stack;

    return nextOp ();
}


d_m3Op (SetSlot_i)
{
    i32 offset = immediate (i32);

//  printf ("setslot_i %d\n", offset);

    u64 * stack = _sp + offset;
    * stack = _r0;

    return nextOp ();
}


d_m3Op (PreserveSetSlot_i)
{
    u64 * stack = (u64 *) _sp + immediate (i32);
    u64 * preserve = (u64 *) _sp + immediate (i32);
    * preserve = * stack;
    * stack = _r0;

    return nextOp ();
}


d_m3Op (SetSlot_f)
{
    i32 offset = immediate (i32);
    f64 * stack = (f64 *) _sp + offset;
    * stack = _fp0;

    return nextOp ();
}


d_m3Op (PreserveSetSlot_f)
{
    f64 * stack = (f64 *) _sp + immediate (i32);
    f64 * preserve = (f64 *) _sp + immediate (i32);
    * preserve = * stack;
    * stack = _fp0;

    return nextOp ();
}


#define d_outOfBounds return c_m3Err_trapOutOfBoundsMemoryAccess

m3ret_t ReportOutOfBoundsMemoryError (pc_t i_pc, u8 * i_mem, u32 i_offset);

//#define d_outOfBounds { return ReportOutOfBoundsMemoryError (_pc, _mem, operand); }


#define d_m3Load(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    u32 offset = immediate (u32);                       \
    u32 operand = (u32) _r0;                            \
                                                        \
    u8 * src8 = _mem + operand + offset;                \
    u8 * end = * ((u8 **) _mem - 1);                    \
                                                        \
    if (src8 + sizeof (SRC_TYPE) <= end)                \
    {                                                   \
        REG = (DEST_TYPE) (* (SRC_TYPE *) src8);        \
        return nextOp ();                               \
    }                                                   \
    else d_outOfBounds;                                 \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    u32 operand = * (u32 *) (_sp + immediate (i32));    \
    u32 offset = immediate (u32);                       \
                                                        \
    u8 * src8 = _mem + operand + offset;                \
    u8 * end = * ((u8 **) _mem - 1);                    \
                                                        \
    if (src8 + sizeof (SRC_TYPE) <= end)                \
    {                                                   \
        REG = (DEST_TYPE) (* (SRC_TYPE *) src8);        \
        return nextOp ();                               \
    }                                                   \
    else d_outOfBounds;                                 \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#define d_m3Load_i(DEST_TYPE, SRC_TYPE) d_m3Load(_r0, DEST_TYPE, SRC_TYPE)
#define d_m3Load_f(DEST_TYPE, SRC_TYPE) d_m3Load(_fp0, DEST_TYPE, SRC_TYPE)

d_m3Load_f (f32, f32);
d_m3Load_f (f64, f64);

d_m3Load_i (i32, i8);
d_m3Load_i (i32, u8);
d_m3Load_i (i32, i16);
d_m3Load_i (i32, u16);
d_m3Load_i (i32, i32);

d_m3Load_i (i64, i8);
d_m3Load_i (i64, u8);
d_m3Load_i (i64, i16);
d_m3Load_i (i64, u16);
d_m3Load_i (i64, i32);
d_m3Load_i (i64, u32);
d_m3Load_i (i64, i64);

#define d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    u32 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    u8 * end = * ((u8 **) _mem - 1);                    \
    u8 * mem8 = (u8 *) (_mem + operand);                \
    if (mem8 + sizeof (DEST_TYPE) <= end)               \
    {                                                   \
        * (DEST_TYPE *) mem8 = (DEST_TYPE) REG;         \
        return nextOp ();                               \
    }                                                   \
    else d_outOfBounds;                                 \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    SRC_TYPE value = slot (SRC_TYPE);                   \
    u32 operand = (u32) REG;                            \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    u8 * end = * ((u8 **) _mem - 1);                    \
    u8 * mem8 = (u8 *) (_mem + operand);                \
    if (mem8 + sizeof (DEST_TYPE) <= end)               \
    {                                                   \
        * (DEST_TYPE *) mem8 = value;                   \
        return nextOp ();                               \
    }                                                   \
    else d_outOfBounds;                                 \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    SRC_TYPE value = slot (SRC_TYPE);                   \
    u32 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    operand += offset;                                  \
                                                        \
    u8 * end = * ((u8 **) _mem - 1);                    \
    u8 * mem8 = (u8 *) (_mem + operand);                \
    if (mem8 + sizeof (DEST_TYPE) <= end)               \
    {                                                   \
        * (DEST_TYPE *) mem8 = value;                   \
        return nextOp ();                               \
    }                                                   \
    else d_outOfBounds;                                 \
}

#define d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#define d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE)

d_m3Store_f (f32, f32)
d_m3Store_f (f64, f64)

d_m3Store_i (i32, u8)
d_m3Store_i (i32, i16)
d_m3Store_i (i32, i32)

d_m3Store_i (i64, u8)
d_m3Store_i (i64, i16)
d_m3Store_i (i64, i32)
d_m3Store_i (i64, i64)

//---------------------------------------------------------------------------------------------------------------------
# if d_m3EnableOptimizations
//---------------------------------------------------------------------------------------------------------------------

    #define d_m3BinaryOpWith1_i(TYPE, NAME, OPERATION)  \
    d_m3Op(TYPE##_##NAME)                               \
    {                                                   \
        _r0 = _r0 OPERATION 1;                          \
        return nextOp ();                               \
    }

    d_m3BinaryOpWith1_i (u64, Increment,    +)
    d_m3BinaryOpWith1_i (u32, Decrement,    -)

    d_m3BinaryOpWith1_i (u32, ShiftLeft1,   <<)
    d_m3BinaryOpWith1_i (u64, ShiftLeft1,   <<)

    d_m3BinaryOpWith1_i (u32, ShiftRight1,  >>)
    d_m3BinaryOpWith1_i (u64, ShiftRight1,  >>)

//---------------------------------------------------------------------------------------------------------------------
# endif


#endif /* m3_exec_h */
