#include <shogi_camera/shogi_camera.hpp>

#include <iostream>

using namespace std;

namespace sci {

Game::ApplyResult Game::apply(Move const &mv) {
  if (moves.empty()) {
    history[position] = 1;
  }
  if (!position.apply(mv, handBlack, handWhite)) {
    return ApplyResult::Illegal;
  }
  auto opponent = OpponentColor(mv.color);
  if (position.isInCheck(opponent) && !mv.from && mv.piece == MakePiece(mv.color, PieceType::Pawn)) {
    // 打ち歩による王手. 打ち歩詰めになっていないか確かめる
    deque<Move> mvs;
    Generate(position, opponent, handBlack, handWhite, mvs, false);
    if (mvs.empty()) {
      // 打ち歩詰め
      return ApplyResult::Illegal;
    }
  }
  if (position.isInCheck(OpponentColor(mv.color))) {
    if (mv.color == Color::Black) {
      blackCheckHistory[position] += 1;
      if (blackCheckHistory[position] >= 4) {
        return ApplyResult::CheckRepetitionBlack;
      }
    } else {
      whiteCheckHistory[position] += 1;
      if (whiteCheckHistory[position] >= 4) {
        return ApplyResult::CheckRepetitionWhite;
      }
    }
  } else {
    if (mv.color == Color::Black) {
      blackCheckHistory.clear();
    } else {
      whiteCheckHistory.clear();
    }
  }
  history[position] += 1;
  size_t count = history[position];
  if (count >= 4) {
    return ApplyResult::Repetition;
  }
  moves.push_back(mv);
  return ApplyResult::Ok;
}

void Game::Generate(Position const &position,
                    Color color,
                    deque<PieceType> const &handBlack,
                    deque<PieceType> const &handWhite,
                    deque<Move> &moves,
                    bool enablePawnCheckByDrop) {
  // 非合法手を含めた全ての手
  deque<Move> all;

  // 駒打ち
  set<PieceType> hand;
  for (PieceType const &h : (color == Color::Black ? handBlack : handWhite)) {
    hand.insert(h);
  }
  for (PieceType const &h : hand) {
    if (h == PieceType::Pawn) {
      // 二歩と打ち歩詰め
      Rank minY, maxY;
      if (color == Color::Black) {
        minY = Rank::Rank2;
        maxY = Rank::Rank9;
      } else {
        minY = Rank::Rank1;
        maxY = Rank::Rank8;
      }
      for (int y = minY; y <= maxY; y++) {
        for (int x = 0; x < 9; x++) {
          Piece p = position.pieces[x][y];
          if (p != 0) {
            continue;
          }
          int numPawn = 0;
          for (int i = 0; i < 9; i++) {
            if (position.pieces[x][i] == 0) {
              continue;
            }
            if (position.pieces[x][i] == MakePiece(color, PieceType::Pawn)) {
              numPawn++;
            }
          }
          if (numPawn == 0) {
            Move m;
            m.color = color;
            m.to = MakeSquare(x, y);
            m.piece = MakePiece(color, PieceType::Pawn);
            if (enablePawnCheckByDrop) {
              all.push_back(m);
            } else {
              int dy = color == Color::Black ? -1 : 1;
              Piece pp = position.pieces[x][y + dy];
              if (!(ColorFromPiece(pp) != color && PieceTypeFromPiece(pp) == PieceType::King)) {
                all.push_back(m);
              }
            }
          }
        }
      }
    } else {
      Rank minY, maxY;
      if (h == PieceType::Lance) {
        if (color == Color::Black) {
          minY = Rank::Rank2;
          maxY = Rank::Rank9;
        } else {
          minY = Rank::Rank1;
          maxY = Rank::Rank8;
        }
      } else if (h == PieceType::Knight) {
        if (color == Color::Black) {
          minY = Rank::Rank3;
          maxY = Rank::Rank9;
        } else {
          minY = Rank::Rank1;
          maxY = Rank::Rank7;
        }
      } else {
        minY = Rank::Rank1;
        maxY = Rank::Rank9;
      }
      for (int y = minY; y <= maxY; y++) {
        for (int x = 0; x < 9; x++) {
          Piece p = position.pieces[x][y];
          if (p != 0) {
            continue;
          }
          Move m;
          m.color = color;
          m.to = MakeSquare(x, y);
          m.piece = MakePiece(color, h);
          all.push_back(m);
        }
      }
    }
  }

  // 駒の移動
  for (int y = 0; y < 9; y++) {
    for (int x = 0; x < 9; x++) {
      Piece p = position.pieces[x][y];
      if (p == 0) {
        continue;
      }
      if (ColorFromPiece(p) != color) {
        continue;
      }
      Square from = MakeSquare(x, y);
      for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
          if (i == x && j == y) {
            continue;
          }
          Piece p1 = position.pieces[i][j];
          if (p1 != 0 && ColorFromPiece(p1) == color) {
            continue;
          }
          Square to = MakeSquare(i, j);
          if (!Move::CanMove(position, from, to)) {
            continue;
          }
          Move m;
          m.color = color;
          m.piece = p;
          m.from = from;
          m.to = to;
          if (p1 != 0) {
            m.captured = RemoveColorFromPiece(p1);
          }
          if (CanPromote(p) && IsPromotableMove(from, to, color)) {
            // 成
            Move mp = m;
            mp.promote = 1;
            all.push_back(mp);
            if (!MustPromote(PieceTypeFromPiece(p), from, to, color)) {
              // 不成
              Move mnp = m;
              mnp.promote = -1;
              all.push_back(mnp);
            }
          } else {
            all.push_back(m);
          }
        }
      }
    }
  }

  // 自玉に王手が掛かったままになる手を除外する
  for (int i = (int)all.size() - 1; i >= 0; i--) {
    Move mv = all[i];
    Position cp = position;
    deque<PieceType> hb = handBlack;
    deque<PieceType> hw = handWhite;
    if (!cp.apply(mv, hb, hw)) {
      all.erase(all.begin() + i);
      continue;
    }
    if (cp.isInCheck(color)) {
      all.erase(all.begin() + i);
      continue;
    }
    // 打ち歩詰めになっていないか確認する
    int dy = color == Color::Black ? -1 : 1;
    if (!mv.from && PieceTypeFromPiece(mv.piece) == PieceType::Pawn && ColorFromPiece(cp.pieces[mv.to.file][mv.to.rank + dy]) != color && PieceTypeFromPiece(cp.pieces[mv.to.file][mv.to.rank + dy]) == PieceType::King) {
      // 打ち歩による王手
      deque<Move> next;
      Generate(cp, color == Color::Black ? Color::White : Color::Black, hb, hw, next, false);
      if (next.empty()) {
        all.erase(all.begin() + i);
      }
    }
  }
  moves.swap(all);
}

void Game::generate(deque<Move> &moves) const {
  Color color = next();
  Generate(position, color, handBlack, handWhite, moves, true);
}

} // namespace sci
