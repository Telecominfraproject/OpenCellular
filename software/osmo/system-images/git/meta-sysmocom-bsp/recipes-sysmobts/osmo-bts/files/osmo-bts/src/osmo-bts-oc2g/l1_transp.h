#ifndef _L1_TRANSP_H
#define _L1_TRANSP_H

#include <osmocom/core/msgb.h>

/* functions a transport calls on arrival of primitive from BTS */
int l1if_handle_l1prim(int wq, struct oc2gl1_hdl *fl1h, struct msgb *msg);
int l1if_handle_sysprim(struct oc2gl1_hdl *fl1h, struct msgb *msg);

/* functions exported by a transport */
int l1if_transport_open(int q, struct oc2gl1_hdl *fl1h);
int l1if_transport_close(int q, struct oc2gl1_hdl *fl1h);

#endif /* _L1_TRANSP_H */
