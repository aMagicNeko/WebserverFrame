#ifndef __MUTEX_H__
#define __MUTEX_H__
#include <pthread.h>
#include "noncopyable.h"
namespace ekko{
template<class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T& mutex)
        :mutex_(mutex) {
        mutex_.lock();
        m_locked = true;
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            mutex_.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            mutex_.unlock();
            m_locked = false;
        }
    }
private:
    T& mutex_;
    bool m_locked;
};

class Spinlock : Noncopyable {
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&mutex_, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&mutex_);
    }

    void lock() {
        pthread_spin_lock(&mutex_);
    }

    void unlock() {
        pthread_spin_unlock(&mutex_);
    }
private:
    pthread_spinlock_t mutex_;
};

}
#endif