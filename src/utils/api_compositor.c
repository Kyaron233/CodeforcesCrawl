#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "utils/api_compositor.h"

typedef struct {
	const char* key;
	const char* value;
} ParamPair;

static int compare_param_pair(const void* left, const void* right) {
	const ParamPair* a = (const ParamPair*)left;
	const ParamPair* b = (const ParamPair*)right;
	int key_cmp = strcmp(a->key, b->key);

	if (key_cmp != 0) {
		return key_cmp;
	}

	return strcmp(a->value, b->value);
}

static int generate_random_prefix(char out[7]) {
	static int seeded = 0;
	static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	size_t i;

	if (!seeded) {
		srand((unsigned int)(time(NULL) ^ (unsigned long)clock()));
		seeded = 1;
	}

	for (i = 0; i < 6; ++i) {
		out[i] = charset[rand() % 36];
	}
	out[6] = '\0';
	return 1;
}

static char* build_sorted_query(const ParamPair* pairs, size_t pair_count) {
	size_t i;
	size_t query_len = 0;
	char* query;
	char* cursor;

	if (pairs == NULL || pair_count == 0) {
		return NULL;
	}

	for (i = 0; i < pair_count; ++i) {
		query_len += strlen(pairs[i].key) + 1 + strlen(pairs[i].value);
		if (i + 1 < pair_count) {
			query_len += 1;
		}
	}

	query = (char*)malloc(query_len + 1);
	if (query == NULL) {
		return NULL;
	}

	cursor = query;
	for (i = 0; i < pair_count; ++i) {
		size_t key_len = strlen(pairs[i].key);
		size_t value_len = strlen(pairs[i].value);

		memcpy(cursor, pairs[i].key, key_len);
		cursor += key_len;
		*cursor++ = '=';
		memcpy(cursor, pairs[i].value, value_len);
		cursor += value_len;

		if (i + 1 < pair_count) {
			*cursor++ = '&';
		}
	}

	*cursor = '\0';
	return query;
}

char* generate_url_with_access_token(const char* key,
									 const char* secret,
									 const char* method,
									 size_t param_count,
									 const char* const* kv_pairs) {
	size_t i;
	size_t total_pairs;
	size_t method_len;
	size_t secret_len;
	size_t sorted_query_len;
	size_t signature_src_len;
	size_t url_len;
	ParamPair* pairs = NULL;
	char* sorted_query = NULL;
	char random_prefix[7];
	char* signature_src = NULL;
	unsigned char digest[SHA512_DIGEST_LENGTH];
	char digest_hex[SHA512_DIGEST_LENGTH * 2 + 1];
	char token[6 + SHA512_DIGEST_LENGTH * 2 + 1];
	char* final_url = NULL;

	if (key == NULL || secret == NULL || method == NULL || method[0] == '\0') {
		return NULL;
	}
	if (param_count > 0 && kv_pairs == NULL) {
		return NULL;
	}

	total_pairs = param_count + 1;
	pairs = (ParamPair*)malloc(total_pairs * sizeof(ParamPair));
	if (pairs == NULL) {
		return NULL;
	}

	pairs[0].key = "apiKey";
	pairs[0].value = key;

	for (i = 0; i < param_count; ++i) {
		const char* param_key = kv_pairs[i * 2];
		const char* param_value = kv_pairs[i * 2 + 1];

		if (param_key == NULL || param_value == NULL) {
			free(pairs);
			return NULL;
		}

		pairs[i + 1].key = param_key;
		pairs[i + 1].value = param_value;
	}

	qsort(pairs, total_pairs, sizeof(ParamPair), compare_param_pair);

	sorted_query = build_sorted_query(pairs, total_pairs);
	free(pairs);
	pairs = NULL;

	if (sorted_query == NULL) {
		return NULL;
	}

	if (!generate_random_prefix(random_prefix)) {
		free(sorted_query);
		return NULL;
	}

	method_len = strlen(method);
	secret_len = strlen(secret);
	sorted_query_len = strlen(sorted_query);
	signature_src_len = 6 + 1 + method_len + 1 + sorted_query_len + 1 + secret_len;

	signature_src = (char*)malloc(signature_src_len + 1);
	if (signature_src == NULL) {
		free(sorted_query);
		return NULL;
	}

	snprintf(signature_src,
			 signature_src_len + 1,
			 "%s/%s?%s#%s",
			 random_prefix,
			 method,
			 sorted_query,
			 secret);

	if (SHA512((const unsigned char*)signature_src, strlen(signature_src), digest) == NULL) {
		free(signature_src);
		free(sorted_query);
		return NULL;
	}
	free(signature_src);

	for (i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
		snprintf(digest_hex + i * 2, 3, "%02x", digest[i]);
	}
	digest_hex[SHA512_DIGEST_LENGTH * 2] = '\0';

	memcpy(token, random_prefix, 6);
	memcpy(token + 6, digest_hex, sizeof(digest_hex));

	url_len = 1 + method_len + 1 + sorted_query_len + 8 + strlen(token);
	final_url = (char*)malloc(url_len + 1);
	if (final_url == NULL) {
		free(sorted_query);
		return NULL;
	}

	snprintf(final_url,
			 url_len + 1,
			 "/%s?%s&apiSig=%s",
			 method,
			 sorted_query,
			 token);

	free(sorted_query);
	return final_url;
}