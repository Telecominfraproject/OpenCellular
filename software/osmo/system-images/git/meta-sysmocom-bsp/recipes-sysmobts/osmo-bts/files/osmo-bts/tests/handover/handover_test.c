#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sched.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/bits.h>
#include <osmocom/core/backtrace.h>
#include <osmocom/abis/abis.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/handover.h>

uint8_t phys_info[] = { 0x03, 0x03, 0x0d, 0x06, 0x2d, 0x00, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b };

static struct gsm_bts *bts;
struct gsm_bts_trx *trx;
int quit = 0;
uint8_t abis_mac[6] = { 0, 1, 2, 3, 4, 5 };
int modify_count = 0;

static void expect_phys_info(struct lapdm_entity *le)
{
	struct osmo_phsap_prim pp;
	int rc;

	rc = lapdm_phsap_dequeue_prim(le, &pp);
	OSMO_ASSERT(rc == 0);
	OSMO_ASSERT(sizeof(phys_info) == pp.oph.msg->len);
	OSMO_ASSERT(!memcmp(phys_info, pp.oph.msg->data, pp.oph.msg->len));
	msgb_free(pp.oph.msg);
}

int main(int argc, char **argv)
{
	void *tall_bts_ctx;
	struct e1inp_line *line;
	struct gsm_lchan *lchan;
	struct osmo_phsap_prim nl1sap;
	struct msgb *msg;
	struct abis_rsl_dchan_hdr *rslh;
	int i;

	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 0);

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);
	osmo_stderr_target->categories[DHO].loglevel = LOGL_DEBUG;

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (!bts) {
		fprintf(stderr, "Failed to create BTS structure\n");
		exit(1);
	}
	trx = gsm_bts_trx_alloc(bts);
	if (!trx) {
		fprintf(stderr, "Failed to TRX structure\n");
		exit(1);
	}

	if (bts_init(bts) < 0) {
		fprintf(stderr, "unable to to open bts\n");
		exit(1);
	}

	libosmo_abis_init(NULL);

	line = e1inp_line_create(0, "ipa");
	OSMO_ASSERT(line);

	e1inp_ts_config_sign(&line->ts[E1INP_SIGN_RSL-1], line);
	trx->rsl_link = e1inp_sign_link_create(&line->ts[E1INP_SIGN_RSL-1], E1INP_SIGN_RSL, NULL, 0, 0);
	OSMO_ASSERT(trx->rsl_link);
	trx->rsl_link->trx = trx;

	fprintf(stderr, "test 1: without timeout\n");

	/* create two lchans for handover */
	lchan = &trx->ts[1].lchan[0];
	lchan->type = GSM_LCHAN_SDCCH;
	l1sap_chan_act(lchan->ts->trx, 0x09, NULL);
	lchan = &trx->ts[2].lchan[0];
	lchan->type = GSM_LCHAN_TCH_F;
	lchan->ho.active = HANDOVER_ENABLED;
	lchan->ho.ref = 23;
	l1sap_chan_act(lchan->ts->trx, 0x0a, NULL);
	OSMO_ASSERT(msgb_dequeue(&trx->rsl_link->tx_list));
	OSMO_ASSERT(msgb_dequeue(&trx->rsl_link->tx_list));
	OSMO_ASSERT(!msgb_dequeue(&trx->rsl_link->tx_list));

	/* send access burst with wrong ref */
	memset(&nl1sap, 0, sizeof(nl1sap));
	osmo_prim_init(&nl1sap.oph, SAP_GSM_PH, PRIM_PH_RACH, PRIM_OP_INDICATION, NULL);
	nl1sap.u.rach_ind.chan_nr = 0x0a;
	nl1sap.u.rach_ind.ra = 42;
	l1sap_up(trx, &nl1sap);

	/* expect no action */
	OSMO_ASSERT(modify_count == 0);
	OSMO_ASSERT(!msgb_dequeue(&trx->rsl_link->tx_list));

	/* send access burst with correct ref */
	nl1sap.u.rach_ind.ra = 23;
	l1sap_up(trx, &nl1sap);
	OSMO_ASSERT(modify_count == 1);

	/* expect PHYS INFO */
	expect_phys_info(&trx->ts[2].lchan[0].lapdm_ch.lapdm_dcch);

	/* expect exactly one HO.DET */
	OSMO_ASSERT(msg = msgb_dequeue(&trx->rsl_link->tx_list));
	rslh = msgb_l2(msg);
	OSMO_ASSERT(rslh->c.msg_type == RSL_MT_HANDO_DET);
	OSMO_ASSERT(!msgb_dequeue(&trx->rsl_link->tx_list));

	/* expect T3105 running */
	OSMO_ASSERT(osmo_timer_pending(&trx->ts[2].lchan[0].ho.t3105))

	/* indicate frame */
	handover_frame(&trx->ts[2].lchan[0]);

	/* expect T3105 not running */
	OSMO_ASSERT(!osmo_timer_pending(&trx->ts[2].lchan[0].ho.t3105))

	fprintf(stderr, "test 2: with timeout\n");

	/* enable handover again */
	lchan = &trx->ts[2].lchan[0];
	lchan->ho.active = HANDOVER_ENABLED;
	lchan->ho.ref = 23;
	modify_count = 0;

	/* send access burst with correct ref */
	nl1sap.u.rach_ind.ra = 23;
	l1sap_up(trx, &nl1sap);
	OSMO_ASSERT(modify_count == 1);

	/* expect PHYS INFO */
	expect_phys_info(&trx->ts[2].lchan[0].lapdm_ch.lapdm_dcch);

	/* expect exactly one HO.DET */
	OSMO_ASSERT(msg = msgb_dequeue(&trx->rsl_link->tx_list));
	rslh = msgb_l2(msg);
	OSMO_ASSERT(rslh->c.msg_type == RSL_MT_HANDO_DET);
	OSMO_ASSERT(!msgb_dequeue(&trx->rsl_link->tx_list));

	for (i = 0; i < bts->ny1 - 1; i++) {
		/* expect T3105 running */
		OSMO_ASSERT(osmo_timer_pending(&trx->ts[2].lchan[0].ho.t3105))

		/* timeout T3105 */
		gettimeofday(&trx->ts[2].lchan[0].ho.t3105.timeout, NULL);
		osmo_select_main(0);

		/* expect PHYS INFO */
		expect_phys_info(&trx->ts[2].lchan[0].lapdm_ch.lapdm_dcch);
	}

	/* timeout T3105 */
	gettimeofday(&trx->ts[2].lchan[0].ho.t3105.timeout, NULL);
	osmo_select_main(0);

	/* expect T3105 not running */
	OSMO_ASSERT(!osmo_timer_pending(&trx->ts[2].lchan[0].ho.t3105))

	/* expect exactly one CONN.FAIL */
	OSMO_ASSERT(msg = msgb_dequeue(&trx->rsl_link->tx_list));
	rslh = msgb_l2(msg);
	OSMO_ASSERT(rslh->c.msg_type == RSL_MT_CONN_FAIL);
	OSMO_ASSERT(!msgb_dequeue(&trx->rsl_link->tx_list));

