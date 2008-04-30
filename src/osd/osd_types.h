// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef __OSD_TYPES_H
#define __OSD_TYPES_H

#include "msg/msg_types.h"
#include "include/types.h"
#include "include/pobject.h"

/* osdreqid_t - caller name + incarnation# + tid to unique identify this request
 * use for metadata and osd ops.
 */
struct osd_reqid_t {
  entity_name_t name; // who
  tid_t         tid;
  int32_t       inc;  // incarnation
  osd_reqid_t() : tid(0), inc(0) {}
  osd_reqid_t(const entity_name_t& a, int i, tid_t t) : name(a), tid(t), inc(i) {}
};

inline ostream& operator<<(ostream& out, const osd_reqid_t& r) {
  return out << r.name << "." << r.inc << ":" << r.tid;
}

inline bool operator==(const osd_reqid_t& l, const osd_reqid_t& r) {
  return (l.name == r.name) && (l.inc == r.inc) && (l.tid == r.tid);
}
inline bool operator!=(const osd_reqid_t& l, const osd_reqid_t& r) {
  return (l.name != r.name) || (l.inc != r.inc) || (l.tid != r.tid);
}
inline bool operator<(const osd_reqid_t& l, const osd_reqid_t& r) {
  return (l.name < r.name) || (l.inc < r.inc) || 
    (l.name == r.name && l.inc == r.inc && l.tid < r.tid);
}
inline bool operator<=(const osd_reqid_t& l, const osd_reqid_t& r) {
  return (l.name < r.name) || (l.inc < r.inc) ||
    (l.name == r.name && l.inc == r.inc && l.tid <= r.tid);
}
inline bool operator>(const osd_reqid_t& l, const osd_reqid_t& r) { return !(l <= r); }
inline bool operator>=(const osd_reqid_t& l, const osd_reqid_t& r) { return !(l < r); }

namespace __gnu_cxx {
  template<> struct hash<osd_reqid_t> {
    size_t operator()(const osd_reqid_t &r) const { 
      static blobhash H;
      return H((const char*)&r, sizeof(r));
    }
  };
}



// osd types
typedef uint64_t coll_t;        // collection id

// pg stuff

typedef uint16_t ps_t;

#define OSD_METADATA_PG_POOL 0xff
#define OSD_SUPERBLOCK_POBJECT pobject_t(OSD_METADATA_PG_POOL, 0, object_t(0,0))

// placement group id
struct pg_t {
public:
  static const int TYPE_REP   = CEPH_PG_TYPE_REP;
  static const int TYPE_RAID4 = CEPH_PG_TYPE_RAID4;

  //private:
  union ceph_pg u;

public:
  pg_t() { u.pg64 = 0; }
  pg_t(const pg_t& o) { u.pg64 = o.u.pg64; }
  pg_t(int type, int size, ps_t seed, int pool, int pref) {
    u.pg64 = 0;
    u.pg.type = type;
    u.pg.size = size;
    u.pg.ps = seed;
    u.pg.pool = pool;
    u.pg.preferred = pref;   // hack: avoid negative.
    assert(sizeof(u.pg) == sizeof(u.pg64));
  }
  pg_t(uint64_t v) { u.pg64 = v; }
  pg_t(const ceph_pg& cpg) {
    u = cpg;
  }

  int type()      { return u.pg.type; }
  bool is_rep()   { return type() == TYPE_REP; }
  bool is_raid4() { return type() == TYPE_RAID4; }

  int size() { return u.pg.size; }
  ps_t ps() { return u.pg.ps; }
  int pool() { return u.pg.pool; }
  int preferred() { return u.pg.preferred; }   // hack: avoid negative.
  
  /*
  pg_t operator=(uint64_t v) { u.val = v; return *this; }
  pg_t operator&=(uint64_t v) { u.val &= v; return *this; }
  pg_t operator+=(pg_t o) { u.val += o.val; return *this; }
  pg_t operator-=(pg_t o) { u.val -= o.val; return *this; }
  pg_t operator++() { ++u.val; return *this; }
  */
  operator uint64_t() const { return u.pg64; }

  pobject_t to_pobject() const { 
    return pobject_t(OSD_METADATA_PG_POOL,   // osd metadata 
		     0,
		     object_t(u.pg64, 0));
  }
} __attribute__ ((packed));

inline ostream& operator<<(ostream& out, pg_t pg) 
{
  if (pg.is_rep()) 
    out << pg.size() << 'x';
  else if (pg.is_raid4()) 
    out << pg.size() << 'r';
  else 
    out << pg.size() << '?';
  
  out << hex << pg.ps() << dec;

  if (pg.pool() > 0)
    out << 'v' << pg.pool();
  if (pg.preferred() >= 0)
    out << 'p' << pg.preferred();

  //out << "=" << hex << (__uint64_t)pg << dec;
  return out;
}

namespace __gnu_cxx {
  template<> struct hash< pg_t >
  {
    size_t operator()( const pg_t& x ) const
    {
      static rjhash<uint64_t> H;
      return H(x);
    }
  };
}





inline ostream& operator<<(ostream& out, const ceph_object_layout &ol)
{
  out << pg_t(ol.ol_pgid);
  int su = ol.ol_stripe_unit;
  if (su)
    out << ".su=" << su;
  return out;
}



// compound rados version type
class eversion_t {
public:
  version_t version;
  epoch_t epoch;
  eversion_t() : version(0), epoch(0) {}
  eversion_t(epoch_t e, version_t v) : version(v), epoch(e) {}

