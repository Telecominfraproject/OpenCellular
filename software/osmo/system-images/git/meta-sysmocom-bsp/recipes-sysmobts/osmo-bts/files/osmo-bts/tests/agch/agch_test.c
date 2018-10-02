/* testing the agch code */

/* (C) 2011 by Holger Hans Peter Freyther
 * (C) 2014 by sysmocom s.f.m.c. GmbH
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>

#include <osmo-bts/bts.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>

#include <inttypes.h>
#include <unistd.h>

static struct gsm_bts *bts;

static int count_imm_ass_rej_refs(struct gsm48_imm_ass_rej *rej)
{
	int count = 0;
	count++;

	if (memcmp(&rej->req_ref1, &rej->req_ref2, sizeof(rej->req_ref2))) {
		count++;
	}

	if (memcmp(&rej->req_ref1, &rej->req_ref3, sizeof(rej->req_ref3))
	    && memcmp(&rej->req_ref2, &rej->req_ref3, sizeof(rej->req_ref3))) {
		count++;
	}

	if (memcmp(&rej->req_ref1, &rej->req_ref4, sizeof(rej->req_ref4))
	    && memcmp(&rej->req_ref2, &rej->req_ref4, sizeof(rej->req_ref4))
	    && memcmp(&rej->req_ref3, &rej->req_ref4, sizeof(rej->req_ref4))) {
		count++;
	}

	return count;
}

static void put_imm_ass_rej(struct msgb *msg, int idx, uint8_t wait_ind)
{
	/* GSM CCCH - Immediate Assignment Reject */
	static const unsigned char gsm_a_ccch_data[23] = {
		0x4d, 0x06, 0x3a, 0x03, 0x25, 0x00, 0x00, 0x0a,
		0x25, 0x00, 0x00, 0x0a, 0x25, 0x00, 0x00, 0x0a,
		0x25, 0x00, 0x00, 0x0a, 0x2b, 0x2b, 0x2b
	};

	struct gsm48_imm_ass_rej *rej;
	msg->l3h = msgb_put(msg, sizeof(gsm_a_ccch_data));
	rej = (struct gsm48_imm_ass_rej *)msg->l3h;
	memmove(msg->l3h, gsm_a_ccch_data, sizeof(gsm_a_ccch_data));

	rej->req_ref1.t1 = idx;
	rej->wait_ind1 = wait_ind;

	rej->req_ref2.t1 = idx;
	rej->req_ref3.t1 = idx;
	rej->req_ref4.t1 = idx;
}

static void put_imm_ass(struct msgb *msg, int idx)
{
	/* GSM CCCH - Immediate Assignment */
	static const unsigned char gsm_a_ccch_data[23] = {
		0x2d, 0x06, 0x3f, 0x03, 0x0c, 0xe3, 0x69, 0x25,
		0x00, 0x00, 0x00, 0x00, 0x2b, 0x2b, 0x2b, 0x2b,
		0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b
	};

	struct gsm48_imm_ass *ima;
	msg->l3h = msgb_put(msg, sizeof(gsm_a_ccch_data));
	ima = (struct gsm48_imm_ass *)msg->l3h;
	memmove(msg->l3h, gsm_a_ccch_data, sizeof(gsm_a_ccch_data));

	ima->req_ref.t1 = idx;
}

