#include <shogi_camera/shogi_camera.hpp>

#include <iostream>

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

#if !defined(__APPLE__)
struct CsaAdapter::Impl {
  Impl(string const &server, uint32_t port, string const &username, string const &password) {
  }
};

CsaAdapter::CsaAdapter(string const &server, uint32_t port, string const &username, string const &password) : impl(make_unique<Impl>(server, port, username, password)) {
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
      summary = summaryReceiver.validate();
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
  }
}

} // namespace sci
