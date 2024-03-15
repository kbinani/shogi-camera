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

optional<PieceUnderlyingType> PieceTypeFromString(string const &p) {
  if (p == "FU") {
    return static_cast<PieceUnderlyingType>(PieceType::Pawn);
  } else if (p == "KY") {
    return static_cast<PieceUnderlyingType>(PieceType::Lance);
  } else if (p == "KE") {
    return static_cast<PieceUnderlyingType>(PieceType::Knight);
  } else if (p == "GI") {
    return static_cast<PieceUnderlyingType>(PieceType::Silver);
  } else if (p == "KI") {
    return static_cast<PieceUnderlyingType>(PieceType::Gold);
  } else if (p == "HI") {
    return static_cast<PieceUnderlyingType>(PieceType::Rook);
  } else if (p == "KA") {
    return static_cast<PieceUnderlyingType>(PieceType::Bishop);
  } else if (p == "OU") {
    return static_cast<PieceUnderlyingType>(PieceType::King);
  } else if (p == "TO") {
    return static_cast<PieceUnderlyingType>(PieceType::Pawn) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NY") {
    return static_cast<PieceUnderlyingType>(PieceType::Lance) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NK") {
    return static_cast<PieceUnderlyingType>(PieceType::Knight) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "NG") {
    return static_cast<PieceUnderlyingType>(PieceType::Silver) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "UM") {
    return static_cast<PieceUnderlyingType>(PieceType::Bishop) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else if (p == "RY") {
    return static_cast<PieceUnderlyingType>(PieceType::Rook) | static_cast<PieceUnderlyingType>(PieceStatus::Promoted);
  } else {
    return nullopt;
  }
}
} // namespace

#if !defined(__APPLE__)
struct CsaAdapter::Impl {
  Impl(Color color, string const &server, uint32_t port, string const &username, string const &password) {
  }
};

CsaAdapter::CsaAdapter(Color color, string const &server, uint32_t port, string const &username, string const &password) : color(color), impl(make_unique<Impl>(color, server, port, username, password)) {
}

CsaAdapter::~CsaAdapter() {
}

optional<Move> CsaAdapter::next(Position const &p, vector<Move> const &moves, deque<PieceType> const &hand, deque<PieceType> const &handEnemy) {
  return nullopt;
}

void CsaAdapter::send(std::string const &) {
}
#endif

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
        Color expected = color == Color::Black ? Color::White : Color::Black;
        string gameId;
        if (summary->gameId) {
          gameId = " " + *summary->gameId;
        }
        if (summary->yourTurn != expected) {
          send("REJECT" + gameId);
        } else {
          this->summary = summary;
          send("AGREE" + gameId);
        }
      }
    } else if (type == "Position") {
      init = positionReceiver.validate();
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
        auto type = PieceTypeFromString(m.substr(2));
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
  }
}

std::optional<CsaGameSummary> CsaGameSummary::Receiver::validate() const {
  CsaGameSummary r;
  if (!protocolVersion) {
    return std::nullopt;
  }
  if (!format) {
    return std::nullopt;
  }
  r.format = *format;
  r.protocolVersion = *protocolVersion;
  r.protocolMode = protocolMode;
  r.declaration = declaration;
  r.gameId = gameId;
  if (!playerNameBlack) {
    return std::nullopt;
  }
  r.playerNameBlack = *playerNameBlack;
  if (!playerNameWhite) {
    return std::nullopt;
  }
  r.playerNameWhite = *playerNameWhite;
  if (!yourTurn) {
    return std::nullopt;
  }
  r.yourTurn = *yourTurn;
  r.rematchOnDraw = rematchOnDraw;
  if (!toMove) {
    return std::nullopt;
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
  Game g;
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
      auto type = PieceTypeFromString(item.substr(1));
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
