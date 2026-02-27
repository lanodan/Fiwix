/* Applies to i386. */

#include <fiwix/types.h>
#include <fiwix/stdio.h>
#include <fiwix/kernel.h>
#include <fiwix/rand.h>

#include <fiwix/errno.h> /* Instead of LWIP_PROVIDE_ERRNO */

/* stdio.h interface as expected by lwIP */
#define printf printk
#define sprintf sprintk
#define snprintf(BUF, LEN, ...) sprintk(BUF, __VA_ARGS__)

#define BYTE_ORDER LITTLE_ENDIAN

#define LWIP_RAND() rand()

#define LWIP_NO_STDDEF_H 1
typedef __size_t size_t;

/* no stdint.h */
#define LWIP_NO_STDINT_H 1
#define LWIP_HAVE_INT64 1
typedef __u8 u8_t;
typedef __s8 s8_t;
typedef __u16 u16_t;
typedef __s16 s16_t;
typedef __u32 u32_t;
typedef __s32 s32_t;
typedef __u64 u64_t;
typedef __s64 s64_t;
typedef unsigned int mem_ptr_t;

/* or inttypes.h */
#define LWIP_NO_INTTYPES_H 1
#define X8_F "02x"
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "u"

/* or limits.h */
#define LWIP_NO_LIMITS_H 1
#define INT_MAX 2147483647

/* ctypes.h does exist, but not in the format lwIP expects */
#define LWIP_NO_CTYPE_H 1

/* the warnings are ok for now */
#define LWIP_CONST_CAST(target_type, val) val

/* same format as default */
#define LWIP_PLATFORM_ASSERT(x) PANIC("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__)
