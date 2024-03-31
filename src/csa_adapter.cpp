#include <shogi_camera/shogi_camera.hpp>

#include <iostream>
#include <set>

using namespace std;

namespace sci {

namespace {
optional<string> Take(string const &msg, string const &key) {
  if (msg.starts_with(key + ":")) {
    return msg.substr(key.size() + 1);
  } else {
    return nullopt;
  }
}
void AnyStringValue(string const &msg, string const &key, optional<string> &dst) {
  if (auto v = Take(msg, key); v) {
    dst = *v;
  }
}

void StringValueFromPossible(string const &msg, string const &key, optional<string> &dst, initializer_list<string> values) {
  if (auto value = Take(msg, key); value) {
    for (auto const &v : values) {
      if (v == *value) {
        dst = *value;
        return;
      }
    }
  }
}
} // namespace

CsaAdapter::CsaAdapter(std::weak_ptr<CsaServer> server) : server(server), stopSignal(false), game(Handicap::平手, false) {
}

CsaAdapter::~CsaAdapter() {
}

void CsaAdapter::send(std::string const &msg) {
  if (writer) {
    writer->send(msg);
  } else {
    bufferedCommands.push_back(msg);
  }
}

void CsaAdapter::setWriter(std::shared_ptr<CsaServer::Writer> writer) {
  this->writer = writer;
  for (auto const &b : bufferedCommands) {
    writer->send(b);
  }
  bufferedCommands.clear();
}

std::optional<std::u8string> CsaAdapter::name() {
  if (!summary) {
    return std::nullopt;
  }
  if (!color_) {
    return std::nullopt;
  }
  if (*color_ == Color::White) {
    return std::u8string((char8_t const *)summary->playerNameWhite.c_str());
  } else {
    return std::u8string((char8_t const *)summary->playerNameBlack.c_str());
  }
}

void CsaAdapter::onmessage(string const &msg) {
  if (msg.starts_with("BEGIN ")) {
    string type = msg.substr(6);
    current = type;
    stack.push_back(type);
  } else if (msg.starts_with("END ")) {
    string type = msg.substr(4);
    if (stack.empty() || current != type) {
      cerr << "csa:error:invalid message structure" << endl;
      return;
    }
    if (type == "Game_Summary") {
      if (auto summary = summaryReceiver.validate(); summary) {
        string gameId;
        if (summary->gameId) {
          gameId = " " + *summary->gameId;
        }
        color_ = OpponentColor(summary->yourTurn);
        this->summary = summary;
        send("AGREE" + gameId);
      }
    } else if (type == "Position") {
      if (auto init = positionReceiver.validate(); init) {
        this->init = init;
        game = init->first;
      }
      // TODO: 途中局面から開始の場合
    }
    stack.pop_back();
    if (stack.empty()) {
      current = "";
    } else {
      current = stack.back();
    }
  } else if (current == "Game_Summary") {
    AnyStringValue(msg, "Protocol_Version", summaryReceiver.protocolVersion);
    StringValueFromPossible(msg, "Protocol_Mode", summaryReceiver.protocolMode, {"Server", "Direct"});
    AnyStringValue(msg, "Format", summaryReceiver.format);
    AnyStringValue(msg, "Declaration", summaryReceiver.declaration);
    AnyStringValue(msg, "Game_ID", summaryReceiver.gameId);
    AnyStringValue(msg, "Name+", summaryReceiver.playerNameBlack);
    AnyStringValue(msg, "Name-", summaryReceiver.playerNameWhite);
    if (auto v = Take(msg, "Your_Turn"); v) {
      if (v == "+") {
        summaryReceiver.yourTurn = Color::Black;
      } else if (v == "-") {
        summaryReceiver.yourTurn = Color::White;
      }
    }
    if (auto v = Take(msg, "Rematch_On_Draw"); v) {
      if (v == "YES") {
        summaryReceiver.rematchOnDraw = true;
      } else if (v == "NO") {
        summaryReceiver.rematchOnDraw = false;
      }
    }
    if (auto v = Take(msg, "To_Move"); v) {
      if (v == "+") {
        summaryReceiver.toMove = Color::Black;
      } else if (v == "-") {
        summaryReceiver.toMove = Color::White;
      }
    }
    if (auto v = Take(msg, "MaxMoves"); v) {
      summaryReceiver.maxMoves = atoi(v->c_str());
    }
  } else if (current == "Position") {
    if (msg.starts_with("PI")) {
      if (positionReceiver.i) {
        positionReceiver.error = true;
      } else {
        positionReceiver.i = msg;
      }
    } else if (msg == "+") {
      if (positionReceiver.next) {
        positionReceiver.error = true;
      } else {
        positionReceiver.next = Color::Black;
      }
    } else if (msg == "-") {
      if (positionReceiver.next) {
        positionReceiver.error = true;
      } else {
        positionReceiver.next = Color::White;
      }
    } else if (msg.starts_with("P+") || msg.starts_with("P-")) {
      Color color = Color::Black;
      if (msg.starts_with("P-")) {
        color = Color::White;
      }
      auto m = msg.substr(2);
      while (m.size() >= 4) {
        auto f = atoi(m.substr(0, 1).c_str());
        auto r = atoi(m.substr(1, 1).c_str());
        if (!m.starts_with(to_string(f) + to_string(r))) {
          positionReceiver.error = true;
          break;
        }
        auto type = PieceTypeFromCsaString(m.substr(2));
        if (!type) {
          positionReceiver.error = true;
          break;
        }
        positionReceiver.pieces[make_tuple(color, f, r)] = *type;
      }
    } else {
      auto rank = atoi(msg.substr(1, 1).c_str());
      if (msg.starts_with("P" + to_string(rank)) && 1 <= rank && rank <= 9) {
        positionReceiver.ranks[rank] = msg;
      } else {
        positionReceiver.error = true;
      }
    }
  } else if (current.empty()) {
    if (auto start = Take(msg, "START"); start) {
      started = true;
    } else if (auto reject = Take(msg, "REJECT"); reject) {
      rejected = true;
    } else if (started && !resultNotified) {
      if ((msg.starts_with("+") || msg.starts_with("-")) && msg.size() >= 7) {
        auto mv = MoveFromCsaMove(msg, game.position);
        if (holds_alternative<Move>(mv)) {
          auto m = get<Move>(mv);
          switch (game.apply(m)) {
          case Game::ApplyResult::Ok: {
            lock_guard<mutex> lock(mut);
            moves.push_back(m);
            cv.notify_all();
            break;
          }
          case Game::ApplyResult::Illegal:
            if (color_) {
              if (*color_ == Color::Black) {
                result = GameResult::WhiteWin;
                reason = GameResultReason::IllegalAction;
              } else {
                result = GameResult::BlackWin;
                reason = GameResultReason::IllegalAction;
              }
              notifyResult();
            }
            cv.notify_all();
            break;
          case Game::ApplyResult::Repetition:
            result = GameResult::Abort;
            reason = GameResultReason::Repetition;
            notifyResult();
            cv.notify_all();
            break;
          case Game::ApplyResult::CheckRepetitionBlack:
            result = GameResult::WhiteWin;
            reason = GameResultReason::CheckRepetition;
            notifyResult();
            cv.notify_all();
            break;
          case Game::ApplyResult::CheckRepetitionWhite:
            result = GameResult::BlackWin;
            reason = GameResultReason::CheckRepetition;
            notifyResult();
            cv.notify_all();
            break;
          }
        } else if (holds_alternative<u8string>(mv)) {
          error(get<u8string>(mv));
          return;
        }
      } else if (msg.starts_with("#WIN")) {
        if (color_) {
          if (*color_ == Color::Black) {
            result = GameResult::WhiteWin;
          } else {
            result = GameResult::BlackWin;
          }
          notifyResult();
        }
        cv.notify_all();
      } else if (msg.starts_with("#LOSE")) {
        if (color_) {
          if (*color_ == Color::Black) {
            result = GameResult::BlackWin;
          } else {
            result = GameResult::WhiteWin;
          }
          notifyResult();
        }
        cv.notify_all();
      } else if (msg.starts_with("#CHUDAN")) {
        result = GameResult::Abort;
        reason = GameResultReason::Abort;
        notifyResult();
        cv.notify_all();
      } else if (msg.starts_with("#RESIGN") || msg.starts_with("%TORYO,")) {
        reason = GameResultReason::Resign;
        notifyResult();
      } else if (msg == "#OUTE_SENNICHITE") {
        reason = GameResultReason::CheckRepetition;
        notifyResult();
      } else if (msg == "#SENNICHITE") {
        reason = GameResultReason::Repetition;
        notifyResult();
      } else if (msg == "#DRAW") {
        // TODO: GameResult::Draw
        result = GameResult::Abort;
        notifyResult();
        cv.notify_all();
      }
    }
  }
}

void CsaAdapter::notifyResult() {
  if (resultNotified) {
    return;
  }
  if (!result || !reason) {
    return;
  }
  resultNotified = true;
  if (auto d = delegate.lock(); d) {
    d->csaAdapterDidFinishGame(*result, *reason);
  }
}

optional<Move> CsaAdapter::next(Position const &p, Color next, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  if (!color_) {
    return nullopt;
  }
  for (size_t i = this->moves.size(); i < moves.size(); i++) {
    Move m = moves[i];
    if (m.color == OpponentColor(*color_)) {
      string line;
      if (m.color == Color::Black) {
        line += "+";
      } else {
        line += "-";
      }
      if (m.from) {
        line += to_string(9 - m.from->file) + to_string(m.from->rank + 1);
      } else {
        line += "00";
      }
      line += to_string(9 - m.to.file) + to_string(m.to.rank + 1);
      auto p = CsaStringFromPiece(m.piece, m.promote);
      if (!p) {
        error(u8"指し手をcsa形式に変換できませんでした");
        return nullopt;
      }
      line += *p;
      send(line);
    }
  }
  unique_lock<mutex> lock(mut);
  cv.wait(lock, [&]() {
    return this->moves.size() == moves.size() + 1 || stopSignal || result;
  });
  Move mv = this->moves.back();
  lock.unlock();
  return mv;
}

void CsaAdapter::error(std::u8string const &what) {
  if (auto d = delegate.lock(); d) {
    d->csaAdapterDidGetError(what);
  }
}

void CsaAdapter::resign(Color color) {
  auto found = resignSent.find(color);
  if (!this->color_) {
    return;
  }
  if (color != OpponentColor(*this->color_)) {
    return;
  }
  if (found == resignSent.end() || !found->second) {
    send("%TORYO");
    resignSent[color] = true;
  }
}

void CsaAdapter::stop() {
  stopSignal = true;
  cv.notify_all();
}

optional<CsaGameSummary> CsaGameSummary::Receiver::validate() const {
  CsaGameSummary r;
  if (!protocolVersion) {
    return nullopt;
  }
  if (!format) {
    return nullopt;
  }
  r.format = *format;
  r.protocolVersion = *protocolVersion;
  r.protocolMode = protocolMode;
  r.declaration = declaration;
  r.gameId = gameId;
  if (!playerNameBlack) {
    return nullopt;
  }
  r.playerNameBlack = *playerNameBlack;
  if (!playerNameWhite) {
    return nullopt;
  }
  r.playerNameWhite = *playerNameWhite;
  if (!yourTurn) {
    return nullopt;
  }
  r.yourTurn = *yourTurn;
  r.rematchOnDraw = rematchOnDraw;
  if (!toMove) {
    return nullopt;
  }
  r.toMove = *toMove;
  r.maxMoves = maxMoves;
  return r;
}

optional<pair<Game, Color>> CsaPositionReceiver::validate() const {
  if (error) {
    return nullopt;
  }
  if (i != nullopt && !ranks.empty()) {
    return nullopt;
  }
  if (!next) {
    return nullopt;
  }
  Game g(Handicap::平手, false);
  for (int x = 0; x < 9; x++) {
    for (int y = 0; y < 9; y++) {
      g.position.pieces[x][y] = 0;
    }
  }
  multiset<Piece> box;
  for (auto color : {Color::Black, Color::White}) {
    for (int i = 0; i < 9; i++) {
      box.insert(MakePiece(color, PieceType::Pawn));
    }
    for (int i = 0; i < 2; i++) {
      box.insert(MakePiece(color, PieceType::Lance));
      box.insert(MakePiece(color, PieceType::Knight));
      box.insert(MakePiece(color, PieceType::Silver));
      box.insert(MakePiece(color, PieceType::Gold));
    }
    box.insert(MakePiece(color, PieceType::Rook));
    box.insert(MakePiece(color, PieceType::Bishop));
    box.insert(MakePiece(color, PieceType::King));
  }
  if (i) {
    if (!i->starts_with("PI")) {
      return nullopt;
    }
    box.clear();
    u8string r = (char8_t const *)i->substr(2).c_str();
    while (r.size() >= 4) {
      auto p = r.substr(0, 4);
      auto sq = TrimSquarePartFromString(p);
      if (!sq) {
        return nullopt;
      }
      Piece pc = g.position.pieces[sq->file][sq->rank];
      PieceType type = PieceTypeFromPiece(pc);
      if (p == u8"FU" && type != PieceType::Pawn) {
        return nullopt;
      } else if (p == u8"KY" && type != PieceType::Lance) {
        return nullopt;
      } else if (p == u8"KE" && type != PieceType::Knight) {
        return nullopt;
      } else if (p == u8"GI" && type != PieceType::Silver) {
        return nullopt;
      } else if (p == u8"KI" && type != PieceType::Gold) {
        return nullopt;
      } else if (p == u8"HI" && type != PieceType::Rook) {
        return nullopt;
      } else if (p == u8"KA" && type != PieceType::Bishop) {
        return nullopt;
      } else if (p == u8"OU") {
        return nullopt;
      } else if (type == PieceType::King) {
        return nullopt;
      }
      g.position.pieces[sq->file][sq->rank] = 0;
      box.insert(pc);
      r = r.substr(4);
    }
  } else {
    for (int x = 0; x < 9; x++) {
      for (int y = 0; y < 9; y++) {
        g.position.pieces[x][y] = 0;
      }
    }
  }
  for (auto const &it : ranks) {
    int r = it.first;
    Rank rank = static_cast<Rank>(r - 1);
    string line = it.second;
    if (line.size() < 29) {
      return nullopt;
    }
    for (int x = 0; x < 9; x++) {
      string item = line.substr(2 + x * 3, 3);
      if (item == " * ") {
        continue;
      }
      Color color;
      if (item.starts_with("+")) {
        color = Color::Black;
      } else if (item.starts_with("-")) {
        color = Color::White;
      } else {
        return nullopt;
      }
      auto type = PieceTypeFromCsaString(item.substr(1));
      if (!type) {
        return nullopt;
      }
      Piece pc = static_cast<PieceUnderlyingType>(color) | static_cast<PieceUnderlyingType>(*type);
      auto found = box.find(pc);
      if (found == box.end()) {
        return nullopt;
      }
      box.erase(found);
      if (g.position.pieces[x][rank] != 0) {
        return nullopt;
      }
      g.position.pieces[x][rank] = pc;
    }
  }
  if (!pieces.empty()) {
    bool blackAL = false;
    bool whiteAL = false;
    for (auto const &it : pieces) {
      auto [color, f, r] = it.first;
      if (f == 0 && r == 0) {
        if (color == Color::Black) {
          if (blackAL) {
            return nullopt;
          }
          blackAL = true;
        } else {
          if (whiteAL) {
            return nullopt;
          }
          whiteAL = true;
        }
        continue;
      }
      if (f < 1 || 9 < f || r < 1 || 9 < r) {
        return nullopt;
      }
      PieceUnderlyingType piece = it.second;
      File file = static_cast<File>(9 - f);
      Rank rank = static_cast<Rank>(r - 1);
      Piece pc = static_cast<PieceUnderlyingType>(color) | piece;
      auto found = box.find(Unpromote(pc));
      if (found == box.end()) {
        return nullopt;
      }
      if (g.position.pieces[file][rank] != 0) {
        return nullopt;
      }
      g.position.pieces[file][rank] = pc;
      box.erase(found);
    }
    if (box.contains(MakePiece(Color::Black, PieceType::King))) {
      return nullopt;
    }
    if (box.contains(MakePiece(Color::White, PieceType::King))) {
      return nullopt;
    }
    if (blackAL) {
      for (auto const &it : box) {
        if (ColorFromPiece(it) == Color::Black) {
          g.handBlack.push_back(PieceTypeFromPiece(it));
        }
      }
    }
    if (whiteAL) {
      for (auto const &it : box) {
        if (ColorFromPiece(it) == Color::White) {
          g.handBlack.push_back(PieceTypeFromPiece(it));
        }
      }
    }
  }
  return make_pair(g, *next);
}

} // namespace sci
