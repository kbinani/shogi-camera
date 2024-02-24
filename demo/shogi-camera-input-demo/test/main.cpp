#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <shogi_camera_input/shogi_camera_input.hpp>

using namespace sci;

static Position EmptyPosition() {
  Position p;
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      p.pieces[x][y] = 0;
    }
  }
  return p;
}

static void CheckActionWhen(Position const& p, Move base, File fileFrom, Rank rankFrom, uint32_t expected) {
Move mv = base;
mv.from = MakeSquare(fileFrom, rankFrom);
  mv.decideAction(p);
  CHECK(mv.action == expected);
}

TEST_CASE("Move::decideAction") {
  // https://www.shogi.or.jp/faq/kihuhyouki.html
  Piece gold = MakePiece(Color::Black, PieceType::Gold);
  Piece silver = MakePiece(Color::Black, PieceType::Silver);
  Piece pp = MakePiece(Color::Black, PieceType::Pawn, PieceStatus::Promoted);
  Piece prook = MakePiece(Color::Black, PieceType::Rook, PieceStatus::Promoted);
  Piece pbishop = MakePiece(Color::Black, PieceType::Bishop, PieceStatus::Promoted);
  SUBCASE("寄・引・上") {
    Position p = EmptyPosition();
    // A
    p.pieces[File::File7][Rank::Rank2] = gold;
    p.pieces[File::File9][Rank::Rank3] = gold;
    // B
    p.pieces[File::File3][Rank::Rank1] = gold;
    p.pieces[File::File4][Rank::Rank3] = gold;
    // C
    p.pieces[File::File4][Rank::Rank5] = gold;
    p.pieces[File::File5][Rank::Rank6] = gold;
    // D
    p.pieces[File::File7][Rank::Rank7] = silver;
    p.pieces[File::File8][Rank::Rank9] = silver;
    // E
    p.pieces[File::File2][Rank::Rank7] = silver;
    p.pieces[File::File4][Rank::Rank9] = silver;

    SUBCASE("A") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckActionWhen(p, base, File::File9, Rank::Rank3, Action::ActionUp);
      CheckActionWhen(p, base, File::File7, Rank::Rank2, Action::ActionSideway);
    }
    SUBCASE("B") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File3, Rank::Rank2);
      CheckActionWhen(p, base, File::File4, Rank::Rank3, Action::ActionUp);
      CheckActionWhen(p, base, File::File3, Rank::Rank1, Action::ActionDown);
    }
    SUBCASE("C") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File5, Rank::Rank5);
      CheckActionWhen(p, base, File::File5, Rank::Rank6, Action::ActionUp);
      CheckActionWhen(p, base, File::File4, Rank::Rank5, Action::ActionSideway);
    }
    SUBCASE("D") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckActionWhen(p, base, File::File8, Rank::Rank9, Action::ActionUp);
      CheckActionWhen(p, base, File::File7, Rank::Rank7, Action::ActionDown);
    }
    SUBCASE("E") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File3, Rank::Rank8);
      CheckActionWhen(p, base, File::File4, Rank::Rank9, Action::ActionUp);
      CheckActionWhen(p, base, File::File2, Rank::Rank7, Action::ActionDown);
    }
  }
  SUBCASE("右・左１") {
    Position p = EmptyPosition();
    // A
    p.pieces[File::File9][Rank::Rank2] = gold;
    p.pieces[File::File7][Rank::Rank2] = gold;
    // B
    p.pieces[File::File3][Rank::Rank2] = gold;
    p.pieces[File::File1][Rank::Rank2] = gold;
    // C
    p.pieces[File::File6][Rank::Rank5] = silver;
    p.pieces[File::File4][Rank::Rank5] = silver;
    // D
    p.pieces[File::File8][Rank::Rank9] = gold;
    p.pieces[File::File7][Rank::Rank9] = gold;
    // E
    p.pieces[File::File3][Rank::Rank9] = silver;
    p.pieces[File::File2][Rank::Rank9] = silver;
    SUBCASE("A") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank1);
      CheckActionWhen(p, base, File::File9, Rank::Rank2, Action::ActionLeft);
      CheckActionWhen(p, base, File::File7, Rank::Rank2, Action::ActionRight);
    }
    SUBCASE("B") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File2, Rank::Rank2);
      CheckActionWhen(p, base, File::File3, Rank::Rank2, Action::ActionLeft);
      CheckActionWhen(p, base, File::File1, Rank::Rank2, Action::ActionRight);
    }
    SUBCASE("C") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File5, Rank::Rank6);
      CheckActionWhen(p, base, File::File6, Rank::Rank5, Action::ActionLeft);
      CheckActionWhen(p, base, File::File4, Rank::Rank5, Action::ActionRight);
    }
    SUBCASE("D") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File7, Rank::Rank8);
      CheckActionWhen(p, base, File::File8, Rank::Rank9, Action::ActionLeft);
      CheckActionWhen(p, base, File::File7, Rank::Rank9, Action::ActionNearest);
    }
    SUBCASE("E") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File3, Rank::Rank8);
      CheckActionWhen(p, base, File::File3, Rank::Rank9, Action::ActionNearest);
      CheckActionWhen(p, base, File::File2, Rank::Rank9, Action::ActionRight);
    }
  }
  SUBCASE("右・左２") {
    Position p = EmptyPosition();
    // A
    p.pieces[File::File6][Rank::Rank3] = gold;
    p.pieces[File::File5][Rank::Rank3] = gold;
    p.pieces[File::File4][Rank::Rank3] = gold;
    // B
    p.pieces[File::File8][Rank::Rank7] = pp;
    p.pieces[File::File9][Rank::Rank8] = pp;
    p.pieces[File::File9][Rank::Rank9] = pp;
    p.pieces[File::File8][Rank::Rank9] = pp;
    p.pieces[File::File7][Rank::Rank9] = pp;
    // C
    p.pieces[File::File3][Rank::Rank7] = silver;
    p.pieces[File::File1][Rank::Rank7] = silver;
    p.pieces[File::File3][Rank::Rank9] = silver;
    p.pieces[File::File2][Rank::Rank9] = silver;
    SUBCASE("A") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File5, Rank::Rank2);
      CheckActionWhen(p, base, File::File6, Rank::Rank3, Action::ActionLeft);
      CheckActionWhen(p, base, File::File5, Rank::Rank3, Action::ActionNearest);
      CheckActionWhen(p, base, File::File4, Rank::Rank3, Action::ActionRight);
    }
    SUBCASE("B") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckActionWhen(p, base, File::File7, Rank::Rank9, Action::ActionRight);
      CheckActionWhen(p, base, File::File8, Rank::Rank9, Action::ActionNearest);
      CheckActionWhen(p, base, File::File9, Rank::Rank9, Action::ActionLeft | Action::ActionUp);
      CheckActionWhen(p, base, File::File9, Rank::Rank8, Action::ActionSideway);
      CheckActionWhen(p, base, File::File8, Rank::Rank7, Action::ActionDown);
    }
    SUBCASE("C") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File2, Rank::Rank8);
      CheckActionWhen(p, base, File::File2, Rank::Rank9, Action::ActionNearest);
      CheckActionWhen(p, base, File::File1, Rank::Rank7, Action::ActionRight);
      CheckActionWhen(p, base, File::File3, Rank::Rank9, Action::ActionLeft | Action::ActionUp);
      CheckActionWhen(p, base, File::File3, Rank::Rank7, Action::ActionLeft | Action::ActionDown);
    }
  }
  SUBCASE("竜") {
    Position p = EmptyPosition();
    // A
    p.pieces[File::File9][Rank::Rank1] = prook;
    p.pieces[File::File8][Rank::Rank4] = prook;
    // B
    p.pieces[File::File5][Rank::Rank2] = prook;
    p.pieces[File::File2][Rank::Rank3] = prook;
    // C
    p.pieces[File::File5][Rank::Rank5] = prook;
    p.pieces[File::File1][Rank::Rank5] = prook;
    // D
    p.pieces[File::File9][Rank::Rank9] = prook;
    p.pieces[File::File8][Rank::Rank9] = prook;
    // E
    p.pieces[File::File2][Rank::Rank8] = prook;
    p.pieces[File::File1][Rank::Rank9] = prook;
    SUBCASE("A") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckActionWhen(p, base, File::File9, Rank::Rank1, Action::ActionDown);
      CheckActionWhen(p, base, File::File8, Rank::Rank4, Action::ActionUp);
    }
    SUBCASE("B") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File4, Rank::Rank3);
      CheckActionWhen(p, base, File::File2, Rank::Rank3, Action::ActionSideway);
      CheckActionWhen(p, base, File::File5, Rank::Rank2, Action::ActionDown);
    }
    SUBCASE("C") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File3, Rank::Rank5);
      CheckActionWhen(p, base, File::File5, Rank::Rank5, Action::ActionLeft);
      CheckActionWhen(p, base, File::File1, Rank::Rank5, Action::ActionRight);
    }
    SUBCASE("D") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckActionWhen(p, base, File::File9, Rank::Rank9, Action::ActionLeft);
      CheckActionWhen(p, base, File::File8, Rank::Rank9, Action::ActionRight);
    }
    SUBCASE("E") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File1, Rank::Rank7);
      CheckActionWhen(p, base, File::File2, Rank::Rank8, Action::ActionLeft);
      CheckActionWhen(p, base, File::File1, Rank::Rank9, Action::ActionRight);
    }
  }
  SUBCASE("馬") {
    Position p = EmptyPosition();
    // A
    p.pieces[File::File9][Rank::Rank1] = pbishop;
    p.pieces[File::File8][Rank::Rank1] = pbishop;
    // B
    p.pieces[File::File6][Rank::Rank3] = pbishop;
    p.pieces[File::File9][Rank::Rank5] = pbishop;
    // C
    p.pieces[File::File1][Rank::Rank1] = pbishop;
    p.pieces[File::File3][Rank::Rank4] = pbishop;
    // D
    p.pieces[File::File9][Rank::Rank9] = pbishop;
    p.pieces[File::File5][Rank::Rank9] = pbishop;
    // E
    p.pieces[File::File4][Rank::Rank7] = pbishop;
    p.pieces[File::File1][Rank::Rank8] = pbishop;
    SUBCASE("A") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckActionWhen(p, base, File::File9, Rank::Rank1, Action::ActionLeft);
      CheckActionWhen(p, base, File::File8, Rank::Rank1, Action::ActionRight);
    }
    SUBCASE("B") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File8, Rank::Rank5);
      CheckActionWhen(p, base, File::File9, Rank::Rank5, Action::ActionSideway);
      CheckActionWhen(p, base, File::File6, Rank::Rank3, Action::ActionDown);
    }
    SUBCASE("C") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File1, Rank::Rank2);
      CheckActionWhen(p, base, File::File1, Rank::Rank1, Action::ActionDown);
      CheckActionWhen(p, base, File::File3, Rank::Rank4, Action::ActionUp);
    }
    SUBCASE("D") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File7, Rank::Rank7);
      CheckActionWhen(p, base, File::File9, Rank::Rank9, Action::ActionLeft);
      CheckActionWhen(p, base, File::File5, Rank::Rank9, Action::ActionRight);
    }
    SUBCASE("E") {
      Move base;
      base.color = Color::Black;
      base.to = MakeSquare(File::File2, Rank::Rank9);
      CheckActionWhen(p, base, File::File4, Rank::Rank7, Action::ActionLeft);
      CheckActionWhen(p, base, File::File1, Rank::Rank8, Action::ActionRight);
    }
  }
}
