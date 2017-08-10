/*
 * test_iptc.c
 *
 *  Created on: 2017. 7. 12.
 *      Author: yang
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libiptc/libiptc.h>
#include <arpa/inet.h>

// return 0: OK
int	insert_rule(
			const char *table,
			const char *chain,
			unsigned int src,
			int	inverted_src,
			unsigned int dst,
			int	inverted_dst,
			const char *target)
{
	struct {
		struct ipt_entry entry;
		struct xt_standard_target target;
	} entry;
	struct xtc_handle *h;
	int	ret = 1;

	h = iptc_init(table);
	if (h == NULL) {
		printf("Init Error : %s\n", iptc_strerror(errno));
		return ret;
	}

	memset(&entry, 0, sizeof(entry));

	// target
	entry.target.target.u.user.target_size = XT_ALIGN(sizeof(struct xt_standard_target));
	strncpy(entry.target.target.u.user.name, target, sizeof(entry.target.target.u.user.name));

	// entry
	entry.entry.target_offset = sizeof(struct ipt_entry);
	entry.entry.next_offset = entry.entry.target_offset + entry.target.target.u.user.target_size;

	if (src) {
		entry.entry.ip.src.s_addr = src;
		entry.entry.ip.smsk.s_addr = 0xFFFFFFFF;
		if (inverted_src)
			entry.entry.ip.invflags |= IPT_INV_SRCIP;
	}

	if (dst) {
		entry.entry.ip.dst.s_addr = dst;
		entry.entry.ip.dmsk.s_addr = 0xFFFFFFFF;
		if (inverted_dst)
			entry.entry.ip.invflags |= IPT_INV_DSTIP;
	}

	if (iptc_append_entry(chain, (struct ipt_entry*)&entry, h) == 0) {
		printf("Append Error : %s\n", iptc_strerror(errno));
		goto STOP;
	}

	if (iptc_commit(h) == 0) {
		printf("Commit Error : %s\n", iptc_strerror(errno));
		goto STOP;
	}

	ret = 0;

STOP:
	if (h)
		iptc_free(h);

	return ret;
}

/* Here begins some of the code taken from iptables-save.c **************** */
#define IP_PARTS_NATIVE(n)      \
(unsigned int)((n)>>24)&0xFF,   \
(unsigned int)((n)>>16)&0xFF,   \
(unsigned int)((n)>>8)&0xFF,    \
(unsigned int)((n)&0xFF)

#define IP_PARTS(n) IP_PARTS_NATIVE(ntohl(n))

/* This assumes that mask is contiguous, and byte-bounded. */
static void
print_iface(char letter, const char *iface, const unsigned char *mask,
      int invert)
{
  unsigned int i;

  if (mask[0] == 0)
    return;

  printf("-%c %s", letter, invert ? "! " : "");

  for (i = 0; i < IFNAMSIZ; i++) {
    if (mask[i] != 0) {
      if (iface[i] != '\0')
        printf("%c", iface[i]);
    } else {
      /* we can access iface[i-1] here, because
       * a few lines above we make sure that mask[0] != 0 */
      if (iface[i-1] != '\0')
        printf("+");
      break;
    }
  }

  printf(" ");
}

/* These are hardcoded backups in iptables.c, so they are safe */
struct pprot {
  char *name;
  u_int8_t num;
};

/* FIXME: why don't we use /etc/protocols ? */
static const struct pprot chain_protos[] = {
  { "tcp", IPPROTO_TCP },
  { "udp", IPPROTO_UDP },
  { "icmp", IPPROTO_ICMP },
  { "esp", IPPROTO_ESP },
  { "ah", IPPROTO_AH },
};

static void print_proto(u_int16_t proto, int invert)
{
  if (proto) {
    unsigned int i;
    const char *invertstr = invert ? "! " : "";

    for (i = 0; i < sizeof(chain_protos)/sizeof(struct pprot); i++)
      if (chain_protos[i].num == proto) {
        printf("-p %s%s ",
               invertstr, chain_protos[i].name);
        return;
      }

    printf("-p %s%u ", invertstr, proto);
  }
}


static int print_match(const struct ipt_entry_match *e,
      const struct ipt_ip *ip)
{
    printf("-m %s ", e->u.user.name);
/*
  struct iptables_match *match
    = find_match(e->u.user.name, TRY_LOAD);

  if (match) {
    printf("-m %s ", e->u.user.name);

    // some matches don't provide a save function
    if (match->save)
      match->save(ip, e);
  } else {
    if (e->u.match_size) {
      fprintf(stderr,
        "Can't find library for match `%s'\n",
        e->u.user.name);
      exit(1);
    }
  }
 */
  return 0;
}


