#ifndef SINCITY_OBJECT_H
#define SINCITY_OBJECT_H

#include "sc_config.h"
#include "sincity/sc_debug.h"

#define SCObjSafeRelease(pObject)	(pObject) = NULL
#define SCObjSafeFree				SCObjSafeRelease

class SCObj
{
public:
    SCObj():m_nRefCount(0) {}
    SCObj(const SCObj &):m_nRefCount(0) {}
    virtual ~SCObj() {}

public:
    virtual SC_INLINE const char* getObjectId() = 0;
#if !defined(SWIG)
    SC_INLINE int getRefCount() const {
        return m_nRefCount;
    }
    void operator=(const SCObj &) {}
#endif


public:
    SC_INLINE int takeRef() { /*const*/
        sc_atomic_inc(&m_nRefCount);
        return m_nRefCount;
    }
    SC_INLINE int releaseRef() { /*const*/
        if (m_nRefCount) { // must never be equal to zero
            sc_atomic_dec(&m_nRefCount);
        }
        return m_nRefCount;
    }

private:
    volatile long m_nRefCount;
};


//
//	SCObjWrapper declaration
//
template<class SCObjType>
class SCObjWrapper
{

public:
    SC_INLINE SCObjWrapper(SCObjType obj = NULL);
    SC_INLINE SCObjWrapper(const SCObjWrapper<SCObjType> &obj);
    SC_INLINE virtual ~SCObjWrapper();

#if !defined(SWIG)
public:
    SC_INLINE SCObjWrapper<SCObjType>& operator=(const SCObjType other);
    SC_INLINE SCObjWrapper<SCObjType>& operator=(const SCObjWrapper<SCObjType> &other);
    SC_INLINE bool operator ==(const SCObjWrapper<SCObjType> other) const;
    SC_INLINE bool operator!=(const SCObjWrapper<SCObjType> &other) const;
    SC_INLINE bool operator <(const SCObjWrapper<SCObjType> other) const;
    SC_INLINE SCObjType operator->() const;
    SC_INLINE SCObjType operator*() const;
    SC_INLINE operator bool() const;
#endif

protected:
    SC_INLINE int takeRef();
    SC_INLINE int releaseRef();

    SC_INLINE SCObjType getWrappedObject() const;
    SC_INLINE void wrapObject(SCObjType obj);

private:
    SCObjType m_WrappedObject;
};

//
//	SCObjWrapper implementation
//
template<class SCObjType>
SCObjWrapper<SCObjType>::SCObjWrapper(SCObjType obj)
{
    wrapObject(obj), takeRef();
}

template<class SCObjType>
SCObjWrapper<SCObjType>::SCObjWrapper(const SCObjWrapper<SCObjType> &obj)
{
    wrapObject(obj.getWrappedObject()),
               takeRef();
}

template<class SCObjType>
SCObjWrapper<SCObjType>::~SCObjWrapper()
{
    releaseRef(),
               wrapObject(NULL);
}


template<class SCObjType>
int SCObjWrapper<SCObjType>::takeRef()
{
    if (m_WrappedObject /*&& m_WrappedObject->getRefCount() At startup*/) {
        return m_WrappedObject->takeRef();
    }
    return 0;
}

template<class SCObjType>
int SCObjWrapper<SCObjType>::releaseRef()
{
    if (m_WrappedObject && m_WrappedObject->getRefCount()) {
        if (m_WrappedObject->releaseRef() == 0) {
            delete m_WrappedObject, m_WrappedObject = NULL;
        }
        else {
            return m_WrappedObject->getRefCount();
        }
    }
    return 0;
}

template<class SCObjType>
SCObjType SCObjWrapper<SCObjType>::getWrappedObject() const
{
    return m_WrappedObject;
}

template<class SCObjType>
void SCObjWrapper<SCObjType>::wrapObject(const SCObjType obj)
{
    if(obj) {
        if(!(m_WrappedObject = dynamic_cast<SCObjType>(obj))) {
            SC_DEBUG_ERROR("Trying to wrap an object with an invalid type");
        }
    }
    else {
        m_WrappedObject = NULL;
    }
}

template<class SCObjType>
SCObjWrapper<SCObjType>& SCObjWrapper<SCObjType>::operator=(const SCObjType obj)
{
    releaseRef();
    wrapObject(obj), takeRef();
    return *this;
}

template<class SCObjType>
SCObjWrapper<SCObjType>& SCObjWrapper<SCObjType>::operator=(const SCObjWrapper<SCObjType> &obj)
{
    releaseRef();
    wrapObject(obj.getWrappedObject()), takeRef();
    return *this;
}

template<class SCObjType>
bool SCObjWrapper<SCObjType>::operator ==(const SCObjWrapper<SCObjType> other) const
{
    return getWrappedObject() == other.getWrappedObject();
}

template<class SCObjType>
bool SCObjWrapper<SCObjType>::operator!=(const SCObjWrapper<SCObjType> &other) const
{
    return getWrappedObject() != other.getWrappedObject();
}

template<class SCObjType>
bool SCObjWrapper<SCObjType>::operator <(const SCObjWrapper<SCObjType> other) const
{
    return getWrappedObject() < other.getWrappedObject();
}

template<class SCObjType>
SCObjWrapper<SCObjType>::operator bool() const
{
    return (getWrappedObject() != NULL);
}

template<class SCObjType>
SCObjType SCObjWrapper<SCObjType>::operator->() const
{
    return getWrappedObject();
}

template<class SCObjType>
SCObjType SCObjWrapper<SCObjType>::operator*() const
{
    return getWrappedObject();
}

#endif /* SINCITY_OBJECT_H */
