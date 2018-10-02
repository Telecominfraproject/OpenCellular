#ifndef _LC15BTS_SWD_H
#define _LC15BTS_SWD_H

int lc15bts_swd_init(struct lc15bts_mgr_instance *mgr, int swd_num_events);
int lc15bts_swd_event(struct lc15bts_mgr_instance *mgr, enum mgr_swd_events swd_event);

#endif
