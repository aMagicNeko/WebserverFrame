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
        pthread_mutex_init(&mutex_, 0);
    }

    ~Spinlock() {
        pthread_mutex_destroy(&mutex_);
    }

    void lock() {
        pthread_mutex_lock(&mutex_);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }
private:
    pthread_mutex_t mutex_; //I have no spinlock in my system.
};

}
#endif