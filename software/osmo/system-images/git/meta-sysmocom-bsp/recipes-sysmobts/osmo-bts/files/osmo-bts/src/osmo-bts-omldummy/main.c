
#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/oml.h>


int main(int argc, char **argv)
{
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct e1inp_line *line;
	int i;

	char *dst_host = argv[1];
	int site_id = atoi(argv[2]);

	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 10*1024);

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (!bts)
		exit(1);
	bts->ip_access.site_id = site_id;
	bts->ip_access.bts_id = 0;

	/* Additional TRXs */
	for (i = 1; i < 8; i++) {
		trx = gsm_bts_trx_alloc(bts);
		if (!trx)
			exit(1);
	}

	if (bts_init(bts) < 0)
		exit(1);
	//btsb = bts_role_bts(bts);
	abis_init(bts);


	line = abis_open(bts, dst_host, "OMLdummy");
	if (!line)
		exit(2);

	while (1) {
		osmo_select_main(0);
	}

	return EXIT_SUCCESS;
}
