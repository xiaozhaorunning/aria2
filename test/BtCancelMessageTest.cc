#include "BtCancelMessage.h"
#include "PeerMessageUtil.h"
#include "MockBtMessageDispatcher.h"
#include "MockBtContext.h"
#include "Peer.h"
#include "FileEntry.h"
#include "PeerObject.h"
#include "Piece.h"
#include "BtRegistry.h"
#include "BtMessageFactory.h"
#include "BtRequestFactory.h"
#include "BtMessageReceiver.h"
#include "PeerConnection.h"
#include "ExtensionMessageFactory.h"
#include <cstring>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class BtCancelMessageTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BtCancelMessageTest);
  CPPUNIT_TEST(testCreate);
  CPPUNIT_TEST(testGetMessage);
  CPPUNIT_TEST(testDoReceivedAction);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  BtCancelMessageTest():peer(0), btContext(0) {}

  SharedHandle<Peer> peer;
  SharedHandle<MockBtContext> btContext;

  void setUp() {
    BtRegistry::unregisterAll();    
    peer = new Peer("host", 6969);
    btContext = new MockBtContext();
    btContext->setInfoHash((const unsigned char*)"12345678901234567890");
    BtRegistry::registerPeerObjectCluster(btContext->getInfoHashAsString(),
					  new PeerObjectCluster());
    PEER_OBJECT_CLUSTER(btContext)->registerHandle(peer->getID(), new PeerObject());
  }

  void testCreate();
  void testGetMessage();
  void testDoReceivedAction();

  class MockBtMessageDispatcher2 : public MockBtMessageDispatcher {
  public:
    size_t index;
    uint32_t begin;
    size_t length;
  public:
    MockBtMessageDispatcher2():index(0),
			       begin(0),
			       length(0) {}

    virtual void doCancelSendingPieceAction(size_t index, uint32_t begin, size_t length) {
      this->index = index;
      this->begin = begin;
      this->length = length;
    }
  };
};


CPPUNIT_TEST_SUITE_REGISTRATION(BtCancelMessageTest);

void BtCancelMessageTest::testCreate() {
  unsigned char msg[17];
  PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 13, 8);
  PeerMessageUtil::setIntParam(&msg[5], 12345);
  PeerMessageUtil::setIntParam(&msg[9], 256);
  PeerMessageUtil::setIntParam(&msg[13], 1024);
  SharedHandle<BtCancelMessage> pm = BtCancelMessage::create(&msg[4], 13);
  CPPUNIT_ASSERT_EQUAL((uint8_t)8, pm->getId());
  CPPUNIT_ASSERT_EQUAL((size_t)12345, pm->getIndex());
  CPPUNIT_ASSERT_EQUAL((uint32_t)256, pm->getBegin());
  CPPUNIT_ASSERT_EQUAL((size_t)1024, pm->getLength());

  // case: payload size is wrong
  try {
    unsigned char msg[18];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 14, 8);
    BtCancelMessage::create(&msg[4], 14);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
  // case: id is wrong
  try {
    unsigned char msg[17];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 13, 9);
    BtCancelMessage::create(&msg[4], 13);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
}

void BtCancelMessageTest::testGetMessage() {
  BtCancelMessage msg;
  msg.setIndex(12345);
  msg.setBegin(256);
  msg.setLength(1024);
  unsigned char data[17];
  PeerMessageUtil::createPeerMessageString(data, sizeof(data), 13, 8);
  PeerMessageUtil::setIntParam(&data[5], 12345);
  PeerMessageUtil::setIntParam(&data[9], 256);
  PeerMessageUtil::setIntParam(&data[13], 1024);
  CPPUNIT_ASSERT(memcmp(msg.getMessage(), data, 17) == 0);
}

void BtCancelMessageTest::testDoReceivedAction() {
  BtCancelMessage msg;
  msg.setIndex(1);
  msg.setBegin(2*16*1024);
  msg.setLength(16*1024);
  msg.setBtContext(btContext);
  msg.setPeer(peer);
  SharedHandle<MockBtMessageDispatcher2> dispatcher = new MockBtMessageDispatcher2();
  msg.setBtMessageDispatcher(dispatcher);

  msg.doReceivedAction();
  CPPUNIT_ASSERT_EQUAL(msg.getIndex(), dispatcher->index);
  CPPUNIT_ASSERT_EQUAL(msg.getBegin(), dispatcher->begin);
  CPPUNIT_ASSERT_EQUAL(msg.getLength(), dispatcher->length);
}

} // namespace aria2
