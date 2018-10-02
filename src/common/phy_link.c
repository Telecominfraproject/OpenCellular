#include <stdint.h>

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/talloc.h>

#include <osmo-bts/bts.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts_model.h>

static LLIST_HEAD(g_phy_links);

struct phy_link *phy_link_by_num(int num)
{
	struct phy_link *plink;

	llist_for_each_entry(plink, &g_phy_links, list) {
		if (plink->num == num)
			return plink;
	}

	return NULL;
}

struct phy_link *phy_link_create(void *ctx, int num)
{
	struct phy_link *plink;

	if (phy_link_by_num(num))
		return NULL;

	plink = talloc_zero(ctx, struct phy_link);
	plink->num = num;
	plink->state = PHY_LINK_SHUTDOWN;
	INIT_LLIST_HEAD(&plink->instances);
	llist_add_tail(&plink->list, &g_phy_links);

	bts_model_phy_link_set_defaults(plink);

	return plink;
}

const struct value_string phy_link_state_vals[] = {
	{ PHY_LINK_SHUTDOWN, 	"shutdown" },
	{ PHY_LINK_CONNECTING,	"connecting" },
	{ PHY_LINK_CONNECTED,	"connected" },
	{ 0, NULL }
};

void phy_link_state_set(struct phy_link *plink, enum phy_link_state state)
{
	struct phy_instance *pinst;

	LOGP(DL1C, LOGL_INFO, "PHY link state change %s -> %s\n",
	     get_value_string(phy_link_state_vals, plink->state),
	     get_value_string(phy_link_state_vals, state));

	/* notify all TRX associated with this phy */
	llist_for_each_entry(pinst, &plink->instances, list) {
		struct gsm_bts_trx *trx = pinst->trx;
		if (!trx)
			continue;

		switch (state) {
		case PHY_LINK_CONNECTED:
			LOGP(DL1C, LOGL_INFO, "trx_set_avail(1)\n");
			trx_set_available(trx, 1);
			break;
		case PHY_LINK_SHUTDOWN:
			LOGP(DL1C, LOGL_INFO, "trx_set_avail(0)\n");
			trx_set_available(trx, 0);
			break;
		case PHY_LINK_CONNECTING:
			/* nothing to do */
			break;
		}
	}

	plink->state = state;
}

struct phy_instance *phy_instance_by_num(struct phy_link *plink, int num)
{
	struct phy_instance *pinst;

	llist_for_each_entry(pinst, &plink->instances, list) {
		if (pinst->num == num)
			return pinst;
	}
	return NULL;
}

struct phy_instance *phy_instance_create(struct phy_link *plink, int num)
{
	struct phy_instance *pinst;

	if (phy_instance_by_num(plink, num))
		return NULL;

	pinst = talloc_zero(plink, struct phy_instance);
	pinst->num = num;
	pinst->phy_link = plink;
	llist_add_tail(&pinst->list, &plink->instances);

	bts_model_phy_instance_set_defaults(pinst);

	return pinst;
}

void phy_instance_link_to_trx(struct phy_instance *pinst, struct gsm_bts_trx *trx)
{
	trx->role_bts.l1h = pinst;
	pinst->trx = trx;
}

void phy_instance_destroy(struct phy_instance *pinst)
{
	/* remove from list of instances in the link */
	llist_del(&pinst->list);

	/* remove reverse link from TRX */
	OSMO_ASSERT(pinst->trx->role_bts.l1h == pinst);
	pinst->trx->role_bts.l1h = NULL;
	pinst->trx = NULL;

	talloc_free(pinst);
}

void phy_link_destroy(struct phy_link *plink)
{
	struct phy_instance *pinst, *pinst2;

	llist_for_each_entry_safe(pinst, pinst2, &plink->instances, list)
		phy_instance_destroy(pinst);

	talloc_free(plink);
}

int phy_links_open(void)
{
	struct phy_link *plink;

	llist_for_each_entry(plink, &g_phy_links, list) {
		int rc;

		rc = bts_model_phy_link_open(plink);
		if (rc < 0)
			return rc;
	}

	return 0;
}

const char *phy_instance_name(struct phy_instance *pinst)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "phy%u.%u", pinst->phy_link->num,
		 pinst->num);
	return buf;
}
