/**
 * @file newlib_stubs.c
 * @brief Newlib lock function stubs for ARM GCC 14.3.0
 *
 * Arduino's ARM GCC 14.3.0 toolchain uses a newer newlib that requires
 * retarget lock functions. This file provides stub implementations to
 * resolve linker errors.
 */

// Define __lock as a simple integer type for single-threaded operation
typedef int __lock_t;
typedef __lock_t _LOCK_T;

// Define lock structures as simple integers
__lock_t __lock___malloc_recursive_mutex;
__lock_t __lock___env_recursive_mutex;
__lock_t __lock___sfp_recursive_mutex;
__lock_t __lock___atexit_recursive_mutex;
__lock_t __lock___at_quick_exit_mutex;
__lock_t __lock___tz_mutex;
__lock_t __lock___dd_hash_mutex;
__lock_t __lock___arc4random_mutex;

// Define the structure that newlib expects
struct __lock {
    int dummy;
};

// Create dummy lock instances
static struct __lock malloc_lock;
static struct __lock env_lock;

/**
 * Acquire a recursive lock
 * Note: These are stubs for single-threaded operation on Pico
 */
void __retarget_lock_acquire_recursive(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}

/**
 * Try to acquire a recursive lock
 */
int __retarget_lock_try_acquire_recursive(struct __lock *lock) {
    // Always succeed in single-threaded environment
    (void)lock;
    return 1;
}

/**
 * Release a recursive lock
 */
void __retarget_lock_release_recursive(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}

/**
 * Acquire a regular lock
 */
void __retarget_lock_acquire(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}

/**
 * Try to acquire a regular lock
 */
int __retarget_lock_try_acquire(struct __lock *lock) {
    // Always succeed in single-threaded environment
    (void)lock;
    return 1;
}

/**
 * Release a regular lock
 */
void __retarget_lock_release(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}

/**
 * Close/destroy a lock
 */
void __retarget_lock_close(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}

/**
 * Close/destroy a recursive lock
 */
void __retarget_lock_close_recursive(struct __lock *lock) {
    // No-op for single-threaded environment
    (void)lock;
}
