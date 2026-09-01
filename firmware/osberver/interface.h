#include <iostream>

/***************************************************
* CPP Classes
***************************************************/
class IObserver {
    public:
        virtual void Update(const char &message_from_subject) =0;
        virtual ~IObserver(){};
};

/* Interface setup abstract methods tbd into the .c main*/
class ISubject {
    public:
        virtual void Attach(IObserver *observer) =0;
        virtual void Detach(IObserver *observer) =0;
        virtual void Notify() =0;
        virtual ~ISubject(){};
};

/***************************************************
* TYPE DEFINITIONS
***************************************************/

/* List to store the subcribers for the subject */
typedef struct list {
    unsigned int data; /* TBD : Changed depends on type needed */
    struct lsit *next;
};