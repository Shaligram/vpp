/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef included_dns_h
#define included_dns_h

#include <vppinfra/time.h>
#include <vppinfra/cache.h>
#include <vppinfra/error.h>

#include <vppinfra/hash.h>
#include <vnet/dns/dns_packet.h>
#include <vnet/ip/ip.h>

typedef struct
{
  u32 request_type;
  u32 client_index;
  u32 client_context;
} pending_api_request_t;

#define DNS_API_PENDING_NAME_TO_IP 1
#define DNS_API_PENDING_IP_TO_NAME 2

typedef struct
{
  /** flags */
  volatile u8 flags;

  /** The name in "normal human being" notation, e.g. www.foobar.com */
  u8 *name;

  /** For CNAME records, the "next name" to resolve */
  u8 *cname;

  /** Expiration time */
  f64 expiration_time;

  /** Cached dns request, for sending retries */
  u8 *dns_request;

  /** Retry parameters */
  int retry_count;
  int server_rotor;
  int server_af;
  f64 retry_timer;

  /** Cached dns response */
  u8 *dns_response;

  /** Clients awaiting responses */
  pending_api_request_t *pending_api_requests;
  ip4_address_t *ip4_peers_to_notify;
  ip6_address_t *ip6_peers_to_notify;
} dns_cache_entry_t;

#define DNS_CACHE_ENTRY_FLAG_VALID	(1<<0) /**< we have Actual Data */
#define DNS_CACHE_ENTRY_FLAG_STATIC	(1<<1) /**< static entry */
#define DNS_CACHE_ENTRY_FLAG_CNAME	(1<<2) /**< CNAME (indirect) entry */

#define DNS_RETRIES_PER_SERVER 3

#define DNS_RESOLVER_EVENT_RESOLVED	1
#define DNS_RESOLVER_EVENT_PENDING	2


typedef struct
{
  /** Pool of cache entries */
  dns_cache_entry_t *entries;

  /** Pool indices of unresolved entries */
  u32 *unresolved_entries;

  /** Find cached record by name */
  uword *cache_entry_by_name;
  uword *cache_lock;

  /** enable / disable flag */
  int is_enabled;

  /** upstream name servers, e.g. 8.8.8.8 */
  ip4_address_t *ip4_name_servers;
  ip6_address_t *ip6_name_servers;

  /** config parameters */
  u32 name_cache_size;
  u32 max_ttl_in_seconds;
  u32 random_seed;

  /* convenience */
  vlib_main_t *vlib_main;
  vnet_main_t *vnet_main;
} dns_main_t;

extern dns_main_t dns_main;

extern vlib_node_registration_t dns46_reply_node;
extern vlib_node_registration_t dns_resolver_node;

#define foreach_dns46_reply_error                       \
_(PROCESSED, "DNS reply pkts processed")                \
_(NO_ELT, "No DNS pool element")                        \
_(FORMAT_ERROR, "DNS format errors")                    \
_(TEST_DROP, "DNS reply pkt dropped for test purposes")

typedef enum
{
#define _(sym,str) DNS46_REPLY_ERROR_##sym,
  foreach_dns46_reply_error
#undef _
    DNS46_REPLY_N_ERROR,
} dns46_reply_error_t;

void vnet_send_dns_request (dns_main_t * dm, dns_cache_entry_t * ep);
int
vnet_dns_cname_indirection_nolock (dns_main_t * dm, u32 ep_index, u8 * reply);

int vnet_dns_delete_entry_by_index_nolock (dns_main_t * dm, u32 index);

format_function_t format_dns_reply;

static inline void
dns_cache_lock (dns_main_t * dm)
{
  if (dm->cache_lock)
    {
      while (__sync_lock_test_and_set (dm->cache_lock, 1))
	;
    }
}

static inline void
dns_cache_unlock (dns_main_t * dm)
{
  if (dm->cache_lock)
    {
      CLIB_MEMORY_BARRIER ();
      *dm->cache_lock = 0;
    }
}

#endif /* included_dns_h */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
