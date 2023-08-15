#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/provider.h>

struct test_ {
	const char *name;
	const bool expected;
	const bool (*test_fn)(void);
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

static bool
test_fips_module_is_available(void)
{
	OSSL_LIB_CTX *ctx = OSSL_LIB_CTX_get0_global_default();

	if (!OSSL_PROVIDER_available(ctx, "fips"))
		return false;

	return true;
}

static bool
test_legacy_module_is_available(void)
{
	OSSL_LIB_CTX *ctx = OSSL_LIB_CTX_get0_global_default();

	if (!OSSL_PROVIDER_available(ctx, "legacy"))
		return false;

	return true;
}

static bool
test_unspec_md5_hashing_xfail(void)
{
	EVP_MD *md5 = EVP_MD_fetch(NULL, "MD5", NULL);
	if (md5 != NULL)
		return false;

	return true;
}

static bool
test_fips_md5_hashing_xfail(void)
{
	EVP_MD *md5 = EVP_MD_fetch(NULL, "MD5", "provider=fips");
	if (md5 != NULL)
		return false;

	return true;
}

static bool
test_fips_sha2_512_hashing_xpass(void)
{
	EVP_MD *sha2_512 = EVP_MD_fetch(NULL, "SHA2-512", "provider=fips");
	if (sha2_512 == NULL)
		return false;

	return true;
}

static bool
test_escape_fips(void)
{
	EVP_MD *md5 = EVP_MD_fetch(NULL, "MD5", "provider=default");
	if (md5 != NULL)
		return false;

	return true;
}

static bool
test_crypto_is_fips_jailed(void)
{
	/* This should always work. */
	EVP_MD *sha2_512 = EVP_MD_fetch(NULL, "SHA2-512", "provider=fips");
	if (sha2_512 == NULL)
		return false;

	/* This should not. */
	sha2_512 = EVP_MD_fetch(NULL, "SHA2-512", "provider=default");
	if (sha2_512 != NULL)
		return false;

	return true;
}

static const struct test_ tests[] = {
	{
		.name = "FIPS module is available",
		.expected = true,
		.test_fn = test_fips_module_is_available,
	},
	{
		.name = "legacy cryptographic routines are not available",
		.expected = false,
		.test_fn = test_legacy_module_is_available,
	},
	{
		.name = "non-specialized MD5 hashing operations are rejected",
		.expected = true,
		.test_fn = test_unspec_md5_hashing_xfail,
	},
	{
		.name = "the FIPS provider does not provide an MD5 hashing function",
		.expected = true,
		.test_fn = test_fips_md5_hashing_xfail,
	},
	{
		.name = "the FIPS provider does provide an SHA2-512 hashing function",
		.expected = true,
		.test_fn = test_fips_sha2_512_hashing_xpass,
	},
	{
		.name = "requesting MD5 from the default provider is rejected",
		.expected = true,
		.test_fn = test_escape_fips,
	},
	{
		.name = "cryptographic routines are restricted to the FIPS module",
		.expected = true,
		.test_fn = test_crypto_is_fips_jailed,
	},
};

int
main(int argc, const char *argv[])
{
	fprintf(stderr, "Running OpenSSL FIPS validation tests.\n");

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++)
	{
		fprintf(stderr, "*** Running check: %s...", tests[i].name);

		bool ret = tests[i].test_fn();
		if (ret != tests[i].expected)
		{
			fprintf(stderr, " FAILED\n!!! TEST RESULT DID NOT MATCH EXPECTED RESULT !!!\n");
			return EXIT_FAILURE;
		}
		else
		{
			fprintf(stderr, " passed.\n");
		}
	}

	fprintf(stderr, "All FIPS validation tests pass, OpenSSL is configured correctly.\n");

	return EXIT_SUCCESS;
}
