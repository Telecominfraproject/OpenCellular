/* (C) 2012 by Holger Hans Peter Freyther
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

#include <osmo-bts/bts.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/paging.h>
#include <osmo-bts/gsm_data.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>

#include <errno.h>
#include <unistd.h>

static struct gsm_bts *bts;

#define ASSERT_TRUE(rc) \
	if (!(rc)) { \
		printf("Assert failed in %s:%d.\n",  \
		       __FILE__, __LINE__);          \
		abort();			     \
	}

static void test_cipher_parsing(void)
{
	int i;

	bts->support.ciphers = 0;

	/* always support A5/0 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x0) == -ENOTSUP);
	ASSERT_TRUE(bts_supports_cipher(bts, 0x1) == 1); /* A5/0 */
	for (i = 2; i <= 8; ++i) {
		ASSERT_TRUE(bts_supports_cipher(bts, i) == 0);
	}

	/* checking default A5/1 to A5/3 support */
	bts->support.ciphers = CIPHER_A5(1) | CIPHER_A5(2) | CIPHER_A5(3);
	ASSERT_TRUE(bts_supports_cipher(bts, 0x0) == -ENOTSUP);
	ASSERT_TRUE(bts_supports_cipher(bts, 0x1) == 1); /* A5/0 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x2) == 1); /* A5/1 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x3) == 1); /* A5/2 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x4) == 1); /* A5/3 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x5) == 0); /* A5/4 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x6) == 0); /* A5/5 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x7) == 0); /* A5/6 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x8) == 0); /* A5/7 */
	ASSERT_TRUE(bts_supports_cipher(bts, 0x9) == -ENOTSUP);
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

	test_cipher_parsing();
	printf("Success\n");

	return 0;
}

