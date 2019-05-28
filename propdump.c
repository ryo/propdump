#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sysexits.h>
#include <unistd.h>
#include <prop/proplib.h>

static void prop_dump_obj(char *key, size_t keylen, prop_object_t parent, prop_object_t obj);
static void prop_dump_dict(char *key, size_t keylen, prop_dictionary_t dict);
static void prop_dump_array(char *key, size_t keylen, prop_array_t array);

static void
prop_dump_obj(char *key, size_t keylen, prop_object_t parent, prop_object_t obj)
{
	prop_object_t obj2;
	uint64_t num;
	const char *p;
	char *k;
	prop_type_t ptype;

	ptype = prop_object_type(obj);

	switch (ptype) {
	case PROP_TYPE_BOOL:
		printf("%s=%s\n", key, prop_bool_true(obj) ? "true" : "false");
		break;
	case PROP_TYPE_NUMBER:
		num = prop_number_unsigned_integer_value(obj);
		printf("%s=%llu\n", key, (unsigned long long)num);
		break;
	case PROP_TYPE_STRING:
		p = prop_string_cstring_nocopy(obj);
		printf("%s=\"%s\"\n", key, p);
		break;
	case PROP_TYPE_DATA:
		printf("%s=PROP_TYPE_DATA\n", key);
		break;
	case PROP_TYPE_ARRAY:
		prop_dump_array(key, keylen, obj);
		break;
	case PROP_TYPE_DICTIONARY:
		prop_dump_dict(key, keylen, obj);
		break;
	case PROP_TYPE_DICT_KEYSYM:
		p = prop_dictionary_keysym_cstring_nocopy(obj);
		k = key + strlen(key);
		if (key[0] != '\0')
			strlcat(key, ".", keylen);
		strlcat(key, p, keylen);
		obj2 = prop_dictionary_get_keysym(parent, obj);
		prop_dump_obj(key, keylen, obj, obj2);
		*k = '\0';
		break;
	default:
		printf("unknown prop type: %d\n", ptype);
		break;
	}
}

static void
prop_dump_array(char *key, size_t keylen, prop_array_t array)
{
	prop_object_iterator_t itr;
	prop_object_t obj;

	itr = prop_array_iterator(array);
	for (obj = prop_object_iterator_next(itr); obj != NULL;
	    obj = prop_object_iterator_next(itr)) {
		prop_dump_obj(key, keylen, array, obj);
	}

	prop_object_iterator_release(itr);
}

static void
prop_dump_dict(char *key, size_t keylen, prop_dictionary_t dict)
{
	prop_object_iterator_t itr;
	prop_object_t obj;

	itr = prop_dictionary_iterator(dict);
	for (obj = prop_object_iterator_next(itr); obj != NULL;
	    obj = prop_object_iterator_next(itr)) {
		prop_dump_obj(key, keylen, dict, obj);
	}

	prop_object_iterator_release(itr);
}

char *
freadin(FILE *fh)
{
#define READIN_BLOCKSIZE	(1024 * 4)
	char *buf, *p;
	size_t size, done, rc;

	size = READIN_BLOCKSIZE;
	buf = p = malloc(size);
	if (buf == NULL)
		return NULL;
	done = 0;
	for (;;) {
		rc = fread(p, 1, READIN_BLOCKSIZE, fh);
		if (rc < 0)
			err(2, "fread");
		done += rc;
		if (rc < READIN_BLOCKSIZE)
			break;

		size += READIN_BLOCKSIZE;
		p = realloc(buf, size);
		if (p == NULL) {
			err(3, "malloc");
		}
		buf = p;
		p = buf + done;
	}

	if (done == size) {
		p = realloc(buf, size + 1);
		if (p == NULL)
			err(3, "malloc");
		buf = p;
	}
	p = buf + done;
	*p = '\0';

	return buf;
}

static int
fpropdump(FILE *fh)
{
	char key[1024];
	prop_object_t propobj;
	char *xml;

	xml = freadin(fh);
	if (xml == NULL) {
		fprintf(stderr, "cannot allocate memory\n");
		return EX_OSERR;
	}

	propobj = prop_dictionary_internalize(xml);
	if (propobj == NULL)
		return EX_DATAERR;

	key[0] = '\0';
	prop_dump_obj(key, sizeof(key), NULL, propobj);
	prop_object_release(propobj);
	free(xml);
	return 0;
}

int
main(int argc, char *argv[])
{
	FILE *fh;
	int i, ret = 0;
	const char *fname;

	if (argc < 2 || (argc == 2 && strcmp(argv[1], "-") == 0))
		return fpropdump(stdin);

	for (i = 1; i < argc; i++) {
		fname = argv[i];

		if (argc > 2)
			printf("[%s]\n", fname);

		fh = fopen(fname, "r");
		if (fh == NULL) {
			fprintf(stderr, "fopen: %s: %s\n", fname, strerror(errno));
			if (argc == 2)
				return EX_NOINPUT;
			continue;
		}

		ret = fpropdump(fh);
		fclose(fh);
	}

	return ret;
}
