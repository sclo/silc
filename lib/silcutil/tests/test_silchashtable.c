/* Hash table tests */

#include "silcincludes.h"

typedef struct entry_struct {
  char name[8];
  int val;
} *entry;

bool dump = FALSE;
bool auto_rehash = TRUE;
int count = 2000;
SilcHashTable t = NULL;

SilcUInt32 hash_entry(void *key, void *user_context)
{
  entry e = key;
  return e->val + silc_hash_string(e->name, NULL);
}

bool hash_compare(void *key1, void *key2, void *user_context)
{
  entry e = key1;
  entry e2 = key2;
  if (e->val == e2->val && !strcmp(e->name, e2->name))
    return TRUE;
  return FALSE;
}

void hash_destructor(void *key, void *context, void *user_context)
{
  entry e = key;
  char *name = context;
  if (dump)
    SILC_LOG_DEBUG(("e=%p, e->val=%d, e->name=%s, context=%s",
		    e, e->val, e->name, name));
  memset(e, 'F', sizeof(*e));
  silc_free(e);
}

bool add_entries()
{
  entry e;
  int i;

  SILC_LOG_DEBUG(("Adding %d entries", count));

  for (i = 0; i < count; i++) {
    e = silc_calloc(1, sizeof(*e));
    if (!e)
      return FALSE;
    snprintf(e->name, sizeof(e->name), "%d", i);
    e->val = i;

    silc_hash_table_add(t, (void *)e, (void *)e->name);
  }

  SILC_LOG_DEBUG(("Hash table entry count: %d", silc_hash_table_count(t)));

  return TRUE;
}

bool del_entries_with_list()
{
  SilcHashTableList htl;
  entry e;
  char *name;

  SILC_LOG_DEBUG(("Deleting entries with SilcHashTableList"));

  silc_hash_table_list(t, &htl);
  while (silc_hash_table_get(&htl, (void **)&e, (void **)&name)) {
    if (!silc_hash_table_del(t, e))
      return FALSE;
  }
  silc_hash_table_list_reset(&htl);

  SILC_LOG_DEBUG(("Hash table entry count: %d", silc_hash_table_count(t)));

  return TRUE;
}

void del_foreach(void *key, void *context, void *user_context)
{
  entry e = key;
  char *name = context;
  if (dump)
    SILC_LOG_DEBUG(("del_foreach found e=%p, e->val=%d, e->name=%s, context=%s",
		    e, e->val, e->name, name));
  silc_hash_table_del(t, key);
}

bool del_n_entries_foreach()
{
  struct entry_struct f;
  int i;

  SILC_LOG_DEBUG(("Deleting keys 0-%d with foreach", count));

  for (i = 0; i < count; i++) {
    memset(&f, 0, sizeof(f));
    snprintf(f.name, sizeof(f.name), "%d", i);
    f.val = i;

    silc_hash_table_find_foreach(t, &f, del_foreach, NULL);
  }

  return TRUE;
}

bool del_entries_foreach()
{
  SILC_LOG_DEBUG(("Deleting all entries with foreach"));
  silc_hash_table_foreach(t, del_foreach, NULL);
  return TRUE;
}

bool alloc_table()
{
  SILC_LOG_DEBUG(("Allocating hash table with %d entries (%s)",
		  count, auto_rehash ? "auto rehash" : "no auto rehash"));

  t = silc_hash_table_alloc(0, hash_entry, NULL,
			    hash_compare, NULL,
			    hash_destructor, NULL, auto_rehash);

  if (!add_entries())
    return FALSE;

  SILC_LOG_DEBUG(("Hash table size: %d", silc_hash_table_size(t)));

  if (silc_hash_table_count(t) != count) {
    SILC_LOG_DEBUG(("Wrong table count %d", count));
    return FALSE;
  }

  return TRUE;
}

bool delete_table_with_list()
{

  SILC_LOG_DEBUG(("Deleting entries with SilcHashTableList"));

  if (!del_entries_with_list())
    return FALSE;

  SILC_LOG_DEBUG(("Hash table size: %d", silc_hash_table_size(t)));

  if (silc_hash_table_count(t) != 0) {
    SILC_LOG_DEBUG(("Wrong table count %d != 0", count));
    return FALSE;
  }

  silc_hash_table_free(t);
  t = NULL;

  return TRUE;
}

