#ifndef _OC2GBTS_SWD_H
#define _OC2GBTS_SWD_H

int oc2gbts_swd_init(struct oc2gbts_mgr_instance *mgr, int swd_num_events);
int oc2gbts_swd_event(struct oc2gbts_mgr_instance *mgr, enum mgr_swd_events swd_event);

#endif
