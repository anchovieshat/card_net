#ifndef COMMON_H
#define COMMON_H

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long i64;
typedef int i32;
typedef short i16;
typedef char i8;
typedef float f32;
typedef double f64;

#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, __VA_ARGS__);
#else
#define debug(fmt, ...)
#endif

#define get_lock(lock) debug("%s:%s:%d aquiring lock: %s\n", __FILE__, __func__, __LINE__, #lock); pthread_mutex_lock((lock)); debug("%s:%s:%d aquired lock: %s\n", __FILE__, __func__, __LINE__, #lock);
#define release_lock(lock) debug("%s:%s:%d releasing lock: %s\n", __FILE__, __func__, __LINE__, #lock); pthread_mutex_unlock((lock)); debug("%s:%s:%d released lock: %s\n", __FILE__, __func__, __LINE__, #lock);
#define wait_for_lock(cond, lock) debug("%s:%s:%d waiting for signal: %s, %s\n", __FILE__, __func__, __LINE__, #cond, #lock); pthread_cond_wait((cond), (lock)); debug("%s:%s:%d got signal: %s, %s\n", __FILE__, __func__, __LINE__, #cond, #lock);
#define signal_to_lock(cond) debug("%s:%s:%d signaling: %s\n", __FILE__, __func__, __LINE__, #cond); pthread_cond_signal((cond)); debug("%s:%s:%d sent signal: %s\n", __FILE__, __func__, __LINE__, #cond);

#endif
