/**
 * @file test/main.c  Selftest for Baresip core
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <getopt.h>
#include <re.h>
#include <baresip.h>
#include "test.h"


typedef int (test_exec_h)(void);

struct test {
	test_exec_h *exec;
	const char *name;
};

#define TEST(a) {a, #a}

static const struct test tests[] = {
	TEST(test_call_af_mismatch),
	TEST(test_call_answer),
	TEST(test_call_answer_hangup_a),
	TEST(test_call_answer_hangup_b),
	TEST(test_call_reject),
	TEST(test_cmd),
	TEST(test_cplusplus),
	TEST(test_mos),
	TEST(test_ua_alloc),
	TEST(test_ua_register),
	TEST(test_ua_register_dns),
	TEST(test_ua_register_auth),
	TEST(test_uag_find_param),
};


static int run_one_test(const struct test *test)
{
	int err;

	re_printf("[ RUN      ] %s\n", test->name);

	err = test->exec();
	if (err) {
		warning("%s: test failed (%m)\n",
			test->name, err);
		return err;
	}

	re_printf("[       OK ]\n");

	return 0;
}


static int run_tests(void)
{
	size_t i;
	int err;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		re_printf("[ RUN      ] %s\n", tests[i].name);

		err = tests[i].exec();
		if (err) {
			warning("%s: test failed (%m)\n",
				tests[i].name, err);
			return err;
		}

		re_printf("[       OK ]\n");
	}

	return 0;
}


static void test_listcases(void)
{
	size_t i, n;

	n = ARRAY_SIZE(tests);

	(void)re_printf("\n%zu test cases:\n", n);

	for (i=0; i<(n+1)/2; i++) {

		(void)re_printf("    %-32s    %s\n",
				tests[i].name,
				(i+(n+1)/2) < n ? tests[i+(n+1)/2].name : "");
	}

	(void)re_printf("\n");
}


static const struct test *find_test(const char *name)
{
	size_t i;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		if (0 == str_casecmp(name, tests[i].name))
			return &tests[i];
	}

	return NULL;
}


static void usage(void)
{
	(void)re_fprintf(stderr,
			 "Usage: selftest [options] <testcases..>\n"
			 "options:\n"
			 "\t-l               List all testcases and exit\n"
			 "\t-v               Verbose output (INFO level)\n"
			 );
}


int main(int argc, char *argv[])
{
	struct config *config;
	size_t i, ntests;
	int err;

	err = libre_init();
	if (err)
		return err;

	log_enable_info(false);

	for (;;) {
		const int c = getopt(argc, argv, "hlv");
		if (0 > c)
			break;

		switch (c) {

		case '?':
		case 'h':
			usage();
			return -2;

		case 'l':
			test_listcases();
			return 0;

		case 'v':
			log_enable_info(true);
			break;

		default:
			break;
		}
	}

	if (argc >= (optind + 1))
		ntests = argc - optind;
	else
		ntests = ARRAY_SIZE(tests);

	re_printf("running baresip selftest version %s with %zu tests\n",
		  BARESIP_VERSION, ntests);

	/* note: run SIP-traffic on localhost */
	config = conf_config();
	if (!config) {
		err = ENOENT;
		goto out;
	}
	str_ncpy(config->sip.local, "127.0.0.1:0", sizeof(config->sip.local));

#if 0
	/* XXX: needed for ua tests */
	err = ua_init("test", true, true, true, false);
	if (err)
		goto out;
#endif

	if (argc >= (optind + 1)) {

		for (i=0; i<ntests; i++) {
			const char *name = argv[optind + i];
			const struct test *test;

			test = find_test(name);
			if (test) {
				err = run_one_test(test);
				if (err)
					goto out;
			}
			else {
				re_fprintf(stderr,
					   "testcase not found: `%s'\n",
					   name);
				err = ENOENT;
				goto out;
			}
		}
	}
	else {
		err = run_tests();
		if (err)
			goto out;
	}

#if 1
	ua_stop_all(true);
#endif

	re_printf("\x1b[32mOK. %zu tests passed successfully\x1b[;m\n",
		  ntests);

 out:
	if (err) {
		warning("test failed (%m)\n", err);
		re_printf("%H\n", re_debug, 0);
	}
	ua_stop_all(true);
	ua_close();

	libre_close();

	tmr_debug();
	mem_debug();

	return err;
}
