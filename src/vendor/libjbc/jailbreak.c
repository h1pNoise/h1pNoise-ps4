#include "defs.h"
#include "kernelrw.h"
#include "jailbreak.h"

#ifndef HARBOR_LIBJBC_TEST
asm("jbc_raw_open:\nmov $5, %rax\nmov %rcx, %r10\nsyscall\njnc 1f\nneg %rax\n1: ret");
asm("jbc_raw_close:\nmov $6, %rax\nmov %rcx, %r10\nsyscall\nret");
#endif

int jbc_raw_open(const char*, int);
int jbc_raw_close(int);
extern pid_t getpid(void);
static int heap_ptr(uintptr_t value) { return value >= 0xffff800000000000ULL && value < 0xffffffff00000000ULL; }
/* prison0 can be a static object in kernel data, covered by libjbc's
   KERNEL_TEXT range. Process/fd/vnode objects still require heap pointers. */
static int prison_ptr(uintptr_t value) { return heap_ptr(value) || (value >= 0xffffffff00000000ULL && value < 0xfffffffffffff000ULL); }

static uintptr_t prison0;
static uintptr_t rootvnode;
static int resolve_error;
int jbc_resolve_error(void) { return resolve_error; }
static int resolve_fail(int reason) { prison0 = rootvnode = 0; resolve_error = reason; return -1; }

static int resolve(void)
{
    unsigned retries = 0;
    resolve_error = 0;
restart:;
    if (++retries > 8) return resolve_fail(1);
    uintptr_t td = jbc_krw_get_td();
    if(!heap_ptr(td)) return resolve_fail(2);
    uintptr_t proc = jbc_krw_read64(td+8, KERNEL_HEAP);
    for(unsigned steps = 0;;steps++)
    {
        if(steps >= 4096) return resolve_fail(3);
        if(!heap_ptr(proc)) return resolve_fail(4);
        int pid;
        if(jbc_krw_memcpy((uintptr_t)&pid, proc+0xb0, sizeof(pid), KERNEL_HEAP))
            goto restart;
        if(pid == 1)
            break;
        uintptr_t proc2 = jbc_krw_read64(proc, KERNEL_HEAP);
        if(!heap_ptr(proc2)) return resolve_fail(5);
        uintptr_t proc1 = jbc_krw_read64(proc2+8, KERNEL_HEAP);
        if(proc1 != proc)
            goto restart;
        proc = proc2;
    }
    uintptr_t pid1_ucred = jbc_krw_read64(proc+0x40, KERNEL_HEAP);
    uintptr_t pid1_fd = jbc_krw_read64(proc+0x48, KERNEL_HEAP);
    if(!heap_ptr(pid1_ucred) || !heap_ptr(pid1_fd)) return resolve_fail(6);
    if(jbc_krw_memcpy((uintptr_t)&prison0, pid1_ucred+0x30, sizeof(prison0), KERNEL_HEAP))
        return resolve_fail(7);
    if(jbc_krw_memcpy((uintptr_t)&rootvnode, pid1_fd+0x18, sizeof(rootvnode), KERNEL_HEAP))
    {
        return resolve_fail(8);
    }
    if(!prison_ptr(prison0)) return resolve_fail(9);
    if(!heap_ptr(rootvnode)) return resolve_fail(10);
    return 0;
}

uintptr_t jbc_get_prison0(void)
{
    if(!prison0)
        resolve();
    return prison0;
}

uintptr_t jbc_get_rootvnode(void)
{
    if(!rootvnode)
        resolve();
    return rootvnode;
}

static inline int ppcopyout(void* u1, void* u2, uintptr_t k)
{
    return jbc_krw_memcpy((uintptr_t)u1, k, (uintptr_t)u2-(uintptr_t)u1, KERNEL_HEAP);
}

static inline int ppcopyin(const void* u1, const void* u2, uintptr_t k)
{
    return jbc_krw_memcpy(k, (uintptr_t)u1, (uintptr_t)u2-(uintptr_t)u1, KERNEL_HEAP);
}

