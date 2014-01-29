#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "selinux_internal.h"
#include "policy.h"

#define UNSET (const security_context_t) -1

static __thread pid_t cpid;
static __thread pid_t tid;
static __thread security_context_t prev_current = UNSET;
static __thread security_context_t prev_exec = UNSET;
static __thread security_context_t prev_fscreate = UNSET;
static __thread security_context_t prev_keycreate = UNSET;
static __thread security_context_t prev_sockcreate = UNSET;

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t destructor_key;
static int destructor_key_initialized = 0;
static __thread char destructor_initialized;

extern void *__dso_handle __attribute__ ((__weak__, __visibility__ ("hidden")));
extern int __register_atfork (void (*) (void), void (*) (void), void (*) (void), void *);

static int __selinux_atfork (void (*prepare) (void), void (*parent) (void), void (*child) (void))
{
  return __register_atfork (prepare, parent, child,
                            &__dso_handle == NULL ? NULL : __dso_handle);
}

static pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

static void procattr_thread_destructor(void __attribute__((unused)) *unused)
{
	if (prev_current != UNSET)
		free(prev_current);
	if (prev_exec != UNSET)
		free(prev_exec);
	if (prev_fscreate != UNSET)
		free(prev_fscreate);
	if (prev_keycreate != UNSET)
		free(prev_keycreate);
	if (prev_sockcreate != UNSET)
		free(prev_sockcreate);
}

static void free_procattr(void)
{
	procattr_thread_destructor(NULL);
	tid = 0;
	cpid = getpid();
	prev_current = prev_exec = prev_fscreate = prev_keycreate = prev_sockcreate = UNSET;
}

void __attribute__((destructor)) procattr_destructor(void);

void hidden __attribute__((destructor)) procattr_destructor(void)
{
	if (destructor_key_initialized)
		__selinux_key_delete(destructor_key);
}

static inline void init_thread_destructor(void)
{
	if (destructor_initialized == 0) {
		__selinux_setspecific(destructor_key, (void *)1);
		destructor_initialized = 1;
	}
}

static void init_procattr(void)
{
	if (__selinux_key_create(&destructor_key, procattr_thread_destructor) == 0) {
		__selinux_atfork(NULL, NULL, free_procattr);
		destructor_key_initialized = 1;
	}
}

static int openattr(pid_t pid, const char *attr, int flags)
{
	int fd, rc;
	char *path;

	if (cpid != getpid())
		free_procattr();

	if (pid > 0)
		rc = asprintf(&path, "/proc/%d/attr/%s", pid, attr);
	else {
		if (!tid)
			tid = gettid();
		rc = asprintf(&path, "/proc/self/task/%d/attr/%s", tid, attr);
	}
	if (rc < 0)
		return -1;

	fd = open(path, flags | O_CLOEXEC);
	free(path);
	return fd;
}

static int getprocattrcon_raw(security_context_t * context,
			      pid_t pid, const char *attr)
{
	char *buf;
	size_t size;
	int fd;
	ssize_t ret;
	int errno_hold;
	security_context_t prev_context;

	__selinux_once(once, init_procattr);
	init_thread_destructor();

	if (cpid != getpid())
		free_procattr();

	switch (attr[0]) {
		case 'c':
			prev_context = prev_current;
			break;
		case 'e':
			prev_context = prev_exec;
			break;
		case 'f':
			prev_context = prev_fscreate;
			break;
		case 'k':
			prev_context = prev_keycreate;
			break;
		case 's':
			prev_context = prev_sockcreate;
			break;
		case 'p':
			prev_context = NULL;
			break;
		default:
			errno = ENOENT;
			return -1;
	};

	if (prev_context && prev_context != UNSET) {
		*context = strdup(prev_context);
		if (!(*context)) {
			return -1;
		}
		return 0;
	}

	fd = openattr(pid, attr, O_RDONLY);
	if (fd < 0)
		return -1;

	size = selinux_page_size;
	buf = malloc(size);
	if (!buf) {
		ret = -1;
		goto out;
	}
	memset(buf, 0, size);

	do {
		ret = read(fd, buf, size - 1);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0)
		goto out2;

	if (ret == 0) {
		*context = NULL;
		goto out2;
	}

	*context = strdup(buf);
	if (!(*context)) {
		ret = -1;
		goto out2;
	}
	ret = 0;
      out2:
	free(buf);
      out:
	errno_hold = errno;
	close(fd);
	errno = errno_hold;
	return ret;
}

