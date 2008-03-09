#ifndef _D_MOCK_BT_MESSAGE_FACTORY_H_
#define _D_MOCK_BT_MESSAGE_FACTORY_H_

#include "BtMessageFactory.h"

namespace aria2 {

class ExtensionMessage;

class MockBtMessageFactory : public BtMessageFactory {
public:
  MockBtMessageFactory() {}

  virtual ~MockBtMessageFactory() {}

  virtual SharedHandle<BtMessage>
  createBtMessage(const unsigned char* msg, size_t msgLength) {
    return SharedHandle<BtMessage>(0);
  };

  virtual SharedHandle<BtMessage>
  createHandshakeMessage(const unsigned char* msg, size_t msgLength) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage>
  createHandshakeMessage(const unsigned char* infoHash,
			 const unsigned char* peerId) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage>
  createRequestMessage(const SharedHandle<Piece>& piece, size_t blockIndex) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage>
  createCancelMessage(size_t index, uint32_t begin, size_t length) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage>
  createPieceMessage(size_t index, uint32_t begin, size_t length) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createHaveMessage(size_t index) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createChokeMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createUnchokeMessage() {
    return SharedHandle<BtMessage>(0);
  }
  
  virtual SharedHandle<BtMessage> createInterestedMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createNotInterestedMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createBitfieldMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createKeepAliveMessage() {
    return SharedHandle<BtMessage>(0);
  }
  
  virtual SharedHandle<BtMessage> createHaveAllMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createHaveNoneMessage() {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage>
  createRejectMessage(size_t index, uint32_t begin, size_t length) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createAllowedFastMessage(size_t index) {
    return SharedHandle<BtMessage>(0);
  }

  virtual SharedHandle<BtMessage> createPortMessage(uint16_t port)
  {
    return SharedHandle<BtMessage>(0);
  }
  
  virtual SharedHandle<BtMessage>
  createBtExtendedMessage(const SharedHandle<ExtensionMessage>& extmsg)
  {
    return SharedHandle<BtMessage>(0);
  }
};

} // namespace aria2

#endif // _D_MOCK_BT_MESSAGE_FACTORY_H_
