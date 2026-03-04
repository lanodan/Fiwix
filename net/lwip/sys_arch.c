#include "fiwix/types.h"
#include <lwip/sys.h>
#include <fiwix/sleep.h>
#include <fiwix/mm.h>
#include <fiwix/string.h>
#include <fiwix/timer.h>
#include <fiwix/asm.h>
#include <fiwix/sched.h>
#include <fiwix/kernel.h>

#define MS_TO_TICKS(ms) ((ms) * 1000 / TICK)

/* SEMAPHORES */

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	memset_b(&sem, 0, sizeof(struct resource));
	sem->sem.locked = count;
	sem->sem.wanted = 0;
	sem->valid = 1;
	return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem)
{
	/* Optimization: don't disable interrupts, etc if there is nothing to do. */
	if (!sem->sem.locked) {
		return;
	}
	unlock_resource(&sem->sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	printk("[%s: timeout = %d (ticks = %d)\n", __FUNCTION__, timeout, MS_TO_TICKS(timeout));
	return lock_resource_timeout(&sem->sem, MS_TO_TICKS(timeout)) ? SYS_ARCH_TIMEOUT : 0;
}

void sys_sem_free(sys_sem_t *sem)
{
	memset_b(&sem, 0, sizeof(struct resource));
}

int sys_sem_valid(sys_sem_t *sem)
{
	return sem->valid;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->valid = 0;
}

/* MAILBOXES */

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	memset_b(mbox, 0, sizeof(struct mailbox));
	mbox->max_length = size;
	mbox->empty.locked = 1;
	mbox->empty.wanted = 0;
	mbox->valid = 1;
	return ERR_OK;
}

static void mbox_dopost(sys_mbox_t *mbox, void *msg, int acquire_lock);
static void *mbox_dofetch(sys_mbox_t *mbox, int acquire_lock);

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	lock_resource(&mbox->intention_lock);
	if(mbox->intended_length < mbox->max_length) {
		mbox->intended_length++;
	}
	unlock_resource(&mbox->intention_lock);

	/* Ensure the mailbox is not full, and will not become full */
	lock_resource(&mbox->full);

	mbox_dopost(mbox, msg, 1);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	int full = 0;
	lock_resource(&mbox->intention_lock);
	if(mbox->intended_length >= mbox->max_length) {
		full = 1;
	} else {
		/*
		 * If we get here, it is guaranteed no one is waiting for the mailbox
		 * to become full, so this will not sleep
		 */
		lock_resource(&mbox->full);
		mbox->intended_length++;
	}
	unlock_resource(&mbox->intention_lock);
	if(full) {
		return ERR_MEM;
	}

	mbox_dopost(mbox, msg, 1);
	return ERR_OK;
}

/* XXX may not be needed in Fiwix */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
	if(mbox->length >= mbox->max_length) {
		return ERR_MEM;
	}

	mbox_dopost(mbox, msg, 0);
	return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	lock_resource(&mbox->intention_lock);
	if(mbox->intended_length > 0) {
		mbox->intended_length--;
	}
	unlock_resource(&mbox->intention_lock);

	/* timeout is in ms, TICK is in us */
	if(lock_resource_timeout(&mbox->empty, MS_TO_TICKS(timeout))) {
		return SYS_ARCH_TIMEOUT;
	}

	*msg = mbox_dofetch(mbox, 1);
	return 0;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	int empty = 0;
	lock_resource(&mbox->intention_lock);
	if(mbox->intended_length <= 0) {
		empty = 1;
	} else {
		/* Guaranteed the mailbox is non-empty, so will not sleep */
		lock_resource(&mbox->empty);
		mbox->intended_length--;
	}
	unlock_resource(&mbox->intention_lock);
	if(empty) {
		return SYS_MBOX_EMPTY;
	}

	mbox_dofetch(mbox, 1);
	return 0;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	if(mbox->length != 0) {
		printf("ERROR: lwIP mailboxes: mailbox still had mail in it\n");
	}
	memset_b(mbox, 0, sizeof(struct mailbox));
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return mbox->valid;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->valid = 0;
}

static void mbox_dopost(sys_mbox_t *mbox, void *msg, int acquire_lock)
{
	struct mail *entry = (struct mail *)kmalloc(sizeof(struct mail));
	entry->msg = msg;
	entry->next = NULL;

	if(acquire_lock) {
		lock_resource(&mbox->lock);
	}
	if(!mbox->tail) {
		mbox->head = entry;
		mbox->tail = entry;
	} else {
		mbox->tail->next = entry;
		mbox->tail = entry;
	}
	mbox->length++;

	if(mbox->length < mbox->max_length) {
		unlock_resource(&mbox->full);
	}
	if(mbox->empty.locked) {
		unlock_resource(&mbox->empty);
	}
	if(acquire_lock) {
		unlock_resource(&mbox->lock);
	}
}

static void *mbox_dofetch(sys_mbox_t *mbox, int acquire_lock)
{
	/* Here we know it is non-empty */
	if(acquire_lock) {
		lock_resource(&mbox->lock);
	}
	void *msg = mbox->head->msg;
	struct mail *old_head = mbox->head;
	mbox->head = mbox->head->next;
	mbox->length--;

	if(mbox->full.locked) {
		unlock_resource(&mbox->full);
	}
	if(mbox->length > 0) {
		unlock_resource(&mbox->empty);
	}

	if(acquire_lock) {
		unlock_resource(&mbox->lock);
	}

	kfree((unsigned int)old_head);
	return msg;
}

/* MISC */

void sys_msleep(u32_t ms)
{
	unsigned int flags;
	/* see kernel/syscalls/nanosleep.c */
	SAVE_FLAGS(flags); CLI();
	current->timeout = MS_TO_TICKS(ms);
	sleep(&sys_msleep, PROC_UNINTERRUPTIBLE);
	RESTORE_FLAGS(flags);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg,
                    int stacksize, int prio) {
	/* This cast is fine, a int is the same size as a void * */
	return kernel_process_arg(name, (int (*)(void *))thread, arg);
}

void sys_init(void) {
	/* No work */
}

void *my_calloc(__size_t n, __size_t size)
{
	void *mem = (void *)kmalloc(n * size);
	memset_b(mem, 0, size * n);
	return mem;
}

/* TIME */

u32_t sys_now(void)
{
	/* Based on gettimeofday syscall */
	return ((CURRENT_TICKS % HZ) * 1000) / HZ;
}

/* CRITICAL SECTIONS */

struct resource protect_lk;

int sys_arch_protect(void)
{
	lock_resource(&protect_lk);
	return 0;
}

void sys_arch_unprotect(sys_prot_t lev)
{
	unlock_resource(&protect_lk);
}
