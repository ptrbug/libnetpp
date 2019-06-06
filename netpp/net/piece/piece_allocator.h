#pragma once
#include <stdint.h>

namespace netpp {

	const uint16_t kPieceCapacity = 1024 * 4;
	struct Piece
	{
		Piece* next;
		uint16_t off;
		uint16_t len;
		char data[kPieceCapacity];
	};

	Piece* newPiece();
	void deletePiece(Piece* item);
}