static void test_agch_queue(void)
{
	int rc;
	uint8_t out_buf[GSM_MACBLOCK_LEN];
	struct gsm_time g_time;
	const int num_rounds = 40;
	const int num_ima_per_round = 2;
	const int num_rej_per_round = 16;

	int round, idx;
	int count = 0;
	struct msgb *msg = NULL;
	int multiframes = 0;
	int imm_ass_count = 0;
	int imm_ass_rej_count = 0;
	int imm_ass_rej_ref_count = 0;

	g_time.fn = 0;
	g_time.t1 = 0;
	g_time.t2 = 0;
	g_time.t3 = 6;

	printf("Testing AGCH messages queue handling.\n");
	bts->agch_queue.max_length = 32;

	bts->agch_queue.low_level = 30;
	bts->agch_queue.high_level = 30;
	bts->agch_queue.thresh_level = 60;

	for (round = 1; round <= num_rounds; round++) {
		for (idx = 0; idx < num_ima_per_round; idx++) {
			msg = msgb_alloc(GSM_MACBLOCK_LEN, __FUNCTION__);
			put_imm_ass(msg, ++count);
			bts_agch_enqueue(bts, msg);
			imm_ass_count++;
		}
		for (idx = 0; idx < num_rej_per_round; idx++) {
			msg = msgb_alloc(GSM_MACBLOCK_LEN, __FUNCTION__);
			put_imm_ass_rej(msg, ++count, 10);
			bts_agch_enqueue(bts, msg);
			imm_ass_rej_count++;
			imm_ass_rej_ref_count++;
		}
	}

	printf("AGCH filled: count %u, imm.ass %d, imm.ass.rej %d (refs %d), "
	       "queue limit %u, occupied %d, "
	       "dropped %"PRIu64", merged %"PRIu64", rejected %"PRIu64", "
	       "ag-res %"PRIu64", non-res %"PRIu64"\n",
	       count, imm_ass_count, imm_ass_rej_count, imm_ass_rej_ref_count,
	       bts->agch_queue.max_length, bts->agch_queue.length,
	       bts->agch_queue.dropped_msgs, bts->agch_queue.merged_msgs,
	       bts->agch_queue.rejected_msgs, bts->agch_queue.agch_msgs,
	       bts->agch_queue.pch_msgs);

	imm_ass_count = 0;
	imm_ass_rej_count = 0;
	imm_ass_rej_ref_count = 0;

	for (idx = 0; 1; idx++) {
		struct gsm48_imm_ass *ima;
		int is_agch = (idx % 3) == 0; /* 1 AG reserved, 2 PCH */
		if (is_agch)
			multiframes++;

		rc = bts_ccch_copy_msg(bts, out_buf, &g_time, is_agch);
		ima = (struct gsm48_imm_ass *)out_buf;
		switch (ima->msg_type) {
		case GSM48_MT_RR_IMM_ASS:
			imm_ass_count++;
			break;
		case GSM48_MT_RR_IMM_ASS_REJ:
			imm_ass_rej_count++;
			imm_ass_rej_ref_count +=
				count_imm_ass_rej_refs((struct gsm48_imm_ass_rej *)ima);
			break;
		default:
			break;
		}
		if (is_agch && rc <= 0)
			break;

	}

	printf("AGCH drained: multiframes %u, imm.ass %d, imm.ass.rej %d (refs %d), "
	       "queue limit %u, occupied %d, "
	       "dropped %"PRIu64", merged %"PRIu64", rejected %"PRIu64", "
	       "ag-res %"PRIu64", non-res %"PRIu64"\n",
	       multiframes, imm_ass_count, imm_ass_rej_count, imm_ass_rej_ref_count,
	       bts->agch_queue.max_length, bts->agch_queue.length,
	       bts->agch_queue.dropped_msgs, bts->agch_queue.merged_msgs,
	       bts->agch_queue.rejected_msgs, bts->agch_queue.agch_msgs,
	       bts->agch_queue.pch_msgs);
}

static void test_agch_queue_length_computation(void)
{
	static const int ccch_configs[] = {
		RSL_BCCH_CCCH_CONF_1_NC,
		RSL_BCCH_CCCH_CONF_1_C,
		RSL_BCCH_CCCH_CONF_2_NC,
		RSL_BCCH_CCCH_CONF_3_NC,
		RSL_BCCH_CCCH_CONF_4_NC,
	};
	static const uint8_t tx_integer[] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 20, 25, 32, 50,
	};

	int T_idx, c_idx, max_len;

	printf("Testing AGCH queue length computation.\n");

	printf("T\t\tBCCH slots\n");
	printf("\t1(NC)\t1(C)\t2(NC)\t3(NC)\t4(NC)\n");
	for (T_idx = 0; T_idx < ARRAY_SIZE(tx_integer); T_idx++) {
		printf("%d", tx_integer[T_idx]);
		for (c_idx = 0; c_idx < ARRAY_SIZE(ccch_configs); c_idx++) {
			max_len = bts_agch_max_queue_length(tx_integer[T_idx],
							    ccch_configs[c_idx]);
			printf("\t%d", max_len);
		}
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 0);

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (bts_init(bts) < 0) {
		fprintf(stderr, "unable to open bts\n");
		exit(1);
	}

	test_agch_queue_length_computation();
	test_agch_queue();
	printf("Success\n");

	return 0;
}

