/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "BtPieceMessage.h"
#include "PeerMessageUtil.h"
#include "Util.h"
#include "message.h"
#include "DlAbortEx.h"
#include "BtChokingEvent.h"
#include "BtCancelSendingPieceEvent.h"
#include "MessageDigestHelper.h"
#include "DiskAdaptor.h"
#include "Logger.h"
#include "Peer.h"
#include "Piece.h"
#include "BtContext.h"
#include "PieceStorage.h"
#include "BtMessageDispatcher.h"
#include "BtMessageFactory.h"
#include "BtRequestFactory.h"
#include "PeerConnection.h"
#include <cstring>

namespace aria2 {

void BtPieceMessage::setBlock(const unsigned char* block, size_t blockLength) {
  delete [] this->block;
  this->blockLength = blockLength;
  this->block = new unsigned char[this->blockLength];
  memcpy(this->block, block, this->blockLength);
}

BtPieceMessageHandle BtPieceMessage::create(const unsigned char* data, size_t dataLength) {
  if(dataLength <= 9) {
    throw new DlAbortEx(EX_INVALID_PAYLOAD_SIZE, "piece", dataLength, 9);
  }
  uint8_t id = PeerMessageUtil::getId(data);
  if(id != ID) {
    throw new DlAbortEx(EX_INVALID_BT_MESSAGE_ID, id, "piece", ID);
  }
  BtPieceMessageHandle message = new BtPieceMessage();
  message->setIndex(PeerMessageUtil::getIntParam(data, 1));
  message->setBegin(PeerMessageUtil::getIntParam(data, 5));
  message->setBlock(data+9, dataLength-9);
  return message;
}

void BtPieceMessage::doReceivedAction() {
  RequestSlot slot = dispatcher->getOutstandingRequest(index,
						       begin,
						       blockLength);
  peer->updateDownloadLength(blockLength);
  if(!RequestSlot::isNull(slot)) {
    peer->snubbing(false);
    peer->updateLatency(slot.getLatencyInMillis());
    PieceHandle piece = pieceStorage->getPiece(index);
    off_t offset = (off_t)index*btContext->getPieceLength()+begin;
    logger->debug(MSG_PIECE_RECEIVED,
		  cuid, index, begin, blockLength, offset, slot.getBlockIndex());
    pieceStorage->getDiskAdaptor()->writeData(block, blockLength, offset);
    piece->completeBlock(slot.getBlockIndex());
    logger->debug(MSG_PIECE_BITFIELD, cuid,
		  Util::toHex(piece->getBitfield(),
			      piece->getBitfieldLength()).c_str());
    dispatcher->removeOutstandingRequest(slot);
    if(piece->pieceComplete()) {
      if(checkPieceHash(piece)) {
	onNewPiece(piece);
      } else {
	onWrongPiece(piece);
      }
    }
  }
}

size_t BtPieceMessage::MESSAGE_HEADER_LENGTH = 13;

const unsigned char* BtPieceMessage::getMessageHeader() {
  if(!msgHeader) {
    /**
     * len --- 9+blockLength, 4bytes
     * id --- 7, 1byte
     * index --- index, 4bytes
     * begin --- begin, 4bytes
     * total: 13bytes
     */
    msgHeader = new unsigned char[MESSAGE_HEADER_LENGTH];
    PeerMessageUtil::createPeerMessageString(msgHeader, MESSAGE_HEADER_LENGTH,
					     9+blockLength, ID);
    PeerMessageUtil::setIntParam(&msgHeader[5], index);
    PeerMessageUtil::setIntParam(&msgHeader[9], begin);
  }
  return msgHeader;
}

size_t BtPieceMessage::getMessageHeaderLength() {
  return MESSAGE_HEADER_LENGTH;
}

void BtPieceMessage::send() {
  if(invalidate) {
    return;
  }
  if(!headerSent) {
    if(!sendingInProgress) {
      logger->info(MSG_SEND_PEER_MESSAGE,
		   cuid, peer->ipaddr.c_str(), peer->port,
		   toString().c_str());
      getMessageHeader();
      leftDataLength = getMessageHeaderLength();
      sendingInProgress = true;
    }
    size_t writtenLength
      = peerConnection->sendMessage(msgHeader+getMessageHeaderLength()-leftDataLength,
				    leftDataLength);
    if(writtenLength == leftDataLength) {
      headerSent = true;
      leftDataLength = blockLength;
    } else {
      leftDataLength -= writtenLength;
    }
  }
  if(headerSent) {
    sendingInProgress = false;
    off_t pieceDataOffset =
      (off_t)index*btContext->getPieceLength()+begin+blockLength-leftDataLength;
    size_t writtenLength = sendPieceData(pieceDataOffset, leftDataLength);
    peer->updateUploadLength(writtenLength);
    if(writtenLength < leftDataLength) {
      sendingInProgress = true;
    }
    leftDataLength -= writtenLength;
  }
}

size_t BtPieceMessage::sendPieceData(off_t offset, size_t length) const {
  size_t BUF_SIZE = 256;
  unsigned char buf[BUF_SIZE];
  div_t res = div(length, BUF_SIZE);
  size_t writtenLength = 0;
  for(int i = 0; i < res.quot; i++) {
    if((size_t)pieceStorage->getDiskAdaptor()->readData(buf, BUF_SIZE, offset+i*BUF_SIZE) < BUF_SIZE) {
      throw new DlAbortEx(EX_DATA_READ);
    }
    size_t ws = peerConnection->sendMessage(buf, BUF_SIZE);
    writtenLength += ws;
    if(ws != BUF_SIZE) {
      return writtenLength;
    }
  }
  if(res.rem > 0) {
    if(pieceStorage->getDiskAdaptor()->readData(buf, res.rem, offset+res.quot*BUF_SIZE) < res.rem) {
      throw new DlAbortEx(EX_DATA_READ);
    }
    size_t ws = peerConnection->sendMessage(buf, res.rem);
    writtenLength += ws;
  }
  return writtenLength;
}

std::string BtPieceMessage::toString() const {
  return "piece index="+Util::itos(index)+", begin="+Util::itos(begin)+
    ", length="+Util::itos(blockLength);
}

bool BtPieceMessage::checkPieceHash(const PieceHandle& piece) {
  off_t offset = (off_t)piece->getIndex()*btContext->getPieceLength();
  
  return MessageDigestHelper::staticSHA1Digest(pieceStorage->getDiskAdaptor(), offset, piece->getLength())
    == btContext->getPieceHash(piece->getIndex());
}

void BtPieceMessage::onNewPiece(const PieceHandle& piece) {
  logger->info(MSG_GOT_NEW_PIECE, cuid, piece->getIndex());
  pieceStorage->completePiece(piece);
  pieceStorage->advertisePiece(cuid, piece->getIndex());
}

void BtPieceMessage::onWrongPiece(const PieceHandle& piece) {
  logger->info(MSG_GOT_WRONG_PIECE, cuid, piece->getIndex());
  erasePieceOnDisk(piece);
  piece->clearAllBlock();
  requestFactory->removeTargetPiece(piece);
}

void BtPieceMessage::erasePieceOnDisk(const PieceHandle& piece) {
  size_t BUFSIZE = 4096;
  unsigned char buf[BUFSIZE];
  memset(buf, 0, BUFSIZE);
  off_t offset = (off_t)piece->getIndex()*btContext->getPieceLength();
  div_t res = div(piece->getLength(), BUFSIZE);
  for(int i = 0; i < res.quot; i++) {
    pieceStorage->getDiskAdaptor()->writeData(buf, BUFSIZE, offset);
    offset += BUFSIZE;
  }
  if(res.rem > 0) {
    pieceStorage->getDiskAdaptor()->writeData(buf, res.rem, offset);
  }
}

bool BtPieceMessage::BtChokingEventListener::canHandle(const BtEventHandle& event) {
  BtChokingEvent* intEvent = dynamic_cast<BtChokingEvent*>(event.get());
  return intEvent != 0;
}

void BtPieceMessage::BtChokingEventListener::handleEventInternal(const BtEventHandle& event) {
  message->handleChokingEvent(event);
}

void BtPieceMessage::handleChokingEvent(const BtEventHandle& event) {
  if(!invalidate &&
     !sendingInProgress &&
     !peer->isInAmAllowedIndexSet(index)) {
    logger->debug(MSG_REJECT_PIECE_CHOKED,
		  cuid,
		  index,
		  begin,
		  blockLength);

    if(peer->isFastExtensionEnabled()) {
      BtMessageHandle rej = messageFactory->createRejectMessage(index,
								begin,
								blockLength);
      dispatcher->addMessageToQueue(rej);
    }
    invalidate = true;
  }
}

bool BtPieceMessage::BtCancelSendingPieceEventListener::canHandle(const BtEventHandle& event) {
  BtCancelSendingPieceEvent* intEvent = dynamic_cast<BtCancelSendingPieceEvent*>(event.get());
  return intEvent != 0;
}

void BtPieceMessage::BtCancelSendingPieceEventListener::handleEventInternal(const BtEventHandle& event) {
  message->handleCancelSendingPieceEvent(event);
}

void BtPieceMessage::handleCancelSendingPieceEvent(const BtEventHandle& event) {
  BtCancelSendingPieceEvent* intEvent = (BtCancelSendingPieceEvent*)(event.get());
  if(!invalidate &&
     !sendingInProgress &&
     index == intEvent->getIndex() &&
     begin == intEvent->getBegin() &&
     blockLength == intEvent->getLength()) {
    logger->debug(MSG_REJECT_PIECE_CANCEL,
		  cuid, index, begin, blockLength);
    if(peer->isFastExtensionEnabled()) {
      BtMessageHandle rej = messageFactory->createRejectMessage(index,
								begin,
								blockLength);
      dispatcher->addMessageToQueue(rej);
    }
    invalidate = true;
  } 
}

} // namespace aria2
