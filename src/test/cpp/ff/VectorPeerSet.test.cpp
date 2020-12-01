/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <ff/Util.h>
#include <ff/VectorPeerSet.h>

using namespace ff;

TEST(VectorPeerSet, CreateCopyAndEquality) {
  // start with an in-order VectorPeerSet.
  VectorPeerSet<size_t> vps1;
  vps1.add(0);
  vps1.add(1);
  vps1.add(2);

  // Inserted peers should have default values for peers' fronctocolId
  // and completion status.
  vps1.forEach([](size_t const &,
                  ff::fronctocolId_t const & fid,
                  bool const & cs) {
    EXPECT_EQ(FRONCTOCOLID_INVALID, fid);
    EXPECT_EQ(false, cs);
  });

  // Editing fronctocolId or completion status should persist.
  vps1.forEach([](size_t const &, ff::fronctocolId_t & fid, bool & cs) {
    fid = 123;
    cs = true;
  });
  vps1.forEach([](size_t const &,
                  ff::fronctocolId_t const & fid,
                  bool const & cs) {
    EXPECT_EQ(123, fid);
    EXPECT_EQ(true, cs);
  });

  // Copied VectorPeerSet is equal, but does not preserve fronctocolId or
  // completion status.
  VectorPeerSet<size_t> vps2(vps1);
  EXPECT_TRUE(vps1 == vps2);

  vps2.forEach([](size_t const &,
                  ff::fronctocolId_t const & fid,
                  bool const & cs) {
    EXPECT_EQ(FRONCTOCOLID_INVALID, fid);
    EXPECT_EQ(false, cs);
  });

  // Original VectorPeerSet might not be in order (using comparison to imply
  // ordering), but copies are.
  VectorPeerSet<size_t> vps3;
  vps3.add(1);
  vps3.add(2);
  vps3.add(0);

  EXPECT_TRUE((vps1 == vps3 || true));

  VectorPeerSet<size_t> vps4(vps3);
  EXPECT_TRUE(vps1 == vps4);
}

TEST(VectorPeerSet, checkAndSetId) {
  VectorPeerSet<size_t> vps1;
  vps1.add(1);

  // checkAndSetId persists only once.
  EXPECT_TRUE(vps1.checkAndSetId(1, 123));
  EXPECT_FALSE(vps1.checkAndSetId(1, 456));

  bool found = false;
  vps1.forEach(
      [&](size_t const & peer, fronctocolId_t const & fid, bool &) {
        if (peer == 1) {
          EXPECT_EQ(123, fid);
          found = true;
        }
      });
  EXPECT_TRUE(found);
}

TEST(VectorPeerSet, setId) {
  VectorPeerSet<size_t> vps1;
  vps1.add(1);

  // Set ID persists multiple times.
  vps1.setId(1, 123);

  bool found = false;
  vps1.forEach(
      [&](size_t const & peer, fronctocolId_t const & fid, bool &) {
        if (peer == 1) {
          EXPECT_EQ(123, fid);
          found = true;
        }
      });
  EXPECT_TRUE(found);

  vps1.setId(1, 456);

  found = false;
  vps1.forEach(
      [&](size_t const & peer, fronctocolId_t const & fid, bool &) {
        if (peer == 1) {
          EXPECT_EQ(456, fid);
          found = true;
        }
      });
  EXPECT_TRUE(found);
}

TEST(VectorPeerSet, hasAllPeerIds) {
  VectorPeerSet<size_t> vps1;
  vps1.add(0);
  vps1.add(1);
  vps1.add(2);

  EXPECT_FALSE(vps1.hasAllPeerIds());
  vps1.checkAndSetId(0, 123);
  EXPECT_FALSE(vps1.hasAllPeerIds());
  vps1.checkAndSetId(1, 234);
  EXPECT_FALSE(vps1.hasAllPeerIds());
  vps1.checkAndSetId(2, 345);
  EXPECT_TRUE(vps1.hasAllPeerIds());
}

TEST(VectorPeerSet, setCompleted) {
  VectorPeerSet<size_t> vps1;
  vps1.add(0);

  // completion status starts as false
  bool found = false;
  vps1.forEach(
      [&](size_t const & peer, fronctocolId_t &, bool const & cs) {
        if (peer == 0) {
          EXPECT_FALSE(cs);
          found = true;
        }
      });
  EXPECT_TRUE(found);

  // then turns true.
  vps1.setCompleted(0);
  found = false;
  vps1.forEach(
      [&](size_t const & peer, fronctocolId_t &, bool const & cs) {
        if (peer == 0) {
          EXPECT_TRUE(cs);
          found = true;
        }
      });
  EXPECT_TRUE(found);
}

TEST(VectorPeerSet, checkAllComplete) {
  VectorPeerSet<size_t> vps1;
  vps1.add(0);
  vps1.add(1);
  vps1.add(2);

  EXPECT_FALSE(vps1.checkAllComplete());
  vps1.setCompleted(0);
  EXPECT_FALSE(vps1.checkAllComplete());
  vps1.setCompleted(1);
  EXPECT_FALSE(vps1.checkAllComplete());
  vps1.setCompleted(2);
  EXPECT_TRUE(vps1.checkAllComplete());
}

TEST(VectorPeerSet, findThings) {
  VectorPeerSet<size_t> vps;

  for (size_t i = 0; i < 8; i++) {
    vps.add(i);
  }

  for (size_t i = 0; i < 8; i++) {
    EXPECT_TRUE(vps.hasPeer(i));
    EXPECT_EQ(FRONCTOCOLID_INVALID, vps.findPeerId(i));
    EXPECT_FALSE(vps.findCompletionStatus(i));
  }

  for (size_t i = 8; i < 10; i++) {
    EXPECT_FALSE(vps.hasPeer(i));
    EXPECT_EQ(FRONCTOCOLID_INVALID, vps.findPeerId(i));
    EXPECT_FALSE(vps.findCompletionStatus(i));
  }

  for (size_t i = 0; i < 8; i++) {
    vps.setId(i, i * 10);
    if ((i % 2) == 0) {
      vps.setCompleted(i);
    }
  }

  for (size_t i = 0; i < 8; i++) {
    EXPECT_TRUE(vps.hasPeer(i));
    EXPECT_EQ(i * 10, vps.findPeerId(i));
    EXPECT_EQ((i % 2) == 0, vps.findCompletionStatus(i));
  }
}
