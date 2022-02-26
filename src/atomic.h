# ifndef ATOMIC_H
# define ATOMIC_H
/* Memory barrier */
  #define barrier()             	(__sync_synchronize())
/* Atomic get */
  #define AO_GET(ptr)       		({ __typeof__(*(ptr)) volatile *_val = (ptr); barrier(); (*_val); })
/**
 * Atomic test and set when the value is different from memory.
 * 原子设置，如果原值和新值不一样则设置
 * */
  #define AO_SET(ptr, value)        ((void)__sync_lock_test_and_set((ptr), (value)))
/**
 * Atomic swap, if set succeed returns old value, otherwise returns the new one.
 * 原子交换，如果被设置，则返回旧值，否则返回设置值 
 * */
  #define AO_SWAP(ptr, value)       ((__typeof__(*(ptr)))__sync_lock_test_and_set((ptr), (value)))
/** 
 * Atomic compare and swap. 
 * 原子比较交换，如果当前值等于旧值，则新值被设置，返回旧值，否则返回新值*/
  #define AO_CAS(ptr, comp, value)  ((__typeof__(*(ptr)))__sync_val_compare_and_swap((ptr), (comp), (value)))
/**
 *  Atomic bool compare and swap
 * 原子比较交换，如果当前值等于旧指，则新值被设置，返回真值，否则返回假 
 * */
  #define AO_CASB(ptr, comp, value) (__sync_bool_compare_and_swap((ptr), (comp), (value)) != 0 ? true : false)
/* Atomic clear 原子清零 */
  #define AO_CLEAR(ptr)             ((void)__sync_lock_release((ptr)))
/**
 * Atomic arithmetic and bit ops, returns new value 
 * 通过值与旧值进行算术与位操作，返回旧值 
 */
  #define AO_ADD_F(ptr, value)      ((__typeof__(*(ptr)))__sync_add_and_fetch((ptr), (value)))
  #define AO_SUB_F(ptr, value)      ((__typeof__(*(ptr)))__sync_sub_and_fetch((ptr), (value)))
  #define AO_OR_F(ptr, value)       ((__typeof__(*(ptr)))__sync_or_and_fetch((ptr), (value)))
  #define AO_AND_F(ptr, value)      ((__typeof__(*(ptr)))__sync_and_and_fetch((ptr), (value)))
  #define AO_XOR_F(ptr, value)      ((__typeof__(*(ptr)))__sync_xor_and_fetch((ptr), (value)))
/**
 * Atomic arithmetic and bit ops, returns old value 
 * 通过值与旧值进行算术与位操作，返回旧值 
 * */
  #define AO_F_ADD(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_add((ptr), (value)))
  #define AO_F_SUB(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_sub((ptr), (value)))
  #define AO_F_OR(ptr, value)       ((__typeof__(*(ptr)))__sync_fetch_and_or((ptr), (value)))
  #define AO_F_AND(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_and((ptr), (value)))
  #define AO_F_XOR(ptr, value)      ((__typeof__(*(ptr)))__sync_fetch_and_xor((ptr), (value)))
/** 
 * Bit and arithmetic ops ignoring return values 
 * 忽略返回值的原子算术和位操作
 */
#define AO_INC(ptr)                 ((void)AO_ADD_F((ptr), 1))
#define AO_DEC(ptr)                 ((void)AO_SUB_F((ptr), 1))
#define AO_ADD(ptr, val)            ((void)AO_ADD_F((ptr), (val)))
#define AO_SUB(ptr, val)            ((void)AO_SUB_F((ptr), (val)))
#define AO_OR(ptr, val)			 ((void)AO_OR_F((ptr), (val)))
#define AO_AND(ptr, val)			((void)AO_AND_F((ptr), (val)))
#define AO_XOR(ptr, val)			((void)AO_XOR_F((ptr), (val)))
/* Atomic bit setting via mask, returns new value. 通过掩码，设置某个位为1，并返还新的值 */
#define AO_BIT_ON(ptr, mask)        AO_OR_F((ptr), (mask))
/* Atomic bit unsetting via mask, returns new value. 通过掩码，设置某个位为0，并返还新的值 */
#define AO_BIT_OFF(ptr, mask)       AO_AND_F((ptr), ~(mask))
/* Atomic bit flipping via mask, returns new value. 通过掩码，交换某个位，1变0，0变1，并返还新的值 */
#define AO_BIT_FLIP(ptr, mask)      AO_XOR_F((ptr), (mask))
#endif // ATOMIC_H