int jbc_get_cred(struct jbc_cred* ans)
{
    if(!jbc_krw_available()) return -1;
    uintptr_t td = jbc_krw_get_td();
    if(!heap_ptr(td)) return -1;
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    pid_t pid = 0;
    if(!heap_ptr(proc) || jbc_krw_memcpy((uintptr_t)&pid,proc+0xb0,sizeof(pid),KERNEL_HEAP) || pid != getpid()) return -1;
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);
    if(!heap_ptr(ucred) || !heap_ptr(fd)) return -1;

    if (ppcopyout(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyout(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyout(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyout(&ans->cdir, 1 + &ans->jdir, fd + 0x10)
        || ppcopyout(&ans->sceProcType, 1 + &ans->sceProcCap, ucred + 88))
        return -1;

    return 0;
}

static int jbc_set_cred_internal(const struct jbc_cred* ans)
{
    uintptr_t td = jbc_krw_get_td();
    if(!heap_ptr(td)) return -1;
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    pid_t pid = 0;
    if(!heap_ptr(proc) || jbc_krw_memcpy((uintptr_t)&pid,proc+0xb0,sizeof(pid),KERNEL_HEAP) || pid != getpid()) return -1;
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);
    if(!heap_ptr(ucred) || !heap_ptr(fd) || !prison_ptr(ans->prison) ||
       (ans->cdir && !heap_ptr(ans->cdir)) || (ans->rdir && !heap_ptr(ans->rdir)) ||
       (ans->jdir && !heap_ptr(ans->jdir))) return -1;

    if (ppcopyin(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyin(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyin(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyin(&ans->cdir, 1 + &ans->jdir, fd + 0x10) 
        || ppcopyin(&ans->sceProcType, 1 + &ans->sceProcCap, ucred + 88))
        return -1;
    return 0;
}

int jbc_set_auth(const struct jbc_cred* ans)
{
    if(!jbc_krw_available()) return -1;
    uintptr_t td = jbc_krw_get_td();
    if(!heap_ptr(td)) return -1;
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    pid_t pid = 0;
    if(!heap_ptr(proc) || jbc_krw_memcpy((uintptr_t)&pid,proc+0xb0,sizeof(pid),KERNEL_HEAP) || pid != getpid()) return -1;
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    if(!heap_ptr(ucred)) return -1;
    return ppcopyin(&ans->sceProcType,1 + &ans->sceProcCap,ucred + 88);
}

#ifdef HARBOR_LIBJBC_TEST
int jbc_test_set_cred_internal(const struct jbc_cred* cred) { return jbc_set_cred_internal(cred); }
#endif

int jbc_jailbreak_cred(struct jbc_cred* ans)
{
    uintptr_t prison0 = jbc_get_prison0();
    if (!prison0)
        return -1;
    uintptr_t rootvnode = jbc_get_rootvnode();
    if (!rootvnode)
        return -1;

    //without some modules wont load like Apputils
    ans->sceProcCap = 0xffffffffffffffff;
    ans->sceProcType = 0x3801000000000013;
    ans->sonyCred = 0xffffffffffffffff;

    ans->uid = ans->ruid = ans->svuid = ans->rgid = ans->svgid = 0;
    ans->prison = prison0;
    ans->cdir = ans->rdir = ans->jdir = rootvnode;
    return 0;
}

static int jbc_open_this(const struct jbc_cred* cred0, uintptr_t vnode)
{
    if(!vnode)
        return -1;
    struct jbc_cred cred = *cred0;
    if(jbc_jailbreak_cred(&cred)) return -1;
    cred.cdir = cred.rdir = cred.jdir = vnode;
    if(jbc_set_cred_internal(&cred)) {
        jbc_set_cred_internal(cred0);
        return -1;
    }
    int ans = jbc_raw_open("/", 0);
    if(jbc_set_cred_internal(cred0)) {
        if(ans >= 0) jbc_raw_close(ans);
        return -1;
    }
    return ans;
}

static int swap64(uintptr_t p1, KmemKind k1, uintptr_t p2, KmemKind k2)
{
    uintptr_t v1 = jbc_krw_read64(p1, k1);
    uintptr_t v2 = jbc_krw_read64(p2, k2);
    if(ppcopyin(&v2, 1+&v2, p1)
    || ppcopyin(&v1, 1+&v1, p2))
        return -1;
    return 0;
}

static int return0(void)
{
    return 0;
}

static void* fake_vtable[16] = {};

int jbc_set_cred(const struct jbc_cred* newp)
{
    struct jbc_cred old, neww = *newp;
    if(jbc_get_cred(&old))
        return -1;
    if(old.cdir && !neww.cdir)
        neww.cdir = jbc_get_rootvnode();
    if(old.rdir && !neww.rdir)
        neww.rdir = jbc_get_rootvnode();
    if(old.jdir && !neww.jdir)
        neww.jdir = jbc_get_rootvnode();
    /* A credential-only operation must not temporarily remount its roots.
       Open replacement vnode descriptors only for directories that change. */
    int cdir_fd = neww.cdir != old.cdir ? jbc_open_this(&old, neww.cdir) : -1;
    int rdir_fd = neww.rdir != old.rdir ? jbc_open_this(&old, neww.rdir) : -1;
    int jdir_fd = neww.jdir != old.jdir ? jbc_open_this(&old, neww.jdir) : -1;
    if((neww.cdir != old.cdir && cdir_fd < 0) ||
       (neww.rdir != old.rdir && rdir_fd < 0) ||
       (neww.jdir != old.jdir && jdir_fd < 0)) {
        if(cdir_fd >= 0) jbc_raw_close(cdir_fd);
        if(rdir_fd >= 0) jbc_raw_close(rdir_fd);
        if(jdir_fd >= 0) jbc_raw_close(jdir_fd);
        return -1;
    }
    struct jbc_cred elevated = neww;
    elevated.cdir = old.cdir;
    elevated.jdir = old.jdir;
    elevated.rdir = old.rdir;
    //this offset is the same in 5.05-9.60, probably safe to use
    uint32_t rc;
    if(elevated.prison)
    {
        if(jbc_krw_memcpy((uintptr_t)&rc, elevated.prison+0x14, sizeof(rc), KERNEL_HEAP)
        && jbc_krw_memcpy((uintptr_t)&rc, elevated.prison+0x14, sizeof(rc), KERNEL_TEXT))
            return -1;
        rc++;
        if(jbc_krw_memcpy(elevated.prison+0x14, (uintptr_t)&rc, sizeof(rc), KERNEL_HEAP)
        && jbc_krw_memcpy(elevated.prison+0x14, (uintptr_t)&rc, sizeof(rc), KERNEL_TEXT))
            return -1;
    }
    if(jbc_set_cred_internal(&elevated))
        return -1;
    if(old.prison)
    {
        if(jbc_krw_memcpy((uintptr_t)&rc, old.prison+0x14, sizeof(rc), KERNEL_HEAP)
        && jbc_krw_memcpy((uintptr_t)&rc, old.prison+0x14, sizeof(rc), KERNEL_TEXT))
            return -1;
        rc--;
        if(jbc_krw_memcpy(old.prison+0x14, (uintptr_t)&rc, sizeof(rc), KERNEL_HEAP)
        && jbc_krw_memcpy(old.prison+0x14, (uintptr_t)&rc, sizeof(rc), KERNEL_TEXT))
            return -1;
    }
    if(cdir_fd < 0 && rdir_fd < 0 && jdir_fd < 0) return 0;
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);
    uintptr_t ofiles = jbc_krw_read64(fd, KERNEL_HEAP);
    if(!heap_ptr(ofiles)) return -1;
    int fds[3] = {cdir_fd, rdir_fd, jdir_fd};
    for(int i = 0; i < 3; i++)
    {
        if(fds[i] < 0)
            continue;
        uintptr_t file = jbc_krw_read64(ofiles+8*fds[i], KERNEL_HEAP);
        if(!heap_ptr(file)) return -1;
        if(swap64(file, KERNEL_HEAP, fd+0x10+8*i, KERNEL_HEAP))
            return -1;
        if(jbc_krw_read64(file, KERNEL_HEAP) == 0)
        {
            if(!fake_vtable[0])
                for(size_t i = 0; i < sizeof(fake_vtable) / sizeof(fake_vtable[0]); i++)
                    fake_vtable[i] = return0;
            uintptr_t p = (uintptr_t)fake_vtable;
            if(ppcopyin(&p, 1+&p, file+8))
                return -1;
        }
        jbc_raw_close(fds[i]);
    }
    return 0;
}