static int getprocattrcon(security_context_t * context,
			  pid_t pid, const char *attr)
{
	int ret;
	security_context_t rcontext;

	ret = getprocattrcon_raw(&rcontext, pid, attr);

	if (!ret) {
		ret = selinux_raw_to_trans_context(rcontext, context);
		freecon(rcontext);
	}

	return ret;
}

static int setprocattrcon_raw(security_context_t context,
			      pid_t pid, const char *attr)
{
	int fd;
	ssize_t ret;
	int errno_hold;
	security_context_t *prev_context;

	__selinux_once(once, init_procattr);
	init_thread_destructor();

	if (cpid != getpid())
		free_procattr();

	switch (attr[0]) {
		case 'c':
			prev_context = &prev_current;
			break;
		case 'e':
			prev_context = &prev_exec;
			break;
		case 'f':
			prev_context = &prev_fscreate;
			break;
		case 'k':
			prev_context = &prev_keycreate;
			break;
		case 's':
			prev_context = &prev_sockcreate;
			break;
		default:
			errno = ENOENT;
			return -1;
	};

	if (!context && !*prev_context)
		return 0;
	if (context && *prev_context && *prev_context != UNSET
	    && !strcmp(context, *prev_context))
		return 0;

	fd = openattr(pid, attr, O_RDWR);
	if (fd < 0)
		return -1;
	if (context) {
		ret = -1;
		context = strdup(context);
		if (!context)
			goto out;
		do {
			ret = write(fd, context, strlen(context) + 1);
		} while (ret < 0 && errno == EINTR);
	} else {
		do {
			ret = write(fd, NULL, 0);	/* clear */
		} while (ret < 0 && errno == EINTR);
	}
out:
	errno_hold = errno;
	close(fd);
	errno = errno_hold;
	if (ret < 0) {
		free(context);
		return -1;
	} else {
		if (*prev_context != UNSET)
			free(*prev_context);
		*prev_context = context;
		return 0;
	}
}

static int setprocattrcon(const security_context_t context,
			  pid_t pid, const char *attr)
{
	int ret;
	security_context_t rcontext;

	if (selinux_trans_to_raw_context(context, &rcontext))
		return -1;

	ret = setprocattrcon_raw(rcontext, pid, attr);

	freecon(rcontext);

	return ret;
}

#define getselfattr_def(fn, attr) \
	int get##fn##_raw(security_context_t *c) \
	{ \
		return getprocattrcon_raw(c, 0, #attr); \
	} \
	int get##fn(security_context_t *c) \
	{ \
		return getprocattrcon(c, 0, #attr); \
	}

#define setselfattr_def(fn, attr) \
	int set##fn##_raw(const security_context_t c) \
	{ \
		return setprocattrcon_raw(c, 0, #attr); \
	} \
	int set##fn(const security_context_t c) \
	{ \
		return setprocattrcon(c, 0, #attr); \
	}

#define all_selfattr_def(fn, attr) \
	getselfattr_def(fn, attr)	 \
	setselfattr_def(fn, attr)

#define getpidattr_def(fn, attr) \
	int get##fn##_raw(pid_t pid, security_context_t *c)	\
	{ \
		return getprocattrcon_raw(c, pid, #attr); \
	} \
	int get##fn(pid_t pid, security_context_t *c)	\
	{ \
		return getprocattrcon(c, pid, #attr); \
	}

all_selfattr_def(con, current)
    getpidattr_def(pidcon, current)
    getselfattr_def(prevcon, prev)
    all_selfattr_def(execcon, exec)
    all_selfattr_def(fscreatecon, fscreate)
    all_selfattr_def(sockcreatecon, sockcreate)
    all_selfattr_def(keycreatecon, keycreate)

    hidden_def(getcon_raw)
    hidden_def(getcon)
    hidden_def(getexeccon_raw)
    hidden_def(getfilecon_raw)
    hidden_def(getfilecon)
    hidden_def(getfscreatecon_raw)
    hidden_def(getkeycreatecon_raw)
    hidden_def(getpeercon_raw)
    hidden_def(getpidcon_raw)
    hidden_def(getprevcon_raw)
    hidden_def(getprevcon)
    hidden_def(getsockcreatecon_raw)
    hidden_def(setcon_raw)
    hidden_def(setexeccon_raw)
    hidden_def(setexeccon)
    hidden_def(setfilecon_raw)
    hidden_def(setfscreatecon_raw)
    hidden_def(setkeycreatecon_raw)
    hidden_def(setsockcreatecon_raw)
