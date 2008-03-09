#include "BtHaveNoneMessage.h"
#include "PeerMessageUtil.h"
#include "Peer.h"
#include <cstring>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class BtHaveNoneMessageTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BtHaveNoneMessageTest);
  CPPUNIT_TEST(testCreate);
  CPPUNIT_TEST(testGetMessage);
  CPPUNIT_TEST(testDoReceivedAction);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {
  }

  void testCreate();
  void testGetMessage();
  void testDoReceivedAction();
  void testToString();
};


CPPUNIT_TEST_SUITE_REGISTRATION(BtHaveNoneMessageTest);

void BtHaveNoneMessageTest::testCreate() {
  unsigned char msg[5];
  PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 1, 15);
  SharedHandle<BtHaveNoneMessage> pm = BtHaveNoneMessage::create(&msg[4], 1);
  CPPUNIT_ASSERT_EQUAL((uint8_t)15, pm->getId());

  // case: payload size is wrong
  try {
    unsigned char msg[6];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 2, 15);
    BtHaveNoneMessage::create(&msg[4], 2);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
  // case: id is wrong
  try {
    unsigned char msg[5];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 1, 16);
    BtHaveNoneMessage::create(&msg[4], 1);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
}

void BtHaveNoneMessageTest::testGetMessage() {
  BtHaveNoneMessage msg;
  unsigned char data[5];
  PeerMessageUtil::createPeerMessageString(data, sizeof(data), 1, 15);
  CPPUNIT_ASSERT(memcmp(msg.getMessage(), data, 5) == 0);
}

void BtHaveNoneMessageTest::testDoReceivedAction() {
  BtHaveNoneMessage msg;
  SharedHandle<Peer> peer = new Peer("host", 6969);
  peer->allocateSessionResource(1024, 1024*1024);
  peer->setFastExtensionEnabled(true);
  msg.setPeer(peer);
  msg.doReceivedAction();

  peer->setFastExtensionEnabled(false);
  try {
    msg.doReceivedAction();
    CPPUNIT_FAIL("an exception must be thrown.");
  } catch(...) {}
}

void BtHaveNoneMessageTest::testToString() {
  BtHaveNoneMessage msg;
  CPPUNIT_ASSERT_EQUAL(std::string("have none"), msg.toString());
}

} // namespace aria2