  eversion_t(const ceph_eversion& ce) : 
    version(ce.version),
    epoch(ce.epoch) {}
  operator ceph_eversion() {
    ceph_eversion c;
    c.epoch = epoch;
    c.version = version;
    return c;
  }
} __attribute__ ((packed));

inline bool operator==(const eversion_t& l, const eversion_t& r) {
  return (l.epoch == r.epoch) && (l.version == r.version);
}
inline bool operator!=(const eversion_t& l, const eversion_t& r) {
  return (l.epoch != r.epoch) || (l.version != r.version);
}
inline bool operator<(const eversion_t& l, const eversion_t& r) {
  return (l.epoch == r.epoch) ? (l.version < r.version):(l.epoch < r.epoch);
}
inline bool operator<=(const eversion_t& l, const eversion_t& r) {
  return (l.epoch == r.epoch) ? (l.version <= r.version):(l.epoch <= r.epoch);
}
inline bool operator>(const eversion_t& l, const eversion_t& r) {
  return (l.epoch == r.epoch) ? (l.version > r.version):(l.epoch > r.epoch);
}
inline bool operator>=(const eversion_t& l, const eversion_t& r) {
  return (l.epoch == r.epoch) ? (l.version >= r.version):(l.epoch >= r.epoch);
}
inline ostream& operator<<(ostream& out, const eversion_t e) {
  return out << e.epoch << "'" << e.version;
}



/** osd_stat
 * aggregate stats for an osd
 */
struct osd_stat_t {
  int64_t num_blocks;
  int64_t num_blocks_avail;
  int64_t num_objects;

  osd_stat_t() : num_blocks(0), num_blocks_avail(0), num_objects(0) {}
};



/*
 * pg states
 */
#define PG_STATE_CREATING   1  // creating
#define PG_STATE_ACTIVE     2  // i am active.  (primary: replicas too)
#define PG_STATE_CLEAN      4  // peers are complete, clean of stray replicas.
#define PG_STATE_CRASHED    8  // all replicas went down. 
#define PG_STATE_REPLAY    16  // crashed, waiting for replay
#define PG_STATE_STRAY     32  // i must notify the primary i exist.
#define PG_STATE_SPLITTING 64  // i am splitting

static inline std::string pg_state_string(int state) {
  std::string st;
  if (state & PG_STATE_CREATING) st += "creating+";
  if (state & PG_STATE_ACTIVE) st += "active+";
  if (state & PG_STATE_CLEAN) st += "clean+";
  if (state & PG_STATE_CRASHED) st += "crashed+";
  if (state & PG_STATE_REPLAY) st += "replay+";
  if (state & PG_STATE_STRAY) st += "stray+";
  if (state & PG_STATE_SPLITTING) st += "splitting+";
  if (!st.length()) 
    st = "inactive";
  else 
    st.resize(st.length()-1);
  return st;
}

/** pg_stat
 * aggregate stats for a single PG.
 */
struct pg_stat_t {
  eversion_t reported;
  epoch_t created;
  pg_t    parent;
  int32_t parent_split_bits;
  int32_t state;
  int64_t num_bytes;    // in bytes
  int64_t num_blocks;   // in 4k blocks
  int64_t num_objects;
  
  pg_stat_t() : created(0), parent_split_bits(0), state(0), num_bytes(0), num_blocks(0), num_objects(0) {}
} __attribute__ ((packed));

typedef struct ceph_osd_peer_stat osd_peer_stat_t;

inline ostream& operator<<(ostream& out, const osd_peer_stat_t &stat) {
  return out << "stat(" << stat.stamp
    //<< " oprate=" << stat.oprate
    //	     << " qlen=" << stat.qlen 
    //	     << " recent_qlen=" << stat.recent_qlen
	     << " rdlat=" << stat.read_latency_mine << " / " << stat.read_latency
	     << " fshedin=" << stat.frac_rd_ops_shed_in
	     << ")";
}

// -----------------------------------------

class ObjectExtent {
 public:
  object_t    oid;       // object id
  off_t       start;     // in object
  size_t      length;    // in object

  ceph_object_layout layout;   // object layout (pgid, etc.)

  map<size_t, size_t>  buffer_extents;  // off -> len.  extents in buffer being mapped (may be fragmented bc of striping!)
  
  ObjectExtent() : start(0), length(0) {}
  ObjectExtent(object_t o, off_t s=0, size_t l=0) : oid(o), start(s), length(l) { }
};

inline ostream& operator<<(ostream& out, ObjectExtent &ex)
{
  return out << "extent(" 
             << ex.oid << " in " << ex.layout
             << " " << ex.start << "~" << ex.length
             << ")";
}



// ---------------------------------------

class OSDSuperblock {
public:
  const static uint64_t MAGIC = 0xeb0f505dULL;
  uint64_t magic;
  ceph_fsid fsid;
  int32_t whoami;    // my role in this fs.
  epoch_t current_epoch;             // most recent epoch
  epoch_t oldest_map, newest_map;    // oldest/newest maps we have.
  double weight;
  OSDSuperblock(int w=0) : 
    magic(MAGIC), whoami(w), 
    current_epoch(0), oldest_map(0), newest_map(0), weight(0) {
    memset(&fsid, 0, sizeof(fsid));
  }
};

inline ostream& operator<<(ostream& out, OSDSuperblock& sb)
{
  return out << "sb(fsid " << sb.fsid
             << " osd" << sb.whoami
             << " e" << sb.current_epoch
             << " [" << sb.oldest_map << "," << sb.newest_map
             << "])";
}


#endif
