#ifndef GRID_EXPERIMENT_H
#define GRID_EXPERIMENT_H
/* Diagnostic-only, bounded live experiment. Default always renders. */
typedef struct {unsigned mode,serial,ordinal,frames,seen,skipped,bad;} GridExperiment;
static inline void grid_experiment_set(GridExperiment *s,unsigned mode,unsigned serial){*s=(GridExperiment){.mode=mode<=6?mode:0,.serial=serial};}
static inline int grid_experiment_skip(GridExperiment *s){
 s->ordinal++;s->seen++;
 int skip=(s->mode==1&&s->ordinal==1)||(s->mode==2&&s->ordinal==2);
 if(skip)s->skipped++;
 return skip;
}
static inline void grid_experiment_frame(GridExperiment *s){
 if(s->ordinal&&s->ordinal!=2)s->bad++;
 s->ordinal=0;
 if(s->mode&&++s->frames>=1800)s->mode=0;
}
#endif
