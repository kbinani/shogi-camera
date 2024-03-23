#include <shogi_camera/shogi_camera.hpp>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

namespace sci {

namespace {
optional<PieceUnderlyingType> PieceTypeFromCsaString(string const &p) {
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

struct RemotePeer : public CsaServer::Peer {
  explicit RemotePeer(int socket) : socket(socket) {}
  void onmessage(std::string const &line) override {}
  void send(std::string const &line) override {
    string m = line + "\x0a";
    ::send(socket, m.c_str(), m.size(), 0);
    cout << "csa_server:<<< " << line << endl;
  }
  int socket;
};

struct CsaServer::Impl {
  Impl(int port) : port(port) {
    stop = false;
  }

  ~Impl() {
    stop = true;
    close(socket);
    if (auto remote = this->remote.lock(); remote) {
      close(remote->socket);
    }
  }

  void start(shared_ptr<Peer> local, Color localColor) {
    Local l;
    l.peer = local;
    l.color = localColor;
    this->local = l;
    thread(std::bind(&Impl::run, this)).detach();
  }

  void run() {
    this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("0.0.0.0", &addr.sin_addr);

    if (::bind(this->socket, (sockaddr *)&addr, sizeof(addr)) < 0) {
      cerr << "bind failed" << endl;
      return;
    }
    listen(this->socket, 1);
    while (!stop) {
      int socket = accept(this->socket, nil, nil);
      if (auto existing = this->remote.lock(); existing) {
        close(socket);
        continue;
      }
      auto remote = make_shared<RemotePeer>(socket);
      auto weak = weak_ptr<RemotePeer>(remote);
      thread(std::bind(&Impl::loop, this, remote)).detach();
      this->remote = weak;
    }
  }

  void send(string const &msg) {
    auto remote = this->remote.lock();
    if (!remote) {
      return;
    }
    lock_guard<recursive_mutex> lock(mut);
    if (!info) {
      return;
    }
    auto local = this->local;
    if (!local) {
      return;
    }
    auto sendboth = [local, remote](string const &msg) {
      remote->send(msg);
      if (local) {
        local->peer->onmessage(msg);
      }
    };
    if (msg.starts_with("AGREE")) {
      info->agrees++;
      if (info->agrees > 1) {
        sendboth("START:" + info->gameId);
        info->update();
      }
    } else if (msg.starts_with("+")) {
      if (info->game.moves.size() % 2 != 0) {
        sendboth("#ILLEGAL_ACTION");
        if (local->color == Color::Black) {
          local->peer->onmessage("#LOSE");
          remote->send("#WIN");
        } else {
          local->peer->onmessage("#WIN");
          remote->send("#LOSE");
        }
        info.reset();
      } else {
        auto ret = MoveFromCsaMove(msg, info->game.position);
        if (holds_alternative<Move>(ret)) {
          auto mv = get<Move>(ret);
          info->game.apply(mv);
          info->game.moves.push_back(mv);
          sendboth(msg + "," + info->seconds());
          info->update();
        } else {
          sendboth("#ILLEGAAL_MOVE");
          local->peer->onmessage("#LOSE");
          remote->send("#WIN");
          info.reset();
        }
      }
    } else if (msg.starts_with("-")) {
      if (info->game.moves.size() % 2 != 1) {
        sendboth("#ILLEGAL_ACTION");
        local->peer->onmessage("#LOSE");
        remote->send("#WIN");
        info.reset();
      } else {
        auto ret = MoveFromCsaMove(msg, info->game.position);
        if (holds_alternative<Move>(ret)) {
          auto mv = get<Move>(ret);
          info->game.apply(mv);
          info->game.moves.push_back(mv);
          sendboth(msg + "," + info->seconds());
          info->update();
        } else {
          sendboth("#ILLEGAAL_MOVE");
          local->peer->onmessage("#WIN");
          remote->send("#LOSE");
          info.reset();
        }
      }
    }
  }

  void loop(shared_ptr<RemotePeer> peer) {
    auto local = this->local;
    if (!local) {
      return;
    }
    auto send = [peer](string const &msg) {
      peer->send(msg);
    };
    auto sendboth = [local, peer](string const &msg) {
      peer->send(msg);
      local->peer->onmessage(msg);
    };

    string buffer;
    optional<string> username;

    while (!stop) {
      char tmp[64];
      ssize_t len = recv(peer->socket, tmp, sizeof(tmp), 0);
      if (len <= 0) {
        break;
      }
      copy_n(tmp, len, back_inserter(buffer));
      string::size_type offset = 0;
      while (offset < buffer.size()) {
        auto found = buffer.find('\xa', offset);
        if (found == string::npos) {
          break;
        }
        string line = buffer.substr(offset, found);
        cout << "csa_server:>>> " << line << endl;
        lock_guard<recursive_mutex> lock(mut);
        if (!username && line.starts_with("LOGIN ")) {
          auto n = line.find(' ', 6);
          if (n != string::npos) {
            auto name = line.substr(6, n - 6);
            send("LOGIN:" + name + " OK");
            username = name;
            info = make_unique<Info>();
            sendboth("BEGIN Game_Summary");
            sendboth("Protocol_Version:1.2");
            sendboth("Protocol_Mode:Server");
            sendboth("Format:Shogi 1.0");
            sendboth("Game_ID:" + info->gameId);
            if (local->color == Color::Black) {
              sendboth("Name+:Player");
              sendboth("Name-:" + name);
              send("Your_Turn:-");
              local->peer->onmessage("Your_Turn:+");
            } else {
              sendboth("Name+:" + name);
              sendboth("Name-:Player");
              send("Your_Turn:+");
              local->peer->onmessage("Your_Turn:-");
            }
            sendboth("To_Move:+");
            sendboth("BEGIN Position");
            sendboth("P1-KY-KE-GI-KI-OU-KI-GI-KE-KY");
            sendboth("P2 * -HI *  *  *  *  * -KA * ");
            sendboth("P3-FU-FU-FU-FU-FU-FU-FU-FU-FU");
            sendboth("P4 *  *  *  *  *  *  *  *  * ");
            sendboth("P5 *  *  *  *  *  *  *  *  * ");
            sendboth("P6 *  *  *  *  *  *  *  *  * ");
            sendboth("P7+FU+FU+FU+FU+FU+FU+FU+FU+FU");
            sendboth("P8 * +KA *  *  *  *  * +HI * ");
            sendboth("P9+KY+KE+GI+KI+OU+KI+GI+KE+KY");
            sendboth("P+");
            sendboth("P-");
            sendboth("+");
            sendboth("END Position");
            sendboth("END Game_Summary");
          }
        } else if (username && line == "LOGOUT") {
          send("LOGOUT:completed");
          close(peer->socket);
          return;
        } else if (line == "AGREE" && info) {
          info->agrees++;
          if (info->agrees > 1) {
            sendboth("START:" + info->gameId);
            info->update();
          }
        } else if (info && line == "AGREE " + info->gameId) {
          info->agrees++;
          if (info->agrees > 1) {
            sendboth("START:" + info->gameId);
            info->update();
          }
        } else if (info && line == "REJECT") {
          if (username) {
            local->peer->onmessage("REJECT:" + info->gameId + " by " + *username);
          }
          info.reset();
        } else if (info && line == "REJECT " + info->gameId) {
          if (username) {
            local->peer->onmessage("REJECT:" + info->gameId + " by " + *username);
          }
          info.reset();
        } else if (line == "%CHUDAN") {
          local->peer->onmessage("%CHUDAN");
          info.reset();
        } else if (line.starts_with("+") && info) {
          if (info->game.moves.size() % 2 != 0) {
            sendboth("#ILLEGAL_ACTION");
            if (local->color == Color::Black) {
              send("#LOSE");
              local->peer->onmessage("#WIN");
            } else {
              send("#WIN");
              local->peer->onmessage("#LOSE");
            }
            info.reset();
          } else {
            auto ret = MoveFromCsaMove(line, info->game.position);
            if (holds_alternative<Move>(ret)) {
              auto mv = get<Move>(ret);
              info->game.apply(mv);
              info->game.moves.push_back(mv);
              sendboth(line + "," + info->seconds());
              info->update();
            } else {
              sendboth("#ILLEGAAL_MOVE");
              send("#LOSE");
              local->peer->onmessage("#WIN");
              info.reset();
            }
          }
        } else if (line.starts_with("-") && info) {
          if (info->game.moves.size() % 2 != 1) {
            sendboth("#ILLEGAL_ACTION");
            send("#LOSE");
            local->peer->onmessage("#WIN");
            info.reset();
          } else {
            auto ret = MoveFromCsaMove(line, info->game.position);
            if (holds_alternative<Move>(ret)) {
              auto mv = get<Move>(ret);
              info->game.apply(mv);
              info->game.moves.push_back(mv);
              sendboth(line + "," + info->seconds());
              info->update();
            } else {
              sendboth("#ILLEGAAL_MOVE");
              send("#WIN");
              local->peer->onmessage("#LOSE");
              info.reset();
            }
          }
        } else if (line == "%TORYO") {
          sendboth("%TORYO," + info->seconds());
          sendboth("#RESIGN");
          send("#LOSE");
          local->peer->onmessage("#WIN");
          info.reset();
        }
        offset = found + 1;
      }
      buffer = buffer.substr(offset);
    }
  }

  void setLocal(std::shared_ptr<Peer> local, Color color) {
    Local l;
    l.peer = local;
    l.color = color;
    this->local = l;
  }

  atomic_bool stop;
  int socket;
  int port;
  struct Local {
    shared_ptr<Peer> peer;
    Color color;
  };
  optional<Local> local;
  weak_ptr<RemotePeer> remote;
  std::recursive_mutex mut;

  struct Info {
    Info() {
      gameId = "shogicamera";
      time = chrono::system_clock::now();
    }

    string seconds() const {
      auto elapsed = chrono::duration_cast<chrono::duration<double>>(chrono::system_clock::now() - time).count();
      return "T" + to_string(std::max(0, (int)floor(elapsed)));
    }

    void update() {
      time = chrono::system_clock::now();
    }

    string gameId;
    Game game;
    chrono::system_clock::time_point time;
    int agrees = 0;
  };
  std::unique_ptr<Info> info;
};

CsaServer::CsaServer(int port) : impl(make_unique<Impl>(port)) {
}

CsaServer::~CsaServer() {}

void CsaServer::start(std::shared_ptr<Peer> local, Color color) {
  impl->start(local, color);
}

void CsaServer::send(string const &msg) {
  impl->send(msg);
}

variant<Move, u8string> CsaServer::MoveFromCsaMove(string const &msg, Position const &position) {
  if (!msg.starts_with("+") && !msg.starts_with("-")) {
    return u8"指し手を読み取れませんでした";
  }
  if (msg.size() < 7) {
    return u8"指し手を読み取れませんでした";
  }
  Color color = Color::Black;
  if (msg.starts_with("-")) {
    color = Color::White;
  }
  auto fromFile = atoi(msg.substr(1, 1).c_str());
  auto fromRank = atoi(msg.substr(2, 1).c_str());
  auto toFile = atoi(msg.substr(3, 1).c_str());
  auto toRank = atoi(msg.substr(4, 1).c_str());
  auto piece = PieceTypeFromCsaString(msg.substr(5, 2));
  if (!piece) {
    return u8"指し手を読み取れませんでした";
  }
  if (!msg.substr(1).starts_with(to_string(fromFile) + to_string(fromRank) + to_string(toFile) + to_string(toRank))) {
    return u8"指し手を読み取れませんでした";
  }
  if (fromFile < 0 || 9 < fromFile || fromRank < 0 || 9 < fromRank || toFile < 1 || 9 < toFile || toRank < 1 || 9 < toRank) {
    return u8"指し手を読み取れませんでした";
  }
  if ((fromFile == 0 && fromRank != 0) || (fromFile != 0 && fromRank == 0)) {
    return u8"指し手を読み取れませんでした";
  }
  Move mv;
  mv.color = color;
  mv.piece = *piece | static_cast<PieceUnderlyingType>(color);
  mv.to = MakeSquare(9 - toFile, toRank - 1);
  if (fromFile != 0 && fromRank != 0) {
    Square from = MakeSquare(9 - fromFile, fromRank - 1);
    Piece existing = position.pieces[from.file][from.rank];
    if (existing == 0) {
      return u8"盤上にない駒の移動が指定されました";
    }
    if (existing == mv.piece) {
      if (CanPromote(mv.piece) && IsPromotableMove(from, mv.to, color)) {
        mv.promote = -1;
      }
    } else {
      if (Promote(existing) == mv.piece) {
        mv.promote = 1;
      } else {
        return u8"不正な指し手が指定されました";
      }
    }
    mv.from = from;
  }
  Piece captured = position.pieces[mv.to.file][mv.to.rank];
  if (captured != 0) {
    mv.captured = RemoveColorFromPiece(captured);
  }
  mv.decideSuffix(position);
  return mv;
}

} // namespace sci
