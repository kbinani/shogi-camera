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

static void CheckSuffixWhen(Position const &p, Move base, File fileFrom, Rank rankFrom, uint32_t expected) {
  Move mv = base;
  mv.from = MakeSquare(fileFrom, rankFrom);
  mv.decideSuffix(p);
  if (mv.suffix != expected) {
    static int a = 0;
    a++;
  }
  CHECK(mv.suffix == expected);
}

TEST_CASE("Move::decideSuffix") {
  // https://www.shogi.or.jp/faq/kihuhyouki.html
  Piece gold = MakePiece(Color::Black, PieceType::Gold);
  Piece silver = MakePiece(Color::Black, PieceType::Silver);
  Piece pp = MakePiece(Color::Black, PieceType::Pawn, PieceStatus::Promoted);
  Piece prook = MakePiece(Color::Black, PieceType::Rook, PieceStatus::Promoted);
  Piece pbishop = MakePiece(Color::Black, PieceType::Bishop, PieceStatus::Promoted);
  SUBCASE("寄・引・上") {
    SUBCASE("A") {
      Position p = EmptyPosition();
      p.pieces[File::File7][Rank::Rank2] = gold;
      p.pieces[File::File9][Rank::Rank3] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank3, Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File7, Rank::Rank2, Suffix::SuffixSideway);
    }
    SUBCASE("B") {
      Position p = EmptyPosition();
      p.pieces[File::File3][Rank::Rank1] = gold;
      p.pieces[File::File4][Rank::Rank3] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File3, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank3, Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank1, Suffix::SuffixDown);
    }
    SUBCASE("C") {
      Position p = EmptyPosition();
      p.pieces[File::File4][Rank::Rank5] = gold;
      p.pieces[File::File5][Rank::Rank6] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File5, Rank::Rank5);
      CheckSuffixWhen(p, base, File::File5, Rank::Rank6, Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank5, Suffix::SuffixSideway);
    }
    SUBCASE("D") {
      Position p = EmptyPosition();
      p.pieces[File::File7][Rank::Rank7] = silver;
      p.pieces[File::File8][Rank::Rank9] = silver;

      Move base;
      base.color = Color::Black;
      base.piece = silver;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank9, Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File7, Rank::Rank7, Suffix::SuffixDown);
    }
    SUBCASE("E") {
      Position p = EmptyPosition();
      p.pieces[File::File2][Rank::Rank7] = silver;
      p.pieces[File::File4][Rank::Rank9] = silver;

      Move base;
      base.color = Color::Black;
      base.piece = silver;
      base.to = MakeSquare(File::File3, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank9, Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File2, Rank::Rank7, Suffix::SuffixDown);
    }
  }
  SUBCASE("右・左１") {
    SUBCASE("A") {
      Position p = EmptyPosition();
      p.pieces[File::File9][Rank::Rank2] = gold;
      p.pieces[File::File7][Rank::Rank2] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File8, Rank::Rank1);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank2, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File7, Rank::Rank2, Suffix::SuffixRight);
    }
    SUBCASE("B") {
      Position p = EmptyPosition();
      p.pieces[File::File3][Rank::Rank2] = gold;
      p.pieces[File::File1][Rank::Rank2] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File2, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank2, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank2, Suffix::SuffixRight);
    }
    SUBCASE("C") {
      Position p = EmptyPosition();
      p.pieces[File::File6][Rank::Rank5] = silver;
      p.pieces[File::File4][Rank::Rank5] = silver;

      Move base;
      base.color = Color::Black;
      base.piece = silver;
      base.to = MakeSquare(File::File5, Rank::Rank6);
      CheckSuffixWhen(p, base, File::File6, Rank::Rank5, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank5, Suffix::SuffixRight);
    }
    SUBCASE("D") {
      Position p = EmptyPosition();
      p.pieces[File::File8][Rank::Rank9] = gold;
      p.pieces[File::File7][Rank::Rank9] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File7, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank9, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File7, Rank::Rank9, Suffix::SuffixNearest);
    }
    SUBCASE("E") {
      Position p = EmptyPosition();
      p.pieces[File::File3][Rank::Rank9] = silver;
      p.pieces[File::File2][Rank::Rank9] = silver;

      Move base;
      base.color = Color::Black;
      base.piece = silver;
      base.to = MakeSquare(File::File3, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank9, Suffix::SuffixNearest);
      CheckSuffixWhen(p, base, File::File2, Rank::Rank9, Suffix::SuffixRight);
    }
  }
  SUBCASE("右・左２") {
    SUBCASE("A") {
      Position p = EmptyPosition();
      p.pieces[File::File6][Rank::Rank3] = gold;
      p.pieces[File::File5][Rank::Rank3] = gold;
      p.pieces[File::File4][Rank::Rank3] = gold;

      Move base;
      base.color = Color::Black;
      base.piece = gold;
      base.to = MakeSquare(File::File5, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File6, Rank::Rank3, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File5, Rank::Rank3, Suffix::SuffixNearest);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank3, Suffix::SuffixRight);
    }
    SUBCASE("B") {
      Position p = EmptyPosition();
      p.pieces[File::File8][Rank::Rank7] = pp;
      p.pieces[File::File9][Rank::Rank8] = pp;
      p.pieces[File::File9][Rank::Rank9] = pp;
      p.pieces[File::File8][Rank::Rank9] = pp;
      p.pieces[File::File7][Rank::Rank9] = pp;

      Move base;
      base.color = Color::Black;
      base.piece = pp;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File7, Rank::Rank9, Suffix::SuffixRight);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank9, Suffix::SuffixNearest);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank9, Suffix::SuffixLeft | Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank8, Suffix::SuffixSideway);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank7, Suffix::SuffixDown);
    }
    SUBCASE("C") {
      Position p = EmptyPosition();
      p.pieces[File::File3][Rank::Rank7] = silver;
      p.pieces[File::File1][Rank::Rank7] = silver;
      p.pieces[File::File3][Rank::Rank9] = silver;
      p.pieces[File::File2][Rank::Rank9] = silver;

      Move base;
      base.color = Color::Black;
      base.piece = silver;
      base.to = MakeSquare(File::File2, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File2, Rank::Rank9, Suffix::SuffixNearest);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank7, Suffix::SuffixRight);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank9, Suffix::SuffixLeft | Suffix::SuffixUp);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank7, Suffix::SuffixLeft | Suffix::SuffixDown);
    }
  }
  SUBCASE("竜") {
    SUBCASE("A") {
      Position p = EmptyPosition();
      p.pieces[File::File9][Rank::Rank1] = prook;
      p.pieces[File::File8][Rank::Rank4] = prook;

      Move base;
      base.color = Color::Black;
      base.piece = prook;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank1, Suffix::SuffixDown);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank4, Suffix::SuffixUp);
    }
    SUBCASE("B") {
      Position p = EmptyPosition();
      p.pieces[File::File5][Rank::Rank2] = prook;
      p.pieces[File::File2][Rank::Rank3] = prook;

      Move base;
      base.color = Color::Black;
      base.piece = prook;
      base.to = MakeSquare(File::File4, Rank::Rank3);
      CheckSuffixWhen(p, base, File::File2, Rank::Rank3, Suffix::SuffixSideway);
      CheckSuffixWhen(p, base, File::File5, Rank::Rank2, Suffix::SuffixDown);
    }
    SUBCASE("C") {
      Position p = EmptyPosition();
      p.pieces[File::File5][Rank::Rank5] = prook;
      p.pieces[File::File1][Rank::Rank5] = prook;

      Move base;
      base.color = Color::Black;
      base.piece = prook;
      base.to = MakeSquare(File::File3, Rank::Rank5);
      CheckSuffixWhen(p, base, File::File5, Rank::Rank5, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank5, Suffix::SuffixRight);
    }
    SUBCASE("D") {
      Position p = EmptyPosition();
      p.pieces[File::File9][Rank::Rank9] = prook;
      p.pieces[File::File8][Rank::Rank9] = prook;

      Move base;
      base.color = Color::Black;
      base.piece = prook;
      base.to = MakeSquare(File::File8, Rank::Rank8);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank9, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank9, Suffix::SuffixRight);
    }
    SUBCASE("E") {
      Position p = EmptyPosition();
      p.pieces[File::File2][Rank::Rank8] = prook;
      p.pieces[File::File1][Rank::Rank9] = prook;

      Move base;
      base.color = Color::Black;
      base.piece = prook;
      base.to = MakeSquare(File::File1, Rank::Rank7);
      CheckSuffixWhen(p, base, File::File2, Rank::Rank8, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank9, Suffix::SuffixRight);
    }
  }
  SUBCASE("馬") {
    SUBCASE("A") {
      Position p = EmptyPosition();
      p.pieces[File::File9][Rank::Rank1] = pbishop;
      p.pieces[File::File8][Rank::Rank1] = pbishop;

      Move base;
      base.color = Color::Black;
      base.piece = pbishop;
      base.to = MakeSquare(File::File8, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank1, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File8, Rank::Rank1, Suffix::SuffixRight);
    }
    SUBCASE("B") {
      Position p = EmptyPosition();
      p.pieces[File::File6][Rank::Rank3] = pbishop;
      p.pieces[File::File9][Rank::Rank5] = pbishop;

      Move base;
      base.color = Color::Black;
      base.piece = pbishop;
      base.to = MakeSquare(File::File8, Rank::Rank5);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank5, Suffix::SuffixSideway);
      CheckSuffixWhen(p, base, File::File6, Rank::Rank3, Suffix::SuffixDown);
    }
    SUBCASE("C") {
      Position p = EmptyPosition();
      p.pieces[File::File1][Rank::Rank1] = pbishop;
      p.pieces[File::File3][Rank::Rank4] = pbishop;

      Move base;
      base.color = Color::Black;
      base.piece = pbishop;
      base.to = MakeSquare(File::File1, Rank::Rank2);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank1, Suffix::SuffixDown);
      CheckSuffixWhen(p, base, File::File3, Rank::Rank4, Suffix::SuffixUp);
    }
    SUBCASE("D") {
      Position p = EmptyPosition();
      p.pieces[File::File9][Rank::Rank9] = pbishop;
      p.pieces[File::File5][Rank::Rank9] = pbishop;

      Move base;
      base.color = Color::Black;
      base.piece = pbishop;
      base.to = MakeSquare(File::File7, Rank::Rank7);
      CheckSuffixWhen(p, base, File::File9, Rank::Rank9, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File5, Rank::Rank9, Suffix::SuffixRight);
    }
    SUBCASE("E") {
      Position p = EmptyPosition();
      p.pieces[File::File4][Rank::Rank7] = pbishop;
      p.pieces[File::File1][Rank::Rank8] = pbishop;

      Move base;
      base.color = Color::Black;
      base.piece = pbishop;
      base.to = MakeSquare(File::File2, Rank::Rank9);
      CheckSuffixWhen(p, base, File::File4, Rank::Rank7, Suffix::SuffixLeft);
      CheckSuffixWhen(p, base, File::File1, Rank::Rank8, Suffix::SuffixRight);
    }
  }
}
