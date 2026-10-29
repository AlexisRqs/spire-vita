#ifndef LAUNCHER_FLOW_H
#define LAUNCHER_FLOW_H
/* UI and engine share this only under the launcher's mutex. */
typedef struct {int active,ready,requested,page;} LauncherFlow;
static inline int launcher_flow_handoff(const LauncherFlow *s){
 return s->active&&s->ready&&s->requested&&!s->page;
}
#endif
