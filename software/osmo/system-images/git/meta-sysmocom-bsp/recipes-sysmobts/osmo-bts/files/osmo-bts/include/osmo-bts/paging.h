#ifndef OSMO_BTS_PAGING_H
#define OSMO_BTS_PAGING_H

#include <stdint.h>
#include <osmocom/gsm/gsm_utils.h>
#include <osmocom/gsm/protocol/gsm_04_08.h>

struct paging_state;
struct gsm_bts;

/* initialize paging code */
struct paging_state *paging_init(struct gsm_bts *bts,
				 unsigned int num_paging_max,
				 unsigned int paging_lifetime);

/* (re) configure paging code */
void paging_config(struct paging_state *ps,
		  unsigned int num_paging_max,
		  unsigned int paging_lifetime);

void paging_reset(struct paging_state *ps);

/* The max number of paging entries */
unsigned int paging_get_queue_max(struct paging_state *ps);
void paging_set_queue_max(struct paging_state *ps, unsigned int queue_max);

/* The lifetime of a paging entry */
unsigned int paging_get_lifetime(struct paging_state *ps);
void paging_set_lifetime(struct paging_state *ps, unsigned int lifetime);

/* update with new SYSTEM INFORMATION parameters */
int paging_si_update(struct paging_state *ps, struct gsm48_control_channel_descr *chan_desc);

/* Add an identity to the paging queue */
int paging_add_identity(struct paging_state *ps, uint8_t paging_group,
			const uint8_t *identity_lv, uint8_t chan_needed);

/* Add an IMM.ASS message to the paging queue */
int paging_add_imm_ass(struct paging_state *ps, const uint8_t *data,
                       uint8_t len);

/* generate paging message for given gsm time */
int paging_gen_msg(struct paging_state *ps, uint8_t *out_buf, struct gsm_time *gt,
		   int *is_empty);


/* inspection methods below */
int paging_group_queue_empty(struct paging_state *ps, uint8_t group);
int paging_queue_length(struct paging_state *ps);
int paging_buffer_space(struct paging_state *ps);

#endif
