#pragma once

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
  Piece gold = MakePiece(Color::Black, PieceType::Gold);
  Piece silver = MakePiece(Color::Black, PieceType::Silver);
  Piece pp = MakePiece(Color::Black, PieceType::Pawn, PieceStatus::Promoted);
  Piece prook = MakePiece(Color::Black, PieceType::Rook, PieceStatus::Promoted);
  Piece pbishop = MakePiece(Color::Black, PieceType::Bishop, PieceStatus::Promoted);
  SUBCASE("https://www.shogi.or.jp/faq/kihuhyouki.html") {
    SUBCASE("寄・引・上") {
      SUBCASE("A") {
        Position p = EmptyPosition();
        p.pieces[File::File7][Rank::Rank2] = gold;
        p.pieces[File::File9][Rank::Rank3] = gold;

        Move base;
        base.color = Color::Black;
        base.piece = gold;
        base.to = MakeSquare(File::File8, Rank::Rank2);
        CheckSuffixWhen(p, base, File::File9, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File7, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Sideway));
      }
      SUBCASE("B") {
        Position p = EmptyPosition();
        p.pieces[File::File3][Rank::Rank1] = gold;
        p.pieces[File::File4][Rank::Rank3] = gold;

        Move base;
        base.color = Color::Black;
        base.piece = gold;
        base.to = MakeSquare(File::File3, Rank::Rank2);
        CheckSuffixWhen(p, base, File::File4, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File3, Rank::Rank1, static_cast<SuffixUnderlyingType>(SuffixType::Down));
      }
      SUBCASE("C") {
        Position p = EmptyPosition();
        p.pieces[File::File4][Rank::Rank5] = gold;
        p.pieces[File::File5][Rank::Rank6] = gold;

        Move base;
        base.color = Color::Black;
        base.piece = gold;
        base.to = MakeSquare(File::File5, Rank::Rank5);
        CheckSuffixWhen(p, base, File::File5, Rank::Rank6, static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File4, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Sideway));
      }
      SUBCASE("D") {
        Position p = EmptyPosition();
        p.pieces[File::File7][Rank::Rank7] = silver;
        p.pieces[File::File8][Rank::Rank9] = silver;

        Move base;
        base.color = Color::Black;
        base.piece = silver;
        base.to = MakeSquare(File::File8, Rank::Rank8);
        CheckSuffixWhen(p, base, File::File8, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File7, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Down));
      }
      SUBCASE("E") {
        Position p = EmptyPosition();
        p.pieces[File::File2][Rank::Rank7] = silver;
        p.pieces[File::File4][Rank::Rank9] = silver;

        Move base;
        base.color = Color::Black;
        base.piece = silver;
        base.to = MakeSquare(File::File3, Rank::Rank8);
        CheckSuffixWhen(p, base, File::File4, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File2, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Down));
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
        CheckSuffixWhen(p, base, File::File9, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File7, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("B") {
        Position p = EmptyPosition();
        p.pieces[File::File3][Rank::Rank2] = gold;
        p.pieces[File::File1][Rank::Rank2] = gold;

        Move base;
        base.color = Color::Black;
        base.piece = gold;
        base.to = MakeSquare(File::File2, Rank::Rank2);
        CheckSuffixWhen(p, base, File::File3, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File1, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("C") {
        Position p = EmptyPosition();
        p.pieces[File::File6][Rank::Rank5] = silver;
        p.pieces[File::File4][Rank::Rank5] = silver;

        Move base;
        base.color = Color::Black;
        base.piece = silver;
        base.to = MakeSquare(File::File5, Rank::Rank6);
        CheckSuffixWhen(p, base, File::File6, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File4, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("D") {
        Position p = EmptyPosition();
        p.pieces[File::File8][Rank::Rank9] = gold;
        p.pieces[File::File7][Rank::Rank9] = gold;

        Move base;
        base.color = Color::Black;
        base.piece = gold;
        base.to = MakeSquare(File::File7, Rank::Rank8);
        CheckSuffixWhen(p, base, File::File8, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File7, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Nearest));
      }
      SUBCASE("E") {
        Position p = EmptyPosition();
        p.pieces[File::File3][Rank::Rank9] = silver;
        p.pieces[File::File2][Rank::Rank9] = silver;

        Move base;
        base.color = Color::Black;
        base.piece = silver;
        base.to = MakeSquare(File::File3, Rank::Rank8);
        CheckSuffixWhen(p, base, File::File3, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Nearest));
        CheckSuffixWhen(p, base, File::File2, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Right));
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
        CheckSuffixWhen(p, base, File::File6, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File5, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Nearest));
        CheckSuffixWhen(p, base, File::File4, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Right));
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
        CheckSuffixWhen(p, base, File::File7, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Right));
        CheckSuffixWhen(p, base, File::File8, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Nearest));
        CheckSuffixWhen(p, base, File::File9, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File9, Rank::Rank8, static_cast<SuffixUnderlyingType>(SuffixType::Sideway));
        CheckSuffixWhen(p, base, File::File8, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Down));
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
        CheckSuffixWhen(p, base, File::File2, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Nearest));
        CheckSuffixWhen(p, base, File::File1, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Right));
        CheckSuffixWhen(p, base, File::File3, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Up));
        CheckSuffixWhen(p, base, File::File3, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Left) | static_cast<SuffixUnderlyingType>(SuffixType::Down));
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
        CheckSuffixWhen(p, base, File::File9, Rank::Rank1, static_cast<SuffixUnderlyingType>(SuffixType::Down));
        CheckSuffixWhen(p, base, File::File8, Rank::Rank4, static_cast<SuffixUnderlyingType>(SuffixType::Up));
      }
      SUBCASE("B") {
        Position p = EmptyPosition();
        p.pieces[File::File5][Rank::Rank2] = prook;
        p.pieces[File::File2][Rank::Rank3] = prook;

        Move base;
        base.color = Color::Black;
        base.piece = prook;
        base.to = MakeSquare(File::File4, Rank::Rank3);
        CheckSuffixWhen(p, base, File::File2, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Sideway));
        CheckSuffixWhen(p, base, File::File5, Rank::Rank2, static_cast<SuffixUnderlyingType>(SuffixType::Down));
      }
      SUBCASE("C") {
        Position p = EmptyPosition();
        p.pieces[File::File5][Rank::Rank5] = prook;
        p.pieces[File::File1][Rank::Rank5] = prook;

        Move base;
        base.color = Color::Black;
        base.piece = prook;
        base.to = MakeSquare(File::File3, Rank::Rank5);
        CheckSuffixWhen(p, base, File::File5, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File1, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("D") {
        Position p = EmptyPosition();
        p.pieces[File::File9][Rank::Rank9] = prook;
        p.pieces[File::File8][Rank::Rank9] = prook;

        Move base;
        base.color = Color::Black;
        base.piece = prook;
        base.to = MakeSquare(File::File8, Rank::Rank8);
        CheckSuffixWhen(p, base, File::File9, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File8, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("E") {
        Position p = EmptyPosition();
        p.pieces[File::File2][Rank::Rank8] = prook;
        p.pieces[File::File1][Rank::Rank9] = prook;

        Move base;
        base.color = Color::Black;
        base.piece = prook;
        base.to = MakeSquare(File::File1, Rank::Rank7);
        CheckSuffixWhen(p, base, File::File2, Rank::Rank8, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File1, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Right));
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
        CheckSuffixWhen(p, base, File::File9, Rank::Rank1, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File8, Rank::Rank1, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("B") {
        Position p = EmptyPosition();
        p.pieces[File::File6][Rank::Rank3] = pbishop;
        p.pieces[File::File9][Rank::Rank5] = pbishop;

        Move base;
        base.color = Color::Black;
        base.piece = pbishop;
        base.to = MakeSquare(File::File8, Rank::Rank5);
        CheckSuffixWhen(p, base, File::File9, Rank::Rank5, static_cast<SuffixUnderlyingType>(SuffixType::Sideway));
        CheckSuffixWhen(p, base, File::File6, Rank::Rank3, static_cast<SuffixUnderlyingType>(SuffixType::Down));
      }
      SUBCASE("C") {
        Position p = EmptyPosition();
        p.pieces[File::File1][Rank::Rank1] = pbishop;
        p.pieces[File::File3][Rank::Rank4] = pbishop;

        Move base;
        base.color = Color::Black;
        base.piece = pbishop;
        base.to = MakeSquare(File::File1, Rank::Rank2);
        CheckSuffixWhen(p, base, File::File1, Rank::Rank1, static_cast<SuffixUnderlyingType>(SuffixType::Down));
        CheckSuffixWhen(p, base, File::File3, Rank::Rank4, static_cast<SuffixUnderlyingType>(SuffixType::Up));
      }
      SUBCASE("D") {
        Position p = EmptyPosition();
        p.pieces[File::File9][Rank::Rank9] = pbishop;
        p.pieces[File::File5][Rank::Rank9] = pbishop;

        Move base;
        base.color = Color::Black;
        base.piece = pbishop;
        base.to = MakeSquare(File::File7, Rank::Rank7);
        CheckSuffixWhen(p, base, File::File9, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File5, Rank::Rank9, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
      SUBCASE("E") {
        Position p = EmptyPosition();
        p.pieces[File::File4][Rank::Rank7] = pbishop;
        p.pieces[File::File1][Rank::Rank8] = pbishop;

        Move base;
        base.color = Color::Black;
        base.piece = pbishop;
        base.to = MakeSquare(File::File2, Rank::Rank9);
        CheckSuffixWhen(p, base, File::File4, Rank::Rank7, static_cast<SuffixUnderlyingType>(SuffixType::Left));
        CheckSuffixWhen(p, base, File::File1, Rank::Rank8, static_cast<SuffixUnderlyingType>(SuffixType::Right));
      }
    }
  }
  SUBCASE("original") {
    SUBCASE("1-34") {
      Position p = EmptyPosition();
      p.pieces[File::File5][Rank::Rank2] = MakePiece(Color::White, PieceType::Gold);
      p.pieces[File::File4][Rank::Rank1] = MakePiece(Color::White, PieceType::Gold);
      Move m;
      m.color = Color::White;
      m.piece = MakePiece(Color::White, PieceType::Gold);
      m.from = MakeSquare(File::File5, Rank::Rank2);
      m.to = MakeSquare(File::File5, Rank::Rank1);
      m.decideSuffix(p);
      CHECK(m.suffix == static_cast<SuffixUnderlyingType>(SuffixType::Down));
    }
    SUBCASE("1-55") {
      Position p = EmptyPosition();
      p.pieces[File::File6][Rank::Rank6] = gold;
      p.pieces[File::File5][Rank::Rank8] = gold;
      Move m;
      m.color = Color::Black;
      m.piece = gold;
      m.from = MakeSquare(File::File6, Rank::Rank6);
      m.to = MakeSquare(File::File6, Rank::Rank7);
      m.decideSuffix(p);
      CHECK(m.suffix == static_cast<SuffixUnderlyingType>(SuffixType::Down));
    }
  }
}