/* print a given ip including mask if neccessary */
static void print_ip(char *prefix, u_int32_t ip, u_int32_t mask, int invert)
{
  if (!mask && !ip)
    return;

  printf("%s %s%u.%u.%u.%u",
    prefix,
    invert ? "! " : "",
    IP_PARTS(ip));

  if (mask != 0xffffffff)
    printf("/%u.%u.%u.%u ", IP_PARTS(mask));
  else
    printf(" ");
}

/* We want this to be readable, so only print out neccessary fields.
 * Because that's the kind of world I want to live in.  */
static void print_rule(const struct ipt_entry *e,
    struct xtc_handle *h, const char *chain, int counters)
{
  //struct ipt_entry_target *t;
  const char *target_name;

  /* print counters */
  if (counters)
    printf("[%llu:%llu] ", e->counters.pcnt, e->counters.bcnt);

  /* print chain name */
  printf("-A %s ", chain);

  /* Print IP part. */
  print_ip("-s", e->ip.src.s_addr,e->ip.smsk.s_addr,
      e->ip.invflags & IPT_INV_SRCIP);

  print_ip("-d", e->ip.dst.s_addr, e->ip.dmsk.s_addr,
      e->ip.invflags & IPT_INV_DSTIP);

  print_iface('i', e->ip.iniface, e->ip.iniface_mask,
        e->ip.invflags & IPT_INV_VIA_IN);

  print_iface('o', e->ip.outiface, e->ip.outiface_mask,
        e->ip.invflags & IPT_INV_VIA_OUT);

  print_proto(e->ip.proto, e->ip.invflags & IPT_INV_PROTO);

  if (e->ip.flags & IPT_F_FRAG)
    printf("%s-f ",
           e->ip.invflags & IPT_INV_FRAG ? "! " : "");

  /* Print matchinfo part */
  if (e->target_offset) {
    IPT_MATCH_ITERATE(e, print_match, &e->ip);
  }
 printf(" [e->target_offset:%d] ", e->target_offset);

  /* Print target name */
  target_name = iptc_get_target(e, h);
  if (target_name && (*target_name != '\0'))
    printf("-j %s ", target_name);

  struct ipt_entry_match *match;
  match = (struct ipt_entry_match*)e->elems;

   printf(" <<%s>> ", match->u.user.name);

  /* Print targinfo part */
/*
  t = ipt_get_target((struct ipt_entry *)e);
  if (t->u.user.name[0]) {
    struct iptables_target *target
      = find_target(t->u.user.name, TRY_LOAD);

    if (!target) {
      fprintf(stderr, "Can't find library for target `%s'\n",
        t->u.user.name);
      exit(1);
         }
   if (target->save)
      target->save(&e->ip, t);
    else {
      // If the target size is greater than ipt_entry_target
      // there is something to be saved, we just don't know
      // how to print it
      if (t->u.target_size !=
          sizeof(struct ipt_entry_target)) {
        fprintf(stderr, "Target `%s' is missing "
            "save function\n",
          t->u.user.name);
        exit(1);
      	  }
    	}
  }
  */

  printf("<<%s>>\n", e->elems);

  printf("\n");
}

void test_misc()
{
	struct xtc_handle *h;

	h = iptc_init("filter");		// "filter" "mangle" "nat", ...
	if (h == NULL) {
		printf("Init Error : %s\n", iptc_strerror(errno));
		return;
	}

	const char *chain;

	chain = iptc_first_chain(h);
	while (chain) {
		printf("Chain - %s\n", chain);
		chain = iptc_next_chain(h);
	}

	const struct ipt_entry *entry;
	int cnt = 0;

	entry = iptc_first_rule("INPUT", h);
	while (entry) {
		print_rule(entry, h, "INPUT", cnt);
		cnt++;
		entry = iptc_next_rule(entry, h);
	}


	if (h)
		iptc_free(h);
}

void test_iptc()
{
	unsigned int a, b;

	inet_pton(AF_INET, "1.2.3.4", &a);
	inet_pton(AF_INET, "5.6.7.8", &b);

//	insert_rule("filter", "INPUT", a, 0, b, 1, "DROP");
//		// sames iptables -t filter -A INPUT -s 1.2.3.4/32 !-d 5.6.7.8/32 -j DROP

	test_misc();
}

// ipset
