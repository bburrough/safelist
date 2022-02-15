/***************************************************************************
                          safelist.h  -  description
                             -------------------
    begin                : Sun Jul 6 2003
    copyright            : (C) 2003 by Bob Burrough
    email                : xxx
 ***************************************************************************/


#ifndef SAFELIST_H
#define SAFELIST_H

#include "threadutils.h"
#include <list>
#include <functional>

using namespace std;

/**
  *@author Bob Burrough
  */

template <class T>
class SafeList : protected list<T>
{
public:

    SafeList();
    virtual ~SafeList();


    bool empty() const;

    T pop_front();
    void push_back(const T& arg);
    size_t size() const;
    void remove(const T& arg);

    /*
        visit_all

        Iterate over the list in a thread-safe manner. Your provided
        visitor is called, passing the item as the argument. Your visitor
        returns true to continue iterating, or returns false to terminate
        iteration before reaching the end of the list.
    */
    void visit_all(std::function<bool(const T& item)> visitor)
    {
        Lock();
        for (typename list<T>::iterator itr = list<T>::begin(); itr != list<T>::end(); ++itr)
        {
            if (!visitor(*itr))
                break;
        }
        Unlock();
    }

private:
    bool MutexInit();

    /* These should be private. All manipulation should happen through thread safe interfaces. */
    bool Lock() const;
    bool Unlock() const;

    /*
        I made these private to help enforce the usage
        of:
            push_back(T);
            pop_front(T);

        instead of:

            list.Lock();
            list.insert(list.begin(),T);
            list.Unlock();
    */
    typename list<T>::iterator begin()
	{
		typename list<T>::iterator my_itr;
		Lock();
#ifdef SAFELIST_DEBUG
		cout << "SafeList<T>::begin()" << endl;
#endif
		my_itr = list<T>::begin();
		Unlock();
		return my_itr;
	} 


    typename list<T>::iterator end()
	{
		typename list<T>::iterator my_itr;
		Lock();
#ifdef SAFELIST_DEBUG
		cout << "SafeList<T>::end()" << endl;
#endif
		my_itr = list<T>::end();
		Unlock();
		return my_itr;
	}
    /*
        Thinking about the design of SafeList iteration. ...it would be irresponsible
        to simply allow begin() and end() to be used without enforcing some iteration
        lock. These functions are so common in regular STL usage that it would be
        extremely easy to have one thread adding or removing elements to the list
        while another thread is iterating the list. We must be able to guarantee
        that iteration is protected across threads.
       
        However, I'm also worried about simply allowing begin() and end() to be used
        and relying upon the caller to make sure they properly acquired the lock. We
        need enforcement via the interface somehow.

        What we need, functionally:

        list.Lock();
        for(list::iterator itr = list.begin(); itr != list.end(); ++itr)
        {
            // do something with *itr
        }
        list.Unlock();

        However, we don't want this particular interface layout, primarily because
        of the commonality of usage of begin() and end(). It would be easy for a
        programmer to accidentally do:

        list.insert(list.begin(), item);

        Which is inadvisable because the list can change between the calls to
        list.begin() and list.insert().

        We could do it at runtime by calling TryLock() (or whatever) inside begin()
        to verify that the lock had previously been acquired and throwing an exeception
        if it hadn't been, but that's sloppy runtime enforcement. We'd be better served
        by a more well defined interface that enforces the proper arrangement at compile
        time.

        We could write a function that takes a lambda/functor and iterates on every item
        in the list. I'm trying this. see above:

        void visit_all(std::function<bool>(const T& item) visitor);
    */


    bool insert(const typename list<T>::const_iterator& itr_arg, const T& t_arg);


    typename list<T>::iterator erase(const typename list<T>::const_iterator& itr_arg)
	{
		typename list<T>::iterator my_itr;
		Lock();
#ifdef SAFELIST_DEBUG
		cout << "SafeList<T>::erase(typename list<T>::iterator itr_arg)" << endl;
#endif
		my_itr = list<T>::erase( itr_arg );

#ifdef SAFELIST_DEBUG
		cout << "\tsize() after erase = " << list<T>::size() << endl;
#endif
		Unlock();

		return my_itr;
	}

    pthread_mutex_t safelist_mutex;
};

template<class T>
SafeList<T>::SafeList()
{
      #ifdef SAFELIST_DEBUG
         cout << "SafeList<T>::SafeList()" << endl;
#endif
    MutexInit();
}

template<class T>
SafeList<T>::~SafeList()
{
#ifdef SAFELIST_DEBUG
         cout << "SafeList<T>::~SafeList()" << endl;
#endif
    pthread_mutex_destroy(&safelist_mutex);
}

template<class T>
bool SafeList<T>::insert(const typename list<T>::const_iterator& itr_arg, const T& t_arg)
{
    Lock();
   #ifdef SAFELIST_DEBUG
        cout << "SafeList<T>::insert(typename list<T>::iterator itr_arg)" << endl;
        #endif
    list<T>::insert( itr_arg, t_arg );

    #ifdef SAFELIST_DEBUG
        cout << "\tsize() after insert = " << list<T>::size() << endl;
    #endif
    Unlock();
    return true;
}


template<class T>
bool SafeList<T>::MutexInit()
{
#ifdef SAFELIST_DEBUG
    cout << "SafeList<T>::MutexInit()" << endl;
#endif
    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &safelist_mutex, &attr );
    pthread_mutexattr_destroy(&attr);
    return true;
}

template<class T>
bool SafeList<T>::Lock() const
{
#ifdef SAFELIST_DEBUG
         cout << "SafeList<T>::Lock()" << endl;
#endif

    if ( pthread_mutex_lock( const_cast<pthread_mutex_t*>(&safelist_mutex) ) != 0 )
        return false;
    else
        return true;
}

template<class T>
bool SafeList<T>::Unlock() const
{
    #ifdef SAFELIST_DEBUG
        cout << "SafeList<T>::Unlock()" << endl;
     #endif
    if ( pthread_mutex_unlock( const_cast<pthread_mutex_t*>(&safelist_mutex) ) != 0 )
        return false;
    else
        return true;
}

template<class T>
bool SafeList<T>::empty() const
{
    return list<T>::empty();
}

template<class T>
T SafeList<T>::pop_front()
{
    typename list<T>::iterator itr;
    T ret_val = NULL;
    Lock();
    if(!this->empty())
    {
        itr = begin();
        ret_val = *itr;
        list<T>::erase(itr);
    }
    Unlock();
    return ret_val;
}

template<class T>
void SafeList<T>::push_back(const T& arg)
{
    Lock();
    list<T>::push_back(arg);
    Unlock();
}

template<class T>
size_t SafeList<T>::size() const
{
    size_t size = 0;
    Lock();
    size = list<T>::size();
    Unlock();
    return size;
}

template<class T>
void SafeList<T>::remove(const T& t)
{
    Lock();
    list<T>::remove(t);
    Unlock();
}

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2003-2019 Bobby G. Burrough
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
