#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "interface.h"
#include "actions.h"
#include "telemetry.h"

static const char *action_name(int a)
{
    switch (a) {
        case SDV_RESTART:       return "RESTART";
        case SDV_DEGRADED_MODE: return "DEGRADED_MODE";
        case SDV_LOAD_SHED:     return "LOAD_SHED";
        default:                return "NONE";
    }
}

/* Edge node (K1): observes the hub and reacts to a broadcast incident. */
class EdgeNode : public IObserver {
    public:
        EdgeNode(const char *name, int node_id) : name_(name), node_id_(node_id) {}
        void Update(const char *message_from_subject) override
        {
            std::cout << "      -> [id " << node_id_ << "] " << name_
                      << " : " << message_from_subject << "\n";
        }
    private:
        const char *name_;
        int         node_id_;
};

/* Zonal hub (K3): the Subject. Keeps subscribers in the C `list` and fans out. */
class ZonalHub : public ISubject {
    public:
        ZonalHub() : head_(nullptr) { message_[0] = '\0'; }

        void Attach(IObserver *observer) override
        {
            list *n = (list *)std::malloc(sizeof(list));
            n->observer = observer;
            n->next     = head_;
            head_       = n;
        }

        void Detach(IObserver *observer) override
        {
            (void)observer;
            /* TBD : When we have hardware */
        }

        void Notify() override
        {
            for (list *p = head_; p != nullptr; p = p->next) {
                p->observer->Update(message_);
            }
        }

        /* Detector verdict -> whitelist action -> broadcast to every node. */
        void RaiseIncident(const char *source, int action, float ttf_s)
        {
            std::snprintf(message_, sizeof(message_), "%s ttf %.1fs -> %s",
                          source, ttf_s, action_name(action));
            std::cout << "[K3 hub] incident: " << message_ << "  (notifying all nodes)\n";
            Notify();
        }

    private:
        list *head_;
        char  message_[96];
};

int main()
{
    /* TBD : We could use this in the UI interface, last layer */
    ZonalHub hub;
    EdgeNode powertrain("powertrain", SDV_NODE_POWERTRAIN);
    EdgeNode chassis("chassis", SDV_NODE_CHASSIS);
    EdgeNode body("body", SDV_NODE_BODY);
    EdgeNode acoustic("acoustic", SDV_NODE_ACOUSTIC);

    hub.Attach(&powertrain);
    hub.Attach(&chassis);
    hub.Attach(&body);
    hub.Attach(&acoustic);
    std::cout << "[K3 hub] 4 edge nodes attached\n\n";

    std::cout << "------------------------------------------------------------\n";
    hub.RaiseIncident("powertrain", SDV_RESTART, 6.2f);

    std::cout << "\n------------------------------------------------------------\n";
    hub.RaiseIncident("body", SDV_DEGRADED_MODE, 0.0f);
    return 0;
}