bool find_entries()
{
  struct entry_struct f;
  entry e;
  char *name;
  int i;

  SILC_LOG_DEBUG(("Finding %d entries", count));

  for (i = 0; i < count; i++) {
    memset(&f, 0, sizeof(f));
    snprintf(f.name, sizeof(f.name), "%d", i);
    f.val = i;

    /* Find */
    if (!silc_hash_table_find(t, &f, (void **)&e, (void **)&name))
      return FALSE;

    /* Find itself with context */
    if (!silc_hash_table_find_by_context(t, e, e->name, NULL))
      return FALSE;
  }

  return TRUE;
}

bool dump_table()
{
  SilcHashTableList htl;
  entry e;
  char *name;
  bool dumpped = FALSE;

  SILC_LOG_DEBUG(("Dumping hash table entries"));

  silc_hash_table_list(t, &htl);
  while (silc_hash_table_get(&htl, (void **)&e, (void **)&name)) {
    dumpped = TRUE;
    if (dump)
      SILC_LOG_DEBUG(("e=%p, e->val=%d, e->name=%s, context=%s",
		      e, e->val, e->name, name));
  }
  silc_hash_table_list_reset(&htl);

  return dumpped;
}

int main(int argc, char **argv)
{
  bool success = FALSE;
  int i;

  if (argc > 1 && !strcmp(argv[1], "-d")) {
    silc_debug = 1;
    silc_debug_hexdump = 1;
    silc_log_set_debug_string("*table*");
  }

  if (argc > 1 && !strcmp(argv[1], "-D")) {
    silc_debug = 1;
    dump = TRUE;
    silc_log_set_debug_string("*table*");
  }

  if (!alloc_table())
    goto err;
  if (!dump_table())
    goto err;
  if (!delete_table_with_list())
    goto err;

  count = 1387;
  auto_rehash = FALSE;
  if (!alloc_table())
    goto err;
  if (!dump_table())
    goto err;
  SILC_LOG_DEBUG(("rehash"));
  silc_hash_table_rehash(t, 0);
  SILC_LOG_DEBUG(("Hash table entry count: %d", silc_hash_table_count(t)));
  SILC_LOG_DEBUG(("Hash table size: %d", silc_hash_table_size(t)));
  if (!delete_table_with_list())
    goto err;

  count = 999;
  auto_rehash = TRUE;
  if (!alloc_table())
    goto err;
  count = 3999;
  if (!add_entries())
    goto err;
  SILC_LOG_DEBUG(("rehash"));
  silc_hash_table_rehash(t, 0);
  SILC_LOG_DEBUG(("Hash table entry count: %d", silc_hash_table_count(t)));
  SILC_LOG_DEBUG(("Hash table size: %d", silc_hash_table_size(t)));
  if (!del_entries_with_list())
    goto err;
  SILC_LOG_DEBUG(("rehash"));
  silc_hash_table_rehash(t, 0);
  SILC_LOG_DEBUG(("Hash table entry count: %d", silc_hash_table_count(t)));
  SILC_LOG_DEBUG(("Hash table size: %d", silc_hash_table_size(t)));
  count = 999;
  if (!add_entries())
    goto err;
  /* Adding duplicates */
  for (i = 0; i < 30; i++) {
    count = 50;
    if (!add_entries())
      goto err;
  }
  count = 700;
  if (!find_entries())
    goto err;
  count = 500;
  if (!del_n_entries_foreach())
    goto err;
  count = 999;
  if (!add_entries())
    goto err;
  count = 700;
  if (!find_entries())
    goto err;
  if (!dump_table())
    goto err;
  if (!del_entries_foreach())
    goto err;
  if (!delete_table_with_list())
    goto err;

  success = TRUE;

 err:
  SILC_LOG_DEBUG(("Testing was %s", success ? "SUCCESS" : "FAILURE"));
  fprintf(stderr, "Testing was %s\n", success ? "SUCCESS" : "FAILURE");

  return success;
}