#if 0
	while (!quit) {
		log_reset_context();
		osmo_select_main(0);
	}
#endif

	printf("Success\n");

	return 0;
}

void bts_model_abis_close(struct gsm_bts *bts)
{
}

int bts_model_oml_estab(struct gsm_bts *bts)
{
	return 0;
}

int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	int rc = 0;
	uint8_t chan_nr;
	uint8_t tn, ss;
	struct gsm_lchan *lchan;
	struct msgb *msg = l1sap->oph.msg;
	struct osmo_phsap_prim nl1sap;

	switch (OSMO_PRIM_HDR(&l1sap->oph)) {
	case OSMO_PRIM(PRIM_MPH_INFO, PRIM_OP_REQUEST):
		switch (l1sap->u.info.type) {
		case PRIM_INFO_ACTIVATE:
			chan_nr = l1sap->u.info.u.act_req.chan_nr;
			tn = L1SAP_CHAN2TS(chan_nr);
			ss = l1sap_chan2ss(chan_nr);
			lchan = &trx->ts[tn].lchan[ss];

			lchan_init_lapdm(lchan);

			lchan_set_state(lchan, LCHAN_S_ACTIVE);

			memset(&nl1sap, 0, sizeof(nl1sap));
			osmo_prim_init(&nl1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO, PRIM_OP_CONFIRM, NULL);
			nl1sap.u.info.type = PRIM_INFO_ACTIVATE;
			nl1sap.u.info.u.act_cnf.chan_nr = chan_nr;
			return l1sap_up(trx, &nl1sap);
		case PRIM_INFO_MODIFY:
			modify_count++;
			break;
		default:
			LOGP(DL1C, LOGL_NOTICE, "unknown MPH-INFO.req %d\n",
				l1sap->u.info.type);
			rc = -EINVAL;
			goto done;
		}
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "unknown prim %d op %d\n",
		l1sap->oph.primitive, l1sap->oph.operation);
		rc = -EINVAL;
		goto done;
	}

done:
	if (msg)
		msgb_free(msg);
	return rc;
}

int bts_model_check_oml(struct gsm_bts *bts, uint8_t msg_type, struct tlv_parsed *old_attr, struct tlv_parsed *new_attr, void *obj) { return 0; }
int bts_model_apply_oml(struct gsm_bts *bts, struct msgb *msg, struct tlv_parsed *new_attr, int obj_kind, void *obj) { return 0; }
int bts_model_opstart(struct gsm_bts *bts, struct gsm_abis_mo *mo, void *obj) { return 0; }
int bts_model_chg_adm_state(struct gsm_bts *bts, struct gsm_abis_mo *mo, void *obj, uint8_t adm_state) { return 0; }
int bts_model_init(struct gsm_bts *bts) { return 0; }
int bts_model_trx_deact_rf(struct gsm_bts_trx *trx) { return 0; }
int bts_model_trx_close(struct gsm_bts_trx *trx) { return 0; }
void trx_get_hlayer1(void) {}
int bts_model_adjst_ms_pwr(struct gsm_lchan *lchan) { return 0; }
int bts_model_ts_disconnect(struct gsm_bts_trx_ts *ts) { return 0; }
int bts_model_ts_connect(struct gsm_bts_trx_ts *ts, enum gsm_phys_chan_config as_pchan) { return 0; }
int bts_model_lchan_deactivate(struct gsm_lchan *lchan) { return 0; }
int bts_model_lchan_deactivate_sacch(struct gsm_lchan *lchan) { return 0; }
