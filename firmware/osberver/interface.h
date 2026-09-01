#ifndef SDV_OBSERVER_INTERFACE_H
#define SDV_OBSERVER_INTERFACE_H

#include <iostream>

/***************************************************
* CPP Classes
***************************************************/
class IObserver {
    public:
        virtual void Update(const char *message_from_subject) = 0;
        virtual ~IObserver(){};
};

/* Interface setup abstract methods tbd into the .c main*/
class ISubject {
    public:
        virtual void Attach(IObserver *observer) = 0;
        virtual void Detach(IObserver *observer) = 0;
        virtual void Notify() = 0;
        virtual ~ISubject(){};
};

/***************************************************
* TYPE DEFINITIONS
***************************************************/

/* List to store the subscribers for the subject (C-style, no STL) */
typedef struct list {
    IObserver   *observer;   /* was unsigned int data -> the subscriber pointer */
    struct list *next;
} list;

#endif /* SDV_OBSERVER_INTERFACE_